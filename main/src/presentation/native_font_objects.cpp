#include "fates/presentation/native_font_objects.hpp"
#include <algorithm>
#include <limits>
#include <map>

namespace fates::presentation::native {
using namespace runtime::native;
using namespace io::native;
using S=FontObjectStatus;
namespace {
constexpr std::uint32_t Setup=0x1785a4,Cleanup=0x178820,Deleting=0x178824,Destructor=0x178834;
constexpr std::array Targets{Setup,Cleanup,Deleting,Destructor};
ProcessCall Service(ProcessHandle process,std::uint32_t target,FileObjectHandle object) {
    ProcessCall call;call.process=std::move(process);call.target=target;call.kind=ProcessCallKind::Service;
    call.argument_count=1;call.arguments[0]=object?object->serial:0u;return call;
}
std::uint16_t U16(std::span<const std::uint8_t> bytes,std::size_t at){return std::uint16_t(std::uint16_t(bytes[at])|(std::uint16_t(bytes[at+1])<<8));}
std::uint32_t U32(std::span<const std::uint8_t> bytes,std::size_t at){return std::uint32_t(U16(bytes,at))|(std::uint32_t(U16(bytes,at+2))<<16);}
std::uint8_t Format(std::uint16_t value){constexpr std::array<std::uint8_t,6> formats{10,7,9,5,11,8};return value<formats.size()?formats[value]:std::uint8_t{12};}
bool MethodsMatch(const FileObjectMethods& a,const FileObjectMethods& b){return a.deleting_destructor==b.deleting_destructor && a.setup==b.setup && a.cleanup==b.cleanup && a.get_allocator==b.get_allocator && a.get_align==b.get_align && a.is_delay==b.is_delay;}
}
struct NativeFontObjects::State {
    struct Sheet final {
        std::uint64_t identity{},revision{};std::uint32_t data{};
        std::shared_ptr<const NativeFileImage> image;NativeTextureDescription description;
    };
    struct Object {FileObjectHandle identity;std::vector<std::shared_ptr<const Sheet>> sheets;
        std::uint32_t capacity{};std::uint64_t next_identity{},setups{};
        std::vector<std::uint64_t> destroyed;bool retired{};S status{S::Ready};};
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<NativeFileController> files;std::shared_ptr<NativeFileEntry> entry;
    std::shared_ptr<NativeFontMetrics> metrics;
    std::map<std::uint32_t,std::shared_ptr<Object>> objects;
    bool Live() const {const auto owner=scheduler.lock();return owner && owner->root(2);}
    S Mutable(ProcessAccess* access) const {const auto owner=scheduler.lock();if(!owner || !owner->root(2))return S::Retired;
        if(access)return access->BelongsTo(*owner)?S::Ready:S::MismatchedDomain;
        return owner->busy()?S::Busy:S::Ready;}
    std::shared_ptr<Object> Get(FileObjectHandle handle) const {
        if(!handle)return {};
        const auto found=objects.find(handle->serial);
        return found!=objects.end() && found->second->identity==handle?found->second:nullptr;
    }
    std::shared_ptr<const Sheet> Get(const FontSheetView& view) const {
        const auto object=Get(view.object);
        if(!Live() || !object || object->retired || view.index>=object->sheets.size())return {};
        const auto sheet=object->sheets[view.index];
        return sheet && sheet->identity==view.sheet_identity && files->OwnsLiveData(view.object,sheet->data,sheet->revision)?sheet:nullptr;
    }
};
struct NativeFontObjects::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;std::shared_ptr<State::Object> object;ProcessCall call;
    std::shared_ptr<const NativeFileImage> image;std::uint64_t revision{};
    std::uint32_t data{},count{},stride{},offset{},index{};std::uint16_t width{},height{};
    std::uint8_t format{};unsigned stage{};
    ProcessCallbackStep Block(S status){object->status=status;return ProcessCallbackStep::Blocked();}
    ProcessCallbackStep Invoke(std::uint32_t target){return ProcessCallbackStep::Call(Service(call.process,target,object->identity));}
    ProcessCallbackStep Step(ProcessAccess& access) override {
        const auto scheduler=state->scheduler.lock();if(!scheduler || !access.BelongsTo(*scheduler))return Block(S::MismatchedDomain);
        if(call.target==Deleting || call.target==Destructor) {
            if(stage==0) {
                // ITexture target External never frees this borrowed pixel data.
                // Destruction is forward, unlike TalkWindow array destruction.
                for(const auto& sheet:object->sheets)object->destroyed.push_back(sheet->identity);
                object->sheets.clear();object->capacity=0;object->retired=true;
                if(state->metrics->RetireMetrics(object->identity,access)!=FontMetricStatus::Ready)return Block(S::InvalidObject);
                stage=1;return Invoke(0x17855c);
            }
            if(stage==1 && call.target==Deleting){stage=2;return Invoke(0x2fdce4);}
            object->status=S::Ready;return ProcessCallbackStep::Return(object->identity->serial);
        }
        const auto row=state->files->Observe(object->identity);
        if(!row || row->life!=FileObjectLife::Live || object->retired)return Block(S::InvalidObject);
        // Retail FontObject::Cleanup is literally BX LR. It neither clears the
        // vector nor calls ISFont destruction nor sets a FileObject ready bit.
        if(call.target==Cleanup){object->status=S::Ready;return ProcessCallbackStep::Return();}
        if(stage==0) {
            const auto result=state->entry->ReadResult(object->identity);
            if(!result || !result->image || !row->fields.data)return Block(S::DataUnavailable);
            image=result->image;revision=row->data_revision;data=row->fields.data;
            if(state->metrics->SetupMetrics(object->identity,&access)!=FontMetricStatus::Ready)return Block(S::InvalidData);
            stage=1;
        }
        if(!state->files->OwnsLiveData(object->identity,data,revision))return Block(S::StaleView);
        if(stage==1) {
            if(state->metrics->SetAbsentCode(object->identity,char16_t{0xff1f},&access)!=FontMetricStatus::Ready)return Block(S::InvalidData);
            const auto bytes=image->bytes();if(bytes.size()<40)return Block(S::InvalidData);
            width=U16(bytes,16);height=U16(bytes,18);stride=U32(bytes,20);format=Format(U16(bytes,24));
            count=U16(bytes,26);offset=U32(bytes,36);stage=2;
        }
        while(index<count) {
            std::uint32_t length{};
            if(!ComputeNativeTextureImageSize(width,height,format,1,length))return Block(S::InvalidData);
            // Safe host boundary: reject guest pointer/extent overflow, rather
            // than claim the original has these checks or substitute empty data.
            const std::uint64_t position=std::uint64_t(offset)+std::uint64_t(stride)*index;
            if(position>std::numeric_limits<std::uint32_t>::max() || position>image->bytes().size() || length>image->bytes().size()-position)return Block(S::InvalidData);
            if(object->next_identity==std::numeric_limits<std::uint64_t>::max() || object->sheets.size()>=65536u)return Block(S::IdentityExhausted);
            auto sheet=std::make_shared<State::Sheet>();sheet->identity=++object->next_identity;
            sheet->image=image;sheet->data=data;sheet->revision=revision;
            auto& texture=sheet->description;texture.width=width;texture.height=height;texture.format=format;
            texture.mip_count=1;texture.target=0;texture.filter=0;texture.wrap=0;texture.mode=0;
            texture.source_offset=static_cast<std::uint32_t>(position);texture.packed_size=length;
            texture.matrix={1.0f/static_cast<float>(width),0,0,0,0,-1.0f/static_cast<float>(height),0,1,0,0,1,0};
            if(object->sheets.size()==object->capacity) {
                const auto size=static_cast<std::uint32_t>(object->sheets.size());
                object->capacity=std::max(size+32u,size+(size>>1u)+(size>>3u));
            }
            object->sheets.push_back(std::move(sheet));++index;
        }
        ++object->setups;object->status=S::Ready;return ProcessCallbackStep::Return();
    }
};
NativeFontObjects::NativeFontObjects(std::shared_ptr<State> state):state_(std::move(state)){}
NativeFontObjects::~NativeFontObjects()=default;
S NativeFontObjects::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<NativeFileController> files,std::shared_ptr<NativeFileBase> bases,std::shared_ptr<NativeFileEntry> entry,
    std::shared_ptr<NativeFontMetrics> metrics,std::shared_ptr<NativeFontObjects>& out) {
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !files || !files->UsesScheduler(*scheduler) ||
        !bases || !entry || !entry->UsesOwners(*files,*bases) || !metrics || !metrics->UsesScheduler(*scheduler) || !metrics->UsesOwners(*files,*entry))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->files=std::move(files);state->entry=std::move(entry);state->metrics=std::move(metrics);
    auto owner=std::shared_ptr<NativeFontObjects>(new NativeFontObjects(state));if(!registry->Register(Targets,owner))return S::DuplicateBinding;
    out=std::move(owner);return S::Ready;
}
FileObjectMethods NativeFontObjects::Methods() noexcept{return {Deleting,Setup,Cleanup,0x50a51c,0x50a534,0x50a52c};}
S NativeFontObjects::Construct(FileObjectHandle& out,ProcessAccess* access) {
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    FileObjectHandle object;if(state_->files->ConstructObject(Methods(),object,access)!=FileControllerStatus::Ready)return S::InvalidObject;
    auto owned=std::make_shared<State::Object>();owned->identity=object;state_->objects.emplace(object->serial,std::move(owned));out=std::move(object);return S::Ready;
}
S NativeFontObjects::RestoreEmptyConstructed(FileObjectHandle object,ProcessAccess* access) {
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    const auto row=state_->files->Observe(object);if(!row || row->life!=FileObjectLife::Live || state_->Get(object))return S::InvalidObject;
    auto expected=Methods();const FileObjectMethods actual{row->fields.deleting_destructor,row->fields.setup,row->fields.cleanup,row->fields.get_allocator,row->fields.get_align,row->fields.is_delay};
    // Older metric-only fixtures explicitly carried the nondeleting entry.
    // This is admitted only on this explicitly named carried-state operation.
    if(actual.deleting_destructor==Destructor)expected.deleting_destructor=Destructor;
    if(!MethodsMatch(actual,expected))return S::InvalidObject;
    auto owned=std::make_shared<State::Object>();owned->identity=object;state_->objects.emplace(object->serial,std::move(owned));return S::Ready;
}
std::optional<FontObjectObservation> NativeFontObjects::Observe(FileObjectHandle handle) const {
    const auto object=state_->Get(handle);if(!state_->Live() || !object)return {};
    return FontObjectObservation{handle,static_cast<std::uint32_t>(object->sheets.size()),object->capacity,
        static_cast<std::uint32_t>(object->destroyed.size()),object->setups,object->retired,object->status,object->destroyed};
}
std::optional<FontSheetView> NativeFontObjects::GetSheet(FileObjectHandle handle,std::uint32_t index) const {
    const auto object=state_->Get(handle);if(!object || index>=object->sheets.size())return {};
    FontSheetView view{handle,index,object->sheets[index]->identity};return state_->Get(view)?std::optional{view}:std::nullopt;
}
S NativeFontObjects::Describe(const FontSheetView& view,NativeTextureDescription& out) const {
    const auto sheet=state_->Get(view);if(!sheet)return S::StaleView;out=sheet->description;return S::Ready;
}
S NativeFontObjects::PackedBytes(const FontSheetView& view,std::vector<std::uint8_t>& out) const {
    const auto sheet=state_->Get(view);if(!sheet)return S::StaleView;
    const auto bytes=sheet->image->bytes().subspan(sheet->description.source_offset,sheet->description.packed_size);
    out.assign(bytes.begin(),bytes.end());return S::Ready;
}
ProcessCall NativeFontObjects::SetupCall(ProcessHandle process,FileObjectHandle object){return Service(std::move(process),Setup,std::move(object));}
ProcessCall NativeFontObjects::CleanupCall(ProcessHandle process,FileObjectHandle object){return Service(std::move(process),Cleanup,std::move(object));}
ProcessCall NativeFontObjects::DestructorCall(ProcessHandle process,FileObjectHandle object){return Service(std::move(process),Destructor,std::move(object));}
bool NativeFontObjects::UsesScheduler(const NativeProcessScheduler& scheduler) const noexcept{return state_->scheduler.lock().get()==&scheduler;}
bool NativeFontObjects::UsesOwners(const NativeFileController& files,const NativeFileEntry& entry,const NativeFontMetrics& metrics) const noexcept{return state_->files.get()==&files && state_->entry.get()==&entry && state_->metrics.get()==&metrics;}
std::unique_ptr<ProcessContinuation> NativeFontObjects::Begin(const ProcessCall& call) {
    if(!state_->Live() || !call.process || call.kind!=ProcessCallKind::Service || call.has_self || call.this_adjustment || call.argument_count!=1 ||
        std::find(Targets.begin(),Targets.end(),call.target)==Targets.end())return {};
    const auto object=state_->Get(state_->files->ResolveObject(call.arguments[0]));if(!object || object->retired)return {};
    const auto lower=state_->files->Observe(object->identity);
    if(!lower || ((call.target==Deleting || call.target==Destructor) && lower->life!=FileObjectLife::Destroying))return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->object=object;next->call=call;return next;
}
}
