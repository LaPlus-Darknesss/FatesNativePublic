#include "fates/presentation/native_talk_window.hpp"
#include <bit>
#include <limits>
#include <map>

namespace fates::presentation::native {
using namespace runtime::native;
using namespace io::native;
using S=TalkWindowStatus;
namespace {
constexpr std::uint32_t Destroy=0x190168;
constexpr std::size_t NameReadBound=65536;
void Clear(TalkStringStorage& text) {text.words[0]=0;text.known.set(0);text.length=0;text.display=0;}
void ClearStrings(TalkWindowStorage& row) {
    row.next_icon=255;row.line=0;row.x_offset=0;row.new_string=1;
    for(auto& text:row.strings)Clear(text);
}
void ResetLocal(TalkWindowStorage& row,const TalkWindowPalette& colors) {
    row.talk_type=std::uint8_t{255};row.state=255;row.location=std::uint8_t{9};
    row.name_plate.name.reset();row.name_plate.words.reset();row.name_override=0;
    row.current=0;row.offset_position={};row.frame_color=colors.frame;row.frame_color[3]=0;
    row.font_color=colors.white;row.active=0;ClearStrings(row);
}
S NextLocal(TalkWindowStorage& row) {
    if(!row.text_layout)return S::LayoutUnavailable;
    // The actual expression is signed remainder of the wrapped current+1 word.
    const auto next=std::bit_cast<std::int32_t>(row.current+1u)%8;
    if(next<0)return S::StorageOverflow;
    row.current=static_cast<std::uint32_t>(next);auto& text=row.strings[row.current];Clear(text);
    text.color=row.font_color;
    const auto layout=*row.text_layout;
    const auto x=std::bit_cast<std::int32_t>(std::uint32_t(layout[0])+row.x_offset);
    const auto y=std::bit_cast<std::int32_t>(row.line*std::uint32_t(layout[2])+std::uint32_t(layout[1]));
    text.position={std::bit_cast<std::uint32_t>(static_cast<float>(x)),std::bit_cast<std::uint32_t>(static_cast<float>(y)),0};
    return S::Ready;
}
}
struct NativeTalkWindow::State {
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<ObjectHandleRegistry> objects;
    std::shared_ptr<NativeFileController> files;
    std::shared_ptr<NativeFileBase> bases;
    std::shared_ptr<NativeTexFiles> tex;
    std::optional<TalkWindowPalette> palette;
    std::map<std::uint32_t,TalkWindowStorage> rows;
    std::uint32_t serial{};std::uint64_t name_serial{};
    bool Live() const {const auto owner=scheduler.lock();return owner && owner->root(2);}
    S Mutable(ProcessAccess* access) const {
        const auto owner=scheduler.lock();if(!owner || !owner->root(2))return S::Retired;
        if(access)return access->BelongsTo(*owner)?S::Ready:S::MismatchedDomain;
        return owner->busy()?S::Busy:S::Ready;
    }
    TalkWindowStorage* Get(TalkWindowHandle handle) {
        if(!Live() || !handle)return nullptr;
        const auto it=rows.find(handle->serial);return it!=rows.end() && it->second.identity==handle?&it->second:nullptr;
    }
    bool Entry(TalkMemberIdentity& value) {
        value.identity=objects->NewIdentity();if(!value.identity)return false;
        objects->Entry(value.handle,value.identity);return true; // Original registry exhaustion leaves handle0.
    }
    void Remove(TalkMemberIdentity& value) {objects->Remove(value.handle);}
    TalkWindowRead Read(const TalkWindowSource& source,std::size_t index) {
        if(!Live())return {S::Retired,0};
        if(index>std::numeric_limits<std::size_t>::max()-source.offset)return {S::InvalidSource,0};
        index+=source.offset;
        if(source.kind==TalkWindowSource::Kind::Words) {
            if(!source.words || index>=source.words->size())return {S::InvalidSource,0};
            return {S::Ready,(*source.words)[index]};
        }
        if(source.kind==TalkWindowSource::Kind::Message) {
            if(!source.messages)return {S::InvalidSource,0};
            const auto result=source.messages->ReadSourceWord(source.message,index);
            return {result.status==MessageLookupStatus::Ready?S::Ready:S::StaleSource,result.value};
        }
        if(source.kind==TalkWindowSource::Kind::UnitEdit) {
            if(!source.unit_names)return {S::InvalidSource,0};
            const auto result=source.unit_names->ReadEditSourceWord(source.edit_name_source,index);
            return {result.status==MessageLookupStatus::Ready?S::Ready:S::StaleSource,result.value};
        }
        if(source.kind==TalkWindowSource::Kind::Expanded) {
            if(!source.expanded || index>=8192)return {S::InvalidSource,0};
            if(!source.expanded->live())return {S::StaleSource,0};
            const auto& buffer=source.expanded->buffer();if(!buffer.known.test(index))return {S::UnknownWord,0};
            return {S::Ready,buffer.words[index]};
        }
        if(source.kind==TalkWindowSource::Kind::WindowString) {
            const auto* row=Get(source.string_view.window);
            if(!row)return {S::StaleSource,0};
            if(source.string_view.slot>=8 || index>=65)return {S::InvalidSource,0};
            const auto& text=row->strings[source.string_view.slot];if(!text.known.test(index))return {S::UnknownWord,0};
            return {S::Ready,text.words[index]};
        }
        if(source.kind==TalkWindowSource::Kind::Name) {
            const auto* row=Get(source.name_view.window);
            if(!row || !source.name_view.allocation || row->name_plate.name!=source.name_view.allocation)return {S::StaleSource,0};
            if(index>std::numeric_limits<std::size_t>::max()-source.name_view.offset)return {S::InvalidSource,0};
            index+=source.name_view.offset;
            if(!row->name_plate.words || index>=row->name_plate.words->size())return {S::InvalidSource,0};
            return {S::Ready,(*row->name_plate.words)[index]};
        }
        return {S::InvalidSource,0};
    }
};
struct NativeTalkWindow::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;ProcessCall call;TalkWindowHandle identity;unsigned stage{};
    ProcessCallbackStep Step(ProcessAccess& access) override {
        if(state->Mutable(&access)!=S::Ready)return ProcessCallbackStep::Blocked();
        auto* row=state->Get(identity);if(!row)return ProcessCallbackStep::Blocked();
        if(stage==0) {
            if(!state->palette)return ProcessCallbackStep::Blocked();
            ResetLocal(*row,*state->palette);
            // Embedded NamePlate destructor: name, last color, first color, movable.
            for(std::size_t i=2;i>0;--i)state->Remove(row->name_plate.color_members[i-1]);
            state->Remove(row->name_plate.movable);
            // Original vector destructor iterates from string7 back through0.
            for(std::size_t i=8;i>0;--i)state->Remove(row->strings[i-1].movable);
            state->Remove(row->color_member);state->Remove(row->offset);stage=1;
        }
        if(stage==1) {
            const auto child=state->tex->DestroyCall(call.process,row->texture,false);
            if(!child)return ProcessCallbackStep::Blocked();
            stage=2;return ProcessCallbackStep::Call(*child);
        }
        state->Remove(row->movable);state->rows.erase(identity->serial);return ProcessCallbackStep::Return();
    }
};
TalkWindowSource TalkWindowSource::Text(std::u16string_view input) {std::vector<char16_t> text(input.begin(),input.end());text.push_back(0);return Words(text);}
TalkWindowSource TalkWindowSource::Words(std::span<const char16_t> words) {
    TalkWindowSource value;value.kind=Kind::Words;value.words=std::make_shared<const std::vector<char16_t>>(words.begin(),words.end());return value;
}
TalkWindowSource TalkWindowSource::Message(std::shared_ptr<NativeMessageLookup> owner,MessageLookupResult result) {
    TalkWindowSource value;value.kind=Kind::Message;value.messages=std::move(owner);value.message=std::move(result);return value;
}
NativeTalkWindow::NativeTalkWindow(std::shared_ptr<State> state):state_(std::move(state)){}
NativeTalkWindow::~NativeTalkWindow()=default;
S NativeTalkWindow::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<ObjectHandleRegistry> objects,std::shared_ptr<NativeFileController> files,std::shared_ptr<NativeFileBase> bases,
    std::shared_ptr<NativeTexFiles> tex,std::shared_ptr<NativeTalkWindow>& out) {
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !objects || !files || !files->UsesScheduler(*scheduler) ||
        !bases || !bases->UsesController(*files) || !tex || !tex->UsesOwners(*files,*bases))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->objects=std::move(objects);state->files=std::move(files);state->bases=std::move(bases);state->tex=std::move(tex);
    auto owner=std::shared_ptr<NativeTalkWindow>(new NativeTalkWindow(state));
    if(!registry->Register(std::array{Destroy},owner))return S::DuplicateBinding;
    out=std::move(owner);return S::Ready;
}
S NativeTalkWindow::PublishPalette(std::optional<TalkWindowPalette> palette,ProcessAccess* access) {
    if(auto status=state_->Mutable(access);status!=S::Ready)return status;
    state_->palette=palette;return S::Ready;
}
S NativeTalkWindow::Construct(TalkWindowHandle& out,ProcessAccess* access) {
    if(auto status=state_->Mutable(access);status!=S::Ready)return status;
    if(!state_->palette)return S::PaletteUnavailable;
    if(state_->serial==std::numeric_limits<std::uint32_t>::max())return S::IdentityExhausted;
    TalkWindowStorage row;row.identity=std::make_shared<TalkWindowIdentity>(++state_->serial);
    std::vector<TalkMemberIdentity*> created;
    auto add=[&](TalkMemberIdentity& v){if(!state_->Entry(v))return false;created.push_back(&v);return true;};
    auto rollback=[&] {for(auto it=created.rbegin();it!=created.rend();++it)state_->Remove(**it);if(row.texture)state_->bases->RetireEmpty(row.texture,access);};
    if(!add(row.movable)){rollback();return S::IdentityExhausted;}
    if(state_->tex->Construct(row.texture,access)!=TexFileStatus::Ready){rollback();return S::Unavailable;}
    if(!add(row.offset) || !add(row.color_member)){rollback();return S::IdentityExhausted;}
    row.frame_color=state_->palette->frame;row.frame_color[3]=0;row.font_color=state_->palette->white;
    for(auto& text:row.strings) {
        if(!add(text.movable)){rollback();return S::IdentityExhausted;}
        text.color=state_->palette->white;Clear(text);
    }
    if(!add(row.name_plate.movable) || !add(row.name_plate.color_members[0]) || !add(row.name_plate.color_members[1])){rollback();return S::IdentityExhausted;}
    row.name_plate.colors={TalkColorBytes{255,255,255,0},state_->palette->black};row.name_plate.colors[1][3]=0;
    out=row.identity;state_->rows.emplace(row.identity->serial,std::move(row));return S::Ready;
}
S NativeTalkWindow::Reset(TalkWindowHandle handle,ProcessAccess* access) {
    if(auto status=state_->Mutable(access);status!=S::Ready)return status;
    auto* row=state_->Get(handle);if(!row)return S::InvalidHandle;
    if(!state_->palette)return S::PaletteUnavailable;
    ResetLocal(*row,*state_->palette);return S::Ready;
}
S NativeTalkWindow::InitializeWithoutFace(TalkWindowHandle handle,std::int32_t type,std::uint32_t location,bool skipped,ProcessAccess& access) {
    if(auto status=state_->Mutable(&access);status!=S::Ready)return status;
    auto* row=state_->Get(handle);if(!row)return S::InvalidHandle;
    row->talk_type=static_cast<std::uint8_t>(type);row->location=static_cast<std::uint8_t>(location);
    row->name_effect_enabled=std::uint8_t{0};
    if(skipped)row->active=1;
    // Both normal mode-specific tails return immediately on the admitted null
    // attached-face pointer. No face creation or animation is synthesized here.
    return S::Ready;
}
S NativeTalkWindow::ResetSystem(TalkWindowHandle handle,ProcessAccess* access) {
    if(auto status=state_->Mutable(access);status!=S::Ready)return status;
    auto* row=state_->Get(handle);if(!row)return S::InvalidHandle;
    if(!state_->palette)return S::PaletteUnavailable;
    row->position={0x429c0000,0x43660000,0};row->font_color=state_->palette->white;return S::Ready;
}
S NativeTalkWindow::ResetStrings(TalkWindowHandle handle,ProcessAccess* access) {
    if(auto status=state_->Mutable(access);status!=S::Ready)return status;
    auto* row=state_->Get(handle);if(!row)return S::InvalidHandle;
    ClearStrings(*row);return S::Ready;
}
S NativeTalkWindow::NextString(TalkWindowHandle handle,ProcessAccess* access) {
    if(auto status=state_->Mutable(access);status!=S::Ready)return status;
    auto* row=state_->Get(handle);if(!row)return S::InvalidHandle;
    return NextLocal(*row);
}
S NativeTalkWindow::NextLine(TalkWindowHandle handle,ProcessAccess* access) {
    if(auto status=state_->Mutable(access);status!=S::Ready)return status;
    auto* row=state_->Get(handle);if(!row)return S::InvalidHandle;
    auto next=*row;++next.line;next.x_offset=0;
    const auto result=NextLocal(next);if(result==S::Ready)*row=std::move(next);return result;
}
S NativeTalkWindow::AddLetter(TalkWindowHandle handle,const TalkWindowSource& source,ProcessAccess* access) {
    if(auto status=state_->Mutable(access);status!=S::Ready)return status;
    auto* row=state_->Get(handle);if(!row)return S::InvalidHandle;
    auto next=*row;
    if(next.new_string){next.new_string=0;if(auto result=NextLocal(next);result!=S::Ready)return result;}
    if(next.current>=8)return S::StorageOverflow;
    // Read-after-NextString matters for a source alias to the same ring slot.
    TalkWindowRead read;
    if(source.kind==TalkWindowSource::Kind::WindowString && source.string_view.window==handle) {
        if(source.string_view.slot>=8 || source.offset>=65)return S::InvalidSource;
        const auto& text=next.strings[source.string_view.slot];
        read={text.known.test(source.offset)?S::Ready:S::UnknownWord,text.words[source.offset]};
    } else read=state_->Read(source,0);
    if(read.status!=S::Ready)return read.status;
    auto& text=next.strings[next.current];
    // Retail signed-byte index can escape into adjacent fields. Refuse unsafe
    // host writes, rather than clamp/truncate or claim retail bounds checking.
    if(text.length>=64)return S::StorageOverflow;
    text.words[text.length]=read.value;text.known.set(text.length);++text.length;
    text.words[text.length]=0;text.known.set(text.length);*row=std::move(next);return S::Ready;
}
S NativeTalkWindow::SetName(TalkWindowHandle handle,const TalkWindowSource& source,bool explicit_override,ProcessAccess* access) {
    if(auto status=state_->Mutable(access);status!=S::Ready)return status;
    auto* row=state_->Get(handle);if(!row)return S::InvalidHandle;
    if(source.kind==TalkWindowSource::Kind::Name && source.name_view.window==handle && source.name_view.allocation==row->name_plate.name)
        return S::StaleSource; // Original would free its own input before reading: outside safe admitted domain.
    std::vector<char16_t> words;
    if(source.kind!=TalkWindowSource::Kind::Null) {
        for(std::size_t i=0;i<NameReadBound;++i) {
            const auto read=state_->Read(source,i);if(read.status!=S::Ready)return read.status;
            words.push_back(read.value);if(!read.value)break;
        }
        if(words.empty() || words.back()!=0)return S::StorageOverflow;
        if(state_->name_serial==std::numeric_limits<std::uint64_t>::max())return S::IdentityExhausted;
    }
    row->name_plate.name.reset();row->name_plate.words.reset();
    if(source.kind!=TalkWindowSource::Kind::Null) {
        row->name_plate.name=std::make_shared<TalkNameIdentity>(++state_->name_serial);
        row->name_plate.words=std::make_shared<const std::vector<char16_t>>(std::move(words));
    }
    if(explicit_override)row->name_override=1;
    return S::Ready;
}
S NativeTalkWindow::RestoreTextLayout(TalkWindowHandle handle,std::optional<std::array<std::uint8_t,3>> layout,ProcessAccess* access) {
    if(auto status=state_->Mutable(access);status!=S::Ready)return status;
    auto* row=state_->Get(handle);if(!row)return S::InvalidHandle;row->text_layout=layout;return S::Ready;
}
S NativeTalkWindow::RestoreCursor(TalkWindowHandle handle,std::uint32_t current,std::uint32_t line,std::uint32_t x,std::uint8_t fresh,ProcessAccess* access) {
    if(auto status=state_->Mutable(access);status!=S::Ready)return status;
    auto* row=state_->Get(handle);if(!row)return S::InvalidHandle;
    row->current=current;row->line=line;row->x_offset=x;row->new_string=fresh;return S::Ready;
}
S NativeTalkWindow::RestoreTypeLocation(TalkWindowHandle handle,std::optional<std::uint8_t> type,std::optional<std::uint8_t> loc,ProcessAccess* access) {
    if(auto status=state_->Mutable(access);status!=S::Ready)return status;
    auto* row=state_->Get(handle);if(!row)return S::InvalidHandle;row->talk_type=type;row->location=loc;return S::Ready;
}
S NativeTalkWindow::RestoreColors(TalkWindowHandle handle,TalkColorBytes frame,TalkColorBytes font,ProcessAccess* access) {
    if(auto status=state_->Mutable(access);status!=S::Ready)return status;
    auto* row=state_->Get(handle);if(!row)return S::InvalidHandle;row->frame_color=frame;row->font_color=font;return S::Ready;
}
S NativeTalkWindow::RestoreString(TalkWindowView view,const TalkStringStorage& value,ProcessAccess* access) {
    if(auto status=state_->Mutable(access);status!=S::Ready)return status;
    auto* row=state_->Get(view.window);if(!row || view.slot>=8)return S::InvalidHandle;
    const auto& old=row->strings[view.slot].movable;
    if(value.movable.handle!=old.handle || value.movable.identity!=old.identity)return S::InvalidHandle;
    row->strings[view.slot]=value;return S::Ready;
}
std::optional<std::uint32_t> NativeTalkWindow::Side(TalkWindowHandle handle) const {
    const auto* row=state_->Get(handle);if(!row || !row->talk_type)return {};
    const auto type=std::bit_cast<std::int8_t>(*row->talk_type);
    if(type!=0 && type!=1 && type!=2)return 1u;
    if(!row->location)return {};
    const auto loc=*row->location;
    if(type==0)return loc>=9?1u:(loc%3u==0?0u:(loc%3u==2?2u:1u));
    return loc<=5 || loc==103?0u:(loc>=7?2u:1u);
}
std::optional<TalkWindowStorage> NativeTalkWindow::Observe(TalkWindowHandle handle) const {
    const auto* row=state_->Get(handle);return row?std::optional(*row):std::nullopt;
}
std::vector<TalkWindowHandle> NativeTalkWindow::Handles() const {
    std::vector<TalkWindowHandle> out;if(!state_->Live())return out;
    for(const auto& [id,row]:state_->rows){(void)id;out.push_back(row.identity);}return out;
}
std::optional<TalkStringStorage> NativeTalkWindow::ObserveString(TalkWindowView view) const {
    const auto* row=state_->Get(view.window);if(!row || view.slot>=8)return {};return row->strings[view.slot];
}
std::optional<TalkNameView> NativeTalkWindow::NameView(TalkWindowHandle handle) const {
    const auto* row=state_->Get(handle);if(!row || !row->name_plate.name)return {};return TalkNameView{handle,row->name_plate.name,0};
}
TalkWindowRead NativeTalkWindow::Read(const TalkWindowSource& source,std::size_t index) const {return state_->Read(source,index);}
std::optional<ProcessCall> NativeTalkWindow::DestroyCall(ProcessHandle process,TalkWindowHandle handle) const {
    if(!state_->Get(handle))return {};
    ProcessCall call;call.process=std::move(process);call.kind=ProcessCallKind::Service;call.target=Destroy;call.argument_count=1;call.arguments[0]=handle->serial;return call;
}
bool NativeTalkWindow::UsesScheduler(const NativeProcessScheduler& owner) const noexcept{return state_->scheduler.lock().get()==&owner;}
std::optional<TalkVectorBits> NativeTalkWindow::ReadPosition(ObjectIdentity identity) const {
    if(!state_->Live() || !identity)return {};
    for(const auto& [serial,row]:state_->rows) {
        (void)serial;
        if(row.movable.identity==identity)return row.position;
        if(row.offset.identity==identity)return row.offset_position;
        if(row.name_plate.movable.identity==identity)return row.name_plate.position;
        for(const auto& text:row.strings)if(text.movable.identity==identity)return text.position;
    }
    return {};
}
bool NativeTalkWindow::WritePosition(ObjectIdentity identity,TalkVectorBits position,ProcessAccess& access) {
    if(state_->Mutable(&access)!=S::Ready || !identity)return false;
    for(auto& [serial,row]:state_->rows) {
        (void)serial;
        if(row.movable.identity==identity){row.position=position;return true;}
        if(row.offset.identity==identity){row.offset_position=position;return true;}
        if(row.name_plate.movable.identity==identity){row.name_plate.position=position;return true;}
        for(auto& text:row.strings)if(text.movable.identity==identity){text.position=position;return true;}
    }
    return false;
}
S NativeTalkWindow::StartPageState(TalkWindowHandle handle,ProcessAccess& access) {
    if(auto status=state_->Mutable(&access);status!=S::Ready)return status;
    auto* row=state_->Get(handle);if(!row)return S::InvalidHandle;
    row->next_icon=255;++row->line;row->x_offset=0;return S::Ready;
}
S NativeTalkWindow::FinishPageState(TalkWindowHandle handle,ProcessAccess& access) {
    if(auto status=state_->Mutable(&access);status!=S::Ready)return status;
    auto* row=state_->Get(handle);if(!row)return S::InvalidHandle;
    row->line=0;row->new_string=1;return S::Ready;
}
S NativeTalkWindow::ScrollString(TalkWindowView view,std::uint32_t elapsed,std::uint32_t duration,std::uint8_t fade,ProcessAccess& access) {
    if(auto status=state_->Mutable(&access);status!=S::Ready)return status;
    auto* row=state_->Get(view.window);if(!row || view.slot>=8)return S::InvalidHandle;
    auto& text=row->strings[view.slot];
    const auto d=std::bit_cast<std::int32_t>(duration),e=std::bit_cast<std::int32_t>(elapsed);
    const auto divisor=d<=1?1:d;const auto numerator=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(e>divisor?divisor:e)*255u);
    const auto quotient=numerator/divisor;
    text.color[3]=static_cast<std::uint8_t>(fade?255u-static_cast<std::uint32_t>(quotient):static_cast<std::uint32_t>(quotient));
    if(e>d && fade==1)Clear(text);
    return S::Ready;
}
bool NativeTalkWindow::UsesObjectRegistry(const ObjectHandleRegistry& objects) const noexcept{return state_->objects.get()==&objects;}
std::unique_ptr<ProcessContinuation> NativeTalkWindow::Begin(const ProcessCall& call) {
    if(!state_->Live() || call.kind!=ProcessCallKind::Service || call.target!=Destroy || call.has_self || call.this_adjustment || call.argument_count!=1 || !call.process)return {};
    const auto it=state_->rows.find(call.arguments[0]);if(it==state_->rows.end())return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->call=call;next->identity=it->second.identity;return next;
}
}

namespace fates::presentation::native {
std::optional<TalkColorBytes> NativeTalkWindow::ReadColor(runtime::native::ObjectIdentity identity) const {
    if(!identity || state_->Mutable(nullptr)==TalkWindowStatus::Retired)return {};
    for(const auto& [key,row]:state_->rows) {
        (void)key;
        if(row.color_member.identity==identity && state_->objects->Get(row.color_member.handle)==identity)return row.frame_color;
        for(unsigned i=0;i<2;++i)if(row.name_plate.color_members[i].identity==identity && state_->objects->Get(row.name_plate.color_members[i].handle)==identity)return row.name_plate.colors[i];
    }
    return {};
}
bool NativeTalkWindow::WriteColorChannel(runtime::native::ObjectIdentity identity,std::uint8_t channel,std::uint8_t value,runtime::native::ProcessAccess& access) {
    if(channel>=4 || !identity || state_->Mutable(&access)!=TalkWindowStatus::Ready)return false;
    for(auto& [key,row]:state_->rows) {
        (void)key;
        if(row.color_member.identity==identity && state_->objects->Get(row.color_member.handle)==identity){row.frame_color[channel]=value;return true;}
        for(unsigned i=0;i<2;++i)if(row.name_plate.color_members[i].identity==identity && state_->objects->Get(row.name_plate.color_members[i].handle)==identity){row.name_plate.colors[i][channel]=value;return true;}
    }
    return false;
}
}

namespace fates::presentation::native {
std::optional<TalkWindowPalette> NativeTalkWindow::ObservePalette() const {return state_->Live()?state_->palette:std::nullopt;}
TalkWindowStatus NativeTalkWindow::StartOpeningState(TalkWindowHandle handle,runtime::native::ProcessAccess& access) {
    if(auto status=state_->Mutable(&access);status!=S::Ready)return status;
    auto* row=state_->Get(handle);if(!row)return S::InvalidHandle;
    row->active=1;row->first_message=std::uint8_t{1};row->current=0;row->line=0;row->x_offset=0;row->new_string=1;
    for(auto& text:row->strings)Clear(text); // Unlike ResetStrings, next_icon is untouched.
    row->font_size=std::uint8_t{19};row->text_layout=std::array<std::uint8_t,3>{0,0,19};row->strings[0].position={};return S::Ready;
}
TalkWindowStatus NativeTalkWindow::RestoreSpeakerActive(TalkWindowHandle handle,std::optional<std::uint8_t> value,ProcessAccess* access) {
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    auto* row=state_->Get(handle);if(!row)return S::InvalidHandle;
    row->speaker_active=value;return S::Ready;
}
TalkWindowStatus NativeTalkWindow::ClearFaceState(TalkWindowHandle handle,bool clear_open,ProcessAccess& access) {
    if(const auto status=state_->Mutable(&access);status!=S::Ready)return status;
    auto* row=state_->Get(handle);if(!row)return S::InvalidHandle;
    row->name_effect_enabled=std::uint8_t{0};row->name_override=0;
    if(clear_open)row->active=0;
    return S::Ready;
}
TalkWindowStatus NativeTalkWindow::ClearNextIcon(TalkWindowHandle handle,ProcessAccess& access) {
    if(const auto status=state_->Mutable(&access);status!=S::Ready)return status;
    auto* row=state_->Get(handle);if(!row)return S::InvalidHandle;
    row->next_icon=0;return S::Ready;
}
TalkWindowStatus NativeTalkWindow::HideNextIcon(TalkWindowHandle handle,runtime::native::ProcessAccess& access) {
    if(auto status=state_->Mutable(&access);status!=S::Ready)return status;
    auto* row=state_->Get(handle);if(!row)return S::InvalidHandle;row->next_icon=255;return S::Ready;
}
TalkWindowStatus NativeTalkWindow::WriteShoutIndex(TalkWindowHandle handle,std::uint8_t value,ProcessAccess& access) {
    if(const auto status=state_->Mutable(&access);status!=S::Ready)return status;
    auto* row=state_->Get(handle);if(!row)return S::InvalidHandle;row->next_icon=value;return S::Ready;
}
TalkWindowStatus NativeTalkWindow::CloseEffectState(TalkWindowHandle handle,bool skip,runtime::native::ProcessAccess& access) {
    if(auto status=state_->Mutable(&access);status!=S::Ready)return status;
    auto* row=state_->Get(handle);if(!row)return S::InvalidHandle;
    if(skip)ClearStrings(*row);
    row->active=0;return S::Ready;
}
TalkWindowStatus NativeTalkWindow::RestoreEffectFlags(TalkWindowHandle handle,std::uint8_t active,std::optional<std::uint8_t> first,std::uint8_t wait,runtime::native::ProcessAccess* access) {
    if(auto status=state_->Mutable(access);status!=S::Ready)return status;
    auto* row=state_->Get(handle);if(!row)return S::InvalidHandle;
    row->active=active;row->first_message=first;row->face_pending=wait;return S::Ready;
}
TalkWindowStatus NativeTalkWindow::SetStringAlpha(TalkWindowView view,std::uint8_t alpha,runtime::native::ProcessAccess& access) {
    if(auto status=state_->Mutable(&access);status!=S::Ready)return status;
    auto* row=state_->Get(view.window);if(!row || view.slot>=8)return S::InvalidHandle;
    row->strings[view.slot].color[3]=alpha;return S::Ready;
}
}

namespace fates::presentation::native {
TalkWindowStatus NativeTalkWindow::RestoreNameEffectEnabled(TalkWindowHandle handle,std::optional<std::uint8_t> value,runtime::native::ProcessAccess* access) {
    if(auto status=state_->Mutable(access);status!=TalkWindowStatus::Ready)return status;
    auto* row=state_->Get(handle);if(!row)return TalkWindowStatus::InvalidHandle;
    row->name_effect_enabled=value;return TalkWindowStatus::Ready;
}
}

namespace fates::presentation::native {
TalkWindowStatus NativeTalkWindow::RestoreFaceIdentifier(TalkWindowHandle handle,std::span<const std::uint8_t> words,std::bitset<32> known,runtime::native::ProcessAccess* access) {
    if(const auto status=state_->Mutable(access);status!=TalkWindowStatus::Ready)return status;
    auto* row=state_->Get(handle);if(!row)return TalkWindowStatus::InvalidHandle;
    if(words.size()!=32)return TalkWindowStatus::InvalidSource;
    row->name_effect_enabled=known.test(0)?std::optional<std::uint8_t>{words[0]}:std::nullopt;
    for(std::size_t i=0;i<31;++i){row->face_identifier_tail[i]=words[i+1];row->face_identifier_known[i]=known.test(i+1);}
    return TalkWindowStatus::Ready;
}
TalkWindowStatus NativeTalkWindow::RestoreStringsVisible(TalkWindowHandle handle,std::uint8_t value,runtime::native::ProcessAccess* access) {
    if(const auto status=state_->Mutable(access);status!=TalkWindowStatus::Ready)return status;
    auto* row=state_->Get(handle);if(!row)return TalkWindowStatus::InvalidHandle;
    row->strings_visible=value;return TalkWindowStatus::Ready;
}
}

namespace fates::presentation::native {
TalkWindowStatus NativeTalkWindow::WriteFontColor(TalkWindowHandle handle,TalkColorBytes color,runtime::native::ProcessAccess& access){
    if(const auto status=state_->Mutable(&access);status!=TalkWindowStatus::Ready)return status;
    auto* row=state_->Get(handle);if(!row)return TalkWindowStatus::InvalidHandle;row->font_color=color;return TalkWindowStatus::Ready;
}
TalkWindowStatus NativeTalkWindow::AddMeasuredXOffset(TalkWindowHandle handle,std::uint32_t width,runtime::native::ProcessAccess& access){
    if(const auto status=state_->Mutable(&access);status!=TalkWindowStatus::Ready)return status;
    auto* row=state_->Get(handle);if(!row)return TalkWindowStatus::InvalidHandle;row->x_offset+=width;return TalkWindowStatus::Ready;
}
}
