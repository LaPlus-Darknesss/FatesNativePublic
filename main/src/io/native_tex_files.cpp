#include "fates/io/native_tex_files.hpp"
#include <algorithm>
#include <map>

namespace fates::io::native {
using namespace runtime::native;
namespace {
constexpr std::uint32_t Read=0x112c30,ReadAsync=0x4e0c60,TryRead=0x4e0be4,
    Destroy=0x4e0da8,Delete=0x4e0d80,Count=0x5453d8;
constexpr std::array Targets{Read,ReadAsync,TryRead,Destroy,Delete,Count};
ProcessCall Service(ProcessHandle process,std::uint32_t target,std::initializer_list<std::uint32_t> arguments) {
    ProcessCall call;call.process=std::move(process);call.kind=ProcessCallKind::Service;call.target=target;
    call.argument_count=static_cast<std::uint8_t>(arguments.size());
    std::copy(arguments.begin(),arguments.end(),call.arguments.begin());return call;
}
}
struct NativeTexFiles::State {
    std::weak_ptr<NativeProcessScheduler> scheduler;std::shared_ptr<NativeFileController> files;
    std::shared_ptr<NativeFileBase> bases;std::shared_ptr<NativeFileEntry> entry;
    std::shared_ptr<NativeTextureObjects> textures;std::map<std::uint32_t,FileBaseHandle> handles;
    bool Live() const {auto owner=scheduler.lock();return owner && owner->root(2);}
    bool Owns(FileBaseHandle handle) const {
        if(!handle || !bases->Observe(handle))return false;
        const auto found=handles.find(handle->serial);return found!=handles.end() && found->second==handle;
    }
};
struct NativeTexFiles::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;ProcessCall call;FileBaseHandle base;FilePathHandle path;
    FileObjectHandle object;FileEntryHandle request;unsigned stage{};std::uint32_t file_type{},read_type{};
    ProcessCallbackStep Step(ProcessAccess& access) override {
        auto scheduler=state->scheduler.lock();
        if(!scheduler || !access.BelongsTo(*scheduler) || !state->Owns(base))return ProcessCallbackStep::Blocked();
        if(call.target==Count) {
            auto attachment=state->bases->Observe(base);if(!attachment)return ProcessCallbackStep::Blocked();
            if(!attachment->object)return ProcessCallbackStep::Return(0);
            const auto observed=state->textures->Observe(attachment->object);
            return observed?ProcessCallbackStep::Return(observed->texture_count):ProcessCallbackStep::Blocked();
        }
        if(call.target==Destroy || call.target==Delete) {
            if(stage==0) {
                auto child=state->bases->FreeCall(call.process,base);if(!child)return ProcessCallbackStep::Blocked();
                stage=1;return ProcessCallbackStep::Call(*child);
            }
            if(stage==1 && call.target==Delete) {
                stage=2;return ProcessCallbackStep::Call(Service(call.process,0x2fdce4,{base->serial}));
            }
            if(state->bases->RetireEmpty(base,&access)!=FileBaseStatus::Ready)return ProcessCallbackStep::Blocked();
            state->handles.erase(base->serial);return ProcessCallbackStep::Return(base->serial);
        }
        if(stage==0) {
            const auto child=state->bases->OpenCall(call.process,base,path,read_type);
            if(!child)return ProcessCallbackStep::Blocked();
            stage=1;return ProcessCallbackStep::Call(*child);
        }
        if(stage==1) {file_type=access.call_result();stage=file_type?4u:2u;}
        if(stage==2) {
            if(state->textures->Construct(object,&access)!=TextureObjectStatus::Ready)return ProcessCallbackStep::Blocked();
            stage=3;
        }
        if(stage==3) {
            if(!request && state->entry->Prepare(base,object,path,call.arguments[2],read_type,request,&access)!=FileEntryStatus::Ready)
                return ProcessCallbackStep::Blocked();
            const auto child=state->entry->EntryCall(call.process,request);if(!child)return ProcessCallbackStep::Blocked();
            stage=4;return ProcessCallbackStep::Call(*child);
        }
        if(stage==4) {
            const auto child=state->bases->CloseCall(call.process,base,file_type,read_type);
            if(!child)return ProcessCallbackStep::Blocked();
            stage=5;return ProcessCallbackStep::Call(*child);
        }
        return ProcessCallbackStep::Return(access.call_result());
    }
};
NativeTexFiles::NativeTexFiles(std::shared_ptr<State> state):state_(std::move(state)){}
NativeTexFiles::~NativeTexFiles()=default;
TexFileStatus NativeTexFiles::Create(std::shared_ptr<NativeProcessScheduler> scheduler,
    std::shared_ptr<ProcessCallbackRegistry> registry,std::shared_ptr<NativeFileController> files,
    std::shared_ptr<NativeFileBase> bases,std::shared_ptr<NativeFileEntry> entry,std::shared_ptr<NativeTextureObjects> textures,
    std::shared_ptr<NativeTexFiles>& output) {
    using S=TexFileStatus;
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !files || !files->UsesScheduler(*scheduler) ||
        !bases || !entry || !entry->UsesOwners(*files,*bases) || !textures || !textures->UsesOwners(*files,*entry))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->files=std::move(files);
    state->bases=std::move(bases);state->entry=std::move(entry);state->textures=std::move(textures);
    auto module=std::shared_ptr<NativeTexFiles>(new NativeTexFiles(state));
    if(!registry->Register(Targets,module))return S::DuplicateBinding;
    output=std::move(module);return S::Ready;
}
TexFileStatus NativeTexFiles::Construct(FileBaseHandle& output,ProcessAccess* access) {
    using S=TexFileStatus;auto scheduler=state_->scheduler.lock();
    if(!scheduler || !scheduler->root(2))return S::Retired;
    if(access && !access->BelongsTo(*scheduler))return S::MismatchedDomain;
    if(!access && scheduler->busy())return S::Busy;
    FileBaseHandle base;
    if(state_->bases->RestoreAttachment({},base,access)!=FileBaseStatus::Ready)return S::InvalidHandle;
    state_->handles.emplace(base->serial,base);output=std::move(base);return S::Ready;
}
std::optional<ProcessCall> NativeTexFiles::ReadCall(ProcessHandle process,FileBaseHandle base,FilePathHandle path,
    std::uint32_t flags,TexFileRead mode) const {
    if(!state_->Live() || !state_->Owns(base) || !path || state_->bases->ResolvePath(path->serial)!=path)return {};
    std::uint32_t target{};
    switch(mode){case TexFileRead::Immediate:target=Read;break;case TexFileRead::Asynchronous:target=ReadAsync;break;
    case TexFileRead::TryImmediate:target=TryRead;break;default:return {};}
    return Service(std::move(process),target,{base->serial,path->serial,flags});
}
std::optional<ProcessCall> NativeTexFiles::DestroyCall(ProcessHandle process,FileBaseHandle base,bool deleting) const {
    if(!state_->Live() || !state_->Owns(base))return {};
    return Service(std::move(process),deleting?Delete:Destroy,{base->serial});
}
std::optional<std::uint32_t> NativeTexFiles::TextureCount(FileBaseHandle base) const {
    if(!state_->Live() || !state_->Owns(base))return {};
    const auto attachment=state_->bases->Observe(base);if(!attachment)return {};
    if(!attachment->object)return 0;
    const auto observed=state_->textures->Observe(attachment->object);
    return observed?std::optional(observed->texture_count):std::nullopt;
}
TextureLookupResult NativeTexFiles::GetTexture(FileBaseHandle base,std::int32_t index) const {
    if(index==65535)return {TextureObjectStatus::Dummy,{}};
    if(!state_->Live() || !state_->Owns(base))return {};
    const auto attachment=state_->bases->Observe(base);if(!attachment)return {};
    return attachment->object?state_->textures->GetTexture(attachment->object,index):TextureLookupResult{TextureObjectStatus::Dummy,{}};
}
TextureLookupResult NativeTexFiles::GetTexture(FileBaseHandle base,std::optional<std::string_view> name) const {
    if(!state_->Live() || !state_->Owns(base))return {};
    const auto attachment=state_->bases->Observe(base);if(!attachment)return {};
    return attachment->object?state_->textures->GetTexture(attachment->object,name):TextureLookupResult{TextureObjectStatus::Dummy,{}};
}
bool NativeTexFiles::UsesTextureObjects(const NativeTextureObjects& objects) const noexcept {return state_->textures.get()==&objects;}
bool NativeTexFiles::UsesOwners(const NativeFileController& files,const NativeFileBase& bases) const noexcept {
    return state_->files.get()==&files && state_->bases.get()==&bases;
}
std::unique_ptr<ProcessContinuation> NativeTexFiles::Begin(const ProcessCall& call) {
    if(!state_->Live() || call.kind!=ProcessCallKind::Service || call.has_self || call.this_adjustment || !call.process ||
        std::find(Targets.begin(),Targets.end(),call.target)==Targets.end())return {};
    const bool reading=call.target==Read || call.target==ReadAsync || call.target==TryRead;
    if(call.argument_count!=(reading?3u:1u))return {};
    const auto base=state_->bases->ResolveBase(call.arguments[0]);if(!state_->Owns(base))return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->call=call;next->base=base;
    if(reading) {
        next->path=state_->bases->ResolvePath(call.arguments[1]);if(!next->path)return {};
        next->read_type=call.target==ReadAsync?1u:call.target==TryRead?2u:0u;
    }
    return next;
}
}
