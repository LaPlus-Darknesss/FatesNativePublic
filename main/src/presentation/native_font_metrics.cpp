#include "fates/presentation/native_font_metrics.hpp"
#include <algorithm>
#include <bit>
#include <limits>
#include <map>

namespace fates::presentation::native {
using namespace runtime::native;
using namespace io::native;
using S=FontMetricStatus;
namespace {
constexpr std::uint32_t Current=0x3cf9a4,Set=0x3cfa10,MaxWidth=0x3cfa68,MaxHeight=0x3cfa7c,
    Width=0x3cff30,Lines=0x3cff00,Push=0x3d0018,PushType=0x3cffd8,Pop=0x3cfebc,Reset=0x125f2c;
constexpr std::array Targets{Current,Set,MaxWidth,MaxHeight,Width,Lines,Push,PushType,Pop,Reset};
constexpr std::size_t ReadBound=1u<<20;
ProcessCall Service(ProcessHandle process,std::uint32_t target,std::initializer_list<std::uint32_t> arguments={}) {
    ProcessCall call;call.process=std::move(process);call.target=target;call.kind=ProcessCallKind::Service;
    call.argument_count=static_cast<std::uint8_t>(arguments.size());std::copy(arguments.begin(),arguments.end(),call.arguments.begin());return call;
}
std::uint16_t U16(std::span<const std::uint8_t> bytes,std::size_t at){return std::uint16_t(std::uint16_t(bytes[at])|(std::uint16_t(bytes[at+1])<<8));}
std::uint32_t U32(std::span<const std::uint8_t> bytes,std::size_t at){return std::uint32_t(U16(bytes,at))|(std::uint32_t(U16(bytes,at+2))<<16);}
std::uint32_t Maximum(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b)?b:a;}
}
struct NativeFontMetrics::State {
    struct Data final:FileDataResource {
        FileObjectHandle object;std::shared_ptr<const NativeFileImage> image;
        std::uint64_t revision{},generation{};std::uint32_t data{},start{},physical{},count{};
    };
    struct Font {std::weak_ptr<const Data> data;std::uint16_t absent{};std::uint32_t cache{};std::uint64_t generation{};};
    struct Text {FontTextObservation value;Reader reader;FontTextOperation operation{};};
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<NativeFileController> files;std::shared_ptr<NativeFileEntry> entry;
    std::shared_ptr<NativeFileBase> bases;std::optional<std::array<FileBaseHandle,4>> file_slots;
    std::map<std::uint32_t,Font> fonts;std::map<std::uint32_t,std::shared_ptr<Text>> texts;
    FontSelectionState selection;std::uint32_t serial{};
    S Mutable(ProcessAccess* access) const {
        const auto s=scheduler.lock();if(!s || !s->root(2))return S::Retired;
        if(access)return access->BelongsTo(*s)?S::Ready:S::MismatchedDomain;
        return s->busy()?S::Busy:S::Ready;
    }
    std::optional<FileObjectHandle> Slot(std::size_t index) const {
        if(!file_slots)return selection.slots[index];
        const auto slot=bases->Observe((*file_slots)[index]);
        return slot?std::optional<FileObjectHandle>{slot->object}:std::nullopt;
    }
    bool Live() const {const auto s=scheduler.lock();return s && s->root(2);}
    std::shared_ptr<const Data> Get(FileObjectHandle object) const {
        if(!Live() || !object)return {};
        const auto found=fonts.find(object->serial);if(found==fonts.end())return {};
        auto data=found->second.data.lock();if(!data || data->object!=object || !files->OwnsLiveData(object,data->data,data->revision))return {};
        return data;
    }
    std::shared_ptr<Text> Get(FontTextRequest request) const {
        if(!request)return {};
        const auto found=texts.find(request->serial);return found!=texts.end() && found->second->value.identity==request?found->second:nullptr;
    }
    std::uint32_t Lower(const Data& data,std::uint16_t code) const {
        std::uint32_t first=0,count=data.count;const auto bytes=data.image->bytes();
        while(count){const auto half=count/2;const auto middle=first+half;
            if(U16(bytes,data.start+std::size_t(middle)*16)<code){first=middle+1;count-=half+1;}else count=half;}
        return first;
    }
    FontGlyphResult Glyph(FileObjectHandle object,char16_t code) {
        const auto data=Get(object);if(!data)return {S::StaleData,{}};
        auto& font=fonts.at(object->serial);const auto bytes=data->image->bytes();
        if(font.cache>=data->count)return {S::InvalidData,{}};
        if(U16(bytes,data->start+std::size_t(font.cache)*16)!=static_cast<std::uint16_t>(code)) {
            font.cache=Lower(*data,static_cast<std::uint16_t>(code));
            if(font.cache==data->count || U16(bytes,data->start+std::size_t(font.cache)*16)!=static_cast<std::uint16_t>(code)) {
                font.cache=Lower(*data,font.absent);
                if(font.cache==data->count || U16(bytes,data->start+std::size_t(font.cache)*16)!=font.absent)font.cache=0;
            }
        }
        return {S::Ready,{object,data->revision,data->generation,data->start+font.cache*16u}};
    }
};
struct NativeFontMetrics::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;ProcessCall call;std::shared_ptr<State::Text> text;
    std::optional<FileObjectHandle> captured;std::uint32_t value{},line_width{},maximum{},line_count{1};
    std::size_t cursor{};unsigned stage{};char16_t word{};
    ProcessCallbackStep Block(S status=S::Unavailable){if(text)text->value.status=status;return ProcessCallbackStep::Blocked();}
    ProcessCallbackStep Done(std::uint32_t result=0){if(text){text->value.status=S::Ready;text->value.completed=true;text->value.result=result;}return ProcessCallbackStep::Return(result);}
    std::optional<char16_t> Read(){if(text->value.read_attempts>=ReadBound)return {}; ++text->value.read_attempts;return text->reader(cursor);}
    ProcessCallbackStep Select(){
        // Original compares the full input word against4, but stores just its
        // byte. A wrapped byte0 from type256 still selects known-null.
        state->selection.current_type=static_cast<std::uint8_t>(value);
        if(value>=4){state->selection.current=FileObjectHandle{};return Done();}
        const auto slot=state->Slot(value);if(!slot)return Block();
        state->selection.current=*slot;return Done();
    }
    ProcessCallbackStep Measure(){
        if(text->value.read_attempts>=ReadBound)return Block(S::ReadLimit);
        if(stage==0){captured=state->selection.current;if(!captured)return Block();stage=1;}
        for(;;){
            if(stage==1){const auto read=Read();if(!read)return Block();word=*read;if(!word)return Done(line_width);stage=2;}
            if(stage==2){
                if(word==u'\n'){maximum=Maximum(line_width,maximum);line_width=0;++cursor;stage=5;}
                else stage=3;
            }
            if(stage==3){
                // GetWidth's test and GetGlyphWidth's actual input load are TWO
                // reads. A live source may change between them.
                const auto read=Read();if(!read)return Block();word=*read;stage=4;
            }
            if(stage==4){
                const auto glyph=state->Glyph(*captured,word);if(glyph.status!=S::Ready)return Block(glyph.status);
                const auto data=state->Get(*captured);if(!data)return Block(S::StaleData);
                const auto byte=data->image->bytes()[std::size_t(glyph.view.offset)+12];
                line_width+=std::bit_cast<std::uint32_t>(static_cast<std::int32_t>(std::bit_cast<std::int8_t>(byte)));
                ++cursor;stage=5;
            }
            if(stage==5){const auto read=Read();if(!read)return Block();word=*read;if(!word)return Done(Maximum(line_width,maximum));stage=2;}
        }
    }
    ProcessCallbackStep CountLines(){
        for(;;){
            if(text->value.read_attempts>=ReadBound)return Block(S::ReadLimit);
            if(stage==0){const auto read=Read();if(!read)return Block();if(!*read)return Done(1);stage=1;}
            if(stage==1){const auto read=Read();if(!read)return Block();word=*read;++cursor;stage=2;}
            if(stage==2){const auto read=Read();if(!read)return Block();if(word==u'\n')++line_count;if(!*read)return Done(line_count);stage=1;}
        }
    }
    ProcessCallbackStep Step(ProcessAccess& access) override {
        const auto scheduler=state->scheduler.lock();if(!scheduler || !access.BelongsTo(*scheduler))return Block();
        if(text && state->Get(text->value.identity)!=text)return Block();
        if(call.target==Width)return Measure();
        if(call.target==Lines)return CountLines();
        if(call.target==Current)return state->selection.current_type?Done(*state->selection.current_type):Block();
        if(call.target==MaxWidth || call.target==MaxHeight){
            if(!state->selection.current || !*state->selection.current)return Block();
            const auto data=state->Get(*state->selection.current);if(!data)return Block(S::StaleData);
            const auto bytes=data->image->bytes();return Done(call.target==MaxWidth?U16(bytes,12):std::uint32_t(U16(bytes,8))+U16(bytes,10));
        }
        if(call.target==Reset){if(!state->selection.stack)return Block();state->selection.stack->clear();return Done();}
        if(stage==0){
            if(call.target==Push || call.target==PushType){
                if(!state->selection.stack || !state->selection.current_type)return Block();
                if(state->selection.stack_capacity && state->selection.stack->size()==*state->selection.stack_capacity) {
                    const auto size=static_cast<std::uint64_t>(state->selection.stack->size());
                    const auto capacity=std::max(size+32,size+(size>>1u)+(size>>3u));
                    if(capacity>std::numeric_limits<std::uint32_t>::max())return Block(S::InvalidData);
                    state->selection.stack->reserve(static_cast<std::size_t>(capacity));
                    state->selection.stack_capacity=static_cast<std::uint32_t>(capacity);
                }
                state->selection.stack->push_back(*state->selection.current_type);
                if(call.target==Push)return Done();
                value=call.arguments[0];
            }else if(call.target==Pop){
                if(!state->selection.stack)return Block();
                if(state->selection.stack->empty())return Block(S::StackUnderflow);
                value=state->selection.stack->back();state->selection.stack->pop_back();
            }else value=call.arguments[0];
            stage=1;
        }
        return Select();
    }
};
NativeFontMetrics::NativeFontMetrics(std::shared_ptr<State> state):state_(std::move(state)){}
NativeFontMetrics::~NativeFontMetrics()=default;
S NativeFontMetrics::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<NativeFileController> files,std::shared_ptr<NativeFileBase> bases,std::shared_ptr<NativeFileEntry> entry,std::shared_ptr<NativeFontMetrics>& out){
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !files || !files->UsesScheduler(*scheduler) || !bases || !entry || !entry->UsesOwners(*files,*bases))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->files=std::move(files);state->entry=std::move(entry);state->bases=std::move(bases);
    auto next=std::shared_ptr<NativeFontMetrics>(new NativeFontMetrics(state));if(!registry->Register(Targets,next))return S::DuplicateBinding;
    out=std::move(next);return S::Ready;
}
S NativeFontMetrics::SetupMetrics(FileObjectHandle object,ProcessAccess* access){
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    const auto lower=state_->files->Observe(object);if(!lower || lower->life!=FileObjectLife::Live)return S::InvalidObject;
    const auto read=state_->entry->ReadResult(object);if(!read || !read->image || !read->data || read->data!=lower->fields.data)return S::DataUnavailable;
    const auto bytes=read->image->bytes();if(bytes.size()<40 || bytes.size()!=read->size || read->size!=lower->fields.size)return S::InvalidData;
    const auto start=std::uint32_t(U16(bytes,32)),end=U32(bytes,36);
    if(end<start || end>bytes.size())return S::InvalidData;
    const auto physical=(end-start)>>4;auto count=physical;
    if(!count || start<40)return S::InvalidData;
    while(count && U16(bytes,start+std::size_t(count-1)*16)==0)--count;
    if(!count)return S::InvalidData; // Original would read before the table.
    auto& font=state_->fonts[object->serial];if(font.generation==std::numeric_limits<std::uint64_t>::max())return S::IdentityExhausted;
    auto data=std::make_shared<State::Data>();data->object=object;data->image=read->image;data->data=read->data;
    data->revision=lower->data_revision;data->generation=font.generation+1;data->start=start;data->count=count;data->physical=physical;
    if(state_->files->RetainDataResource(object,read->data,lower->data_revision,data,access)!=FileControllerStatus::Ready)return S::StaleData;
    font.data=data;font.generation=data->generation;font.cache=0;return S::Ready;
}
S NativeFontMetrics::SetAbsentCode(FileObjectHandle object,std::optional<char16_t> code,ProcessAccess* access){
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    const auto data=state_->Get(object);if(!data)return S::StaleData;
    const auto value=code && *code?*code:char16_t{0xff1f};
    const auto selected=state_->Glyph(object,value);if(selected.status!=S::Ready)return selected.status;
    state_->fonts.at(object->serial).absent=selected.view.offset==data->start?std::uint16_t{0x3f}:static_cast<std::uint16_t>(value);return S::Ready;
}
FontGlyphResult NativeFontMetrics::GetGlyph(FileObjectHandle object,char16_t code,ProcessAccess* access){
    if(const auto status=state_->Mutable(access);status!=S::Ready)return {status,{}};
    return state_->Glyph(object,code);
}
S NativeFontMetrics::Describe(const FontMetricView& view,FontGlyphRecord& out) const {
    const auto data=state_->Get(view.object);if(!data || view.data_revision!=data->revision || view.setup_generation!=data->generation)return S::StaleData;
    if(view.offset<data->start || (view.offset-data->start)%16 || (view.offset-data->start)/16>=data->count)return S::InvalidData;
    FontGlyphRecord value;const auto bytes=data->image->bytes();std::copy_n(bytes.begin()+view.offset,16,value.bytes.begin());
    value.code=U16(value.bytes,0);value.advance=std::bit_cast<std::int8_t>(value.bytes[12]);out=value;return S::Ready;
}
std::optional<FontMetricObservation> NativeFontMetrics::Observe(FileObjectHandle object) const {
    const auto data=state_->Get(object);if(!data)return {};
    const auto& font=state_->fonts.at(object->serial);const auto bytes=data->image->bytes();
    return FontMetricObservation{object,data->revision,data->generation,data->physical,data->count,font.cache,font.absent,U16(bytes,12),std::uint32_t(U16(bytes,8))+U16(bytes,10),U16(bytes,8),U16(bytes,26)};
}
S NativeFontMetrics::RestoreSelection(const FontSelectionState& value,ProcessAccess* access){
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    if(value.stack_capacity && (!value.stack || value.stack->size()>*value.stack_capacity))return S::InvalidData;
    for(const auto& slot:value.slots)if(slot && *slot && !state_->files->Observe(*slot))return S::InvalidObject;
    if(value.current && *value.current && !state_->files->Observe(*value.current))return S::InvalidObject;
    if(state_->file_slots)for(std::size_t i=0;i<4;++i)
        if(value.slots[i]!=state_->Slot(i))return S::InvalidObject;
    state_->selection=value;return S::Ready;
}
std::optional<FontSelectionState> NativeFontMetrics::Selection() const {
    if(!state_->Live())return {};
    auto value=state_->selection;
    if(state_->file_slots)for(std::size_t i=0;i<4;++i)value.slots[i]=state_->Slot(i);
    return value;
}
bool NativeFontMetrics::CanConstructStaticSelection() const {
    const auto& value=state_->selection;
    return state_->Live() && !state_->file_slots && !value.current_type && !value.current &&
        !value.stack && !value.stack_capacity &&
        std::none_of(value.slots.begin(),value.slots.end(),[](const auto& slot){return slot.has_value();});
}
S NativeFontMetrics::ReserveStartupStack(ProcessAccess& access) {
    if(const auto status=state_->Mutable(&access);status!=S::Ready)return status;
    auto& selection=state_->selection;
    if(!selection.stack || !selection.stack_capacity)return S::Unavailable;
    if(*selection.stack_capacity<64) {
        const auto size=static_cast<std::uint32_t>(selection.stack->size());
        const auto capacity=std::max({64u,size+32u,size+(size>>1u)+(size>>3u)});
        selection.stack->reserve(capacity);selection.stack_capacity=capacity;
    }
    return S::Ready;
}
S NativeFontMetrics::PrepareText(Reader reader,FontTextOperation operation,FontTextRequest& out,ProcessAccess* access){
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    if(!reader)return S::Unavailable;
    if(operation!=FontTextOperation::Width && operation!=FontTextOperation::Lines)return S::InvalidRequest;
    if(state_->serial==std::numeric_limits<std::uint32_t>::max())return S::IdentityExhausted;
    auto text=std::make_shared<State::Text>();text->value.identity=std::make_shared<FontTextIdentity>(++state_->serial);text->reader=std::move(reader);text->operation=operation;
    state_->texts.emplace(text->value.identity->serial,text);out=text->value.identity;return S::Ready;
}
std::optional<ProcessCall> NativeFontMetrics::TextCall(ProcessHandle process,FontTextRequest request) const {
    const auto text=state_->Get(request);if(!state_->Live() || !text || text->value.completed)return {};
    return Service(std::move(process),text->operation==FontTextOperation::Width?Width:Lines,{request->serial});
}
std::optional<FontTextObservation> NativeFontMetrics::Observe(FontTextRequest request) const {const auto text=state_->Get(request);return state_->Live() && text?std::optional{text->value}:std::nullopt;}
S NativeFontMetrics::Release(FontTextRequest request,ProcessAccess* access){
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    if(!state_->Get(request))return S::InvalidRequest;
    state_->texts.erase(request->serial);return S::Ready;
}
ProcessCall NativeFontMetrics::SetCurrentCall(ProcessHandle process,std::uint32_t value){return Service(std::move(process),Set,{value});}
ProcessCall NativeFontMetrics::GetCurrentCall(ProcessHandle process){return Service(std::move(process),Current);}
ProcessCall NativeFontMetrics::MaxWidthCall(ProcessHandle process){return Service(std::move(process),MaxWidth);}
ProcessCall NativeFontMetrics::MaxHeightCall(ProcessHandle process){return Service(std::move(process),MaxHeight);}
ProcessCall NativeFontMetrics::PushCall(ProcessHandle process,std::optional<std::uint32_t> value){return value?Service(std::move(process),PushType,{*value}):Service(std::move(process),Push);}
ProcessCall NativeFontMetrics::PopCall(ProcessHandle process){return Service(std::move(process),Pop);}
ProcessCall NativeFontMetrics::ResetStackCall(ProcessHandle process){return Service(std::move(process),Reset);}
bool NativeFontMetrics::UsesScheduler(const NativeProcessScheduler& s) const noexcept{return state_->scheduler.lock().get()==&s;}
bool NativeFontMetrics::UsesOwners(const NativeFileController& files,const NativeFileEntry& entry) const noexcept{return state_->files.get()==&files && state_->entry.get()==&entry;}
bool NativeFontMetrics::UsesFileBaseOwners(const NativeFileController& files,const NativeFileBase& bases) const noexcept{return state_->files.get()==&files && state_->bases.get()==&bases;}
std::unique_ptr<ProcessContinuation> NativeFontMetrics::Begin(const ProcessCall& call){
    if(!state_->Live() || !call.process || call.has_self || call.this_adjustment || call.kind!=ProcessCallKind::Service || std::find(Targets.begin(),Targets.end(),call.target)==Targets.end())return {};
    const bool arg=call.target==Set || call.target==PushType || call.target==Width || call.target==Lines;
    if(call.argument_count!=(arg?1u:0u))return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->call=call;
    if(call.target==Width || call.target==Lines){const auto found=state_->texts.find(call.arguments[0]);if(found==state_->texts.end() || found->second->value.completed)return {};
        if((found->second->operation==FontTextOperation::Width)!=(call.target==Width))return {};
        next->text=found->second;}
    return next;
}
}

namespace fates::presentation::native {void NativeFontMetrics::ForgetText(FontTextRequest request) noexcept {if(request)state_->texts.erase(request->serial);}}

namespace fates::presentation::native {
FontMetricStatus NativeFontMetrics::RetireMetrics(io::native::FileObjectHandle object,runtime::native::ProcessAccess& access) {
    if(const auto status=state_->Mutable(&access);status!=FontMetricStatus::Ready)return status;
    const auto found=object?state_->fonts.find(object->serial):state_->fonts.end();
    if(!object || !state_->files->Observe(object))return FontMetricStatus::InvalidObject;
    if(found!=state_->fonts.end()) {
        if(const auto data=found->second.data.lock();data && data->object!=object)return FontMetricStatus::InvalidObject;
        found->second.data.reset();found->second.cache=0;
        // ISFont::~ISFont clears the data/count/cache pointers, not absent code.
    }
    return FontMetricStatus::Ready;
}
}

namespace fates::presentation::native {
FontMetricStatus NativeFontMetrics::BindFileSlots(const std::array<io::native::FileBaseHandle,4>& slots,
    runtime::native::ProcessAccess* access) {
    if(const auto status=state_->Mutable(access);status!=FontMetricStatus::Ready)return status;
    for(std::size_t i=0;i<slots.size();++i) {
        if(!state_->bases->Observe(slots[i]))return FontMetricStatus::InvalidObject;
        for(std::size_t j=0;j<i;++j)if(slots[i]==slots[j])return FontMetricStatus::InvalidObject;
    }
    if(state_->file_slots && *state_->file_slots!=slots)return FontMetricStatus::InvalidObject;
    state_->file_slots=slots;return FontMetricStatus::Ready;
}
}
