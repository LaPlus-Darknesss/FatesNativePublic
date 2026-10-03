#include "fates/io/native_coordinate_resources.hpp"
#include <algorithm>
#include <limits>
#include <map>

namespace fates::io::native {
using namespace runtime::native;
using S=CoordinateResourceStatus;
namespace {
constexpr std::uint32_t Init=0x205dac,Finalize=0x20610c,StructDelete=0x18de74,
    Load=0x205f30,Free=0x205ed4,Destroy=0x20609c;
constexpr std::array Targets{Init,Finalize,StructDelete,Load,Free,Destroy};
constexpr FileObjectMethods UniqueMethods{0x216e20,0x216db8,0x216de8,0x50a51c,0x50a534,0x50a52c};
ProcessCall Service(ProcessHandle process,std::uint32_t target,std::initializer_list<std::uint32_t> args={}) {
    ProcessCall call;call.process=std::move(process);call.kind=ProcessCallKind::Service;call.target=target;
    call.argument_count=static_cast<std::uint8_t>(args.size());std::copy(args.begin(),args.end(),call.arguments.begin());return call;
}
}
struct NativeCoordinateResources::State {
    struct Scene {CoordinateSceneObservation row;std::shared_ptr<NativeTextureCoordinateScene> data;};
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<NativeFileController> files;std::shared_ptr<NativeFileBase> bases;
    std::shared_ptr<NativeFileEntry> entry;std::shared_ptr<NativeUniqueArchiveObjects> archives;
    std::shared_ptr<NativeTexFiles> tex;std::shared_ptr<NativeLanguagePaths> language;
    std::function<FileSourceStatus(std::string_view)> exists;NativeLanguageState language_state;
    FileBaseHandle current; // Original zero-initialized global, not inferred file absence.
    std::map<std::uint32_t,FileBaseHandle> struct_files;
    std::map<std::uint32_t,std::shared_ptr<Scene>> scenes;std::uint32_t serial{};
    bool Live() const {auto owner=scheduler.lock();return owner && owner->root(2);}
    S Mutable(ProcessAccess* access) const {
        auto owner=scheduler.lock();if(!owner || !owner->root(2))return S::Retired;
        if(access)return access->BelongsTo(*owner)?S::Ready:S::MismatchedDomain;
        return owner->busy()?S::Busy:S::Ready;
    }
    std::shared_ptr<Scene> Get(CoordinateSceneHandle handle) const {
        if(!handle)return {};
        const auto it=scenes.find(handle->serial);return it!=scenes.end() && it->second->row.identity==handle?it->second:nullptr;
    }
    std::optional<FileBaseObservation> Archive() const {
        if(!Live())return {};
        if(!current)return FileBaseObservation{};
        return bases->Observe(current);
    }
};
struct NativeCoordinateResources::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;ProcessCall call;std::shared_ptr<State::Scene> scene;
    FileBaseHandle base,pending_file;FileObjectHandle object;FilePathHandle path;FileEntryHandle request;
    TextureCoordinatePlan plan;std::uint32_t type{};std::size_t slot{};unsigned stage{};
    ProcessCallbackStep Invoke(std::uint32_t target,std::initializer_list<std::uint32_t> args={}) {
        return ProcessCallbackStep::Call(Service(call.process,target,args));
    }
    ProcessCallbackStep Global(ProcessAccess& access) {
        if(call.target==StructDelete) {
            if(stage==0) {stage=1;return Invoke(0x11cf84,{base->serial});}
            if(stage==1) {stage=2;return Invoke(0x2fdce4,{base->serial});}
            if(state->bases->RetireEmpty(base,&access)!=FileBaseStatus::Ready)return ProcessCallbackStep::Blocked();
            state->struct_files.erase(base->serial);return ProcessCallbackStep::Return();
        }
        if(stage==0) {
            if(state->current) {base=state->current;stage=1;return Invoke(StructDelete,{base->serial});}
            stage=1;
        }
        if(stage==1) {
            state->current.reset();
            if(call.target==Finalize)return ProcessCallbackStep::Return();
            if(!state->exists)return ProcessCallbackStep::Blocked();
            const auto found=state->exists("TextureCoordinate.bin");
            if(found==FileSourceStatus::Missing)return ProcessCallbackStep::Return();
            if(found!=FileSourceStatus::Ready)return ProcessCallbackStep::Blocked();
            stage=2;
        }
        if(stage==2) {
            if(!base || !state->bases->Observe(base)) {
                if(state->bases->RestoreAttachment({},base,&access)!=FileBaseStatus::Ready)return ProcessCallbackStep::Blocked();
                state->struct_files.emplace(base->serial,base);
            }
            if(!path && state->bases->RegisterPath("TextureCoordinate.bin",path,&access)!=FileBaseStatus::Ready)
                return ProcessCallbackStep::Blocked();
            stage=3;return Invoke(0x112ca8,{base->serial,path->serial,0});
        }
        if(stage==3) {type=access.call_result();stage=type?6u:4u;}
        if(stage==4) {
            if(state->files->ConstructObject(UniqueMethods,object,&access)!=FileControllerStatus::Ready)return ProcessCallbackStep::Blocked();
            stage=5;
        }
        if(stage==5) {
            if(!request && state->entry->Prepare(base,object,path,0,0,request,&access)!=FileEntryStatus::Ready)
                return ProcessCallbackStep::Blocked();
            const auto child=state->entry->EntryCall(call.process,request);if(!child)return ProcessCallbackStep::Blocked();
            stage=6;return ProcessCallbackStep::Call(*child);
        }
        if(stage==6) {
            const auto attachment=state->bases->Observe(base);if(!attachment)return ProcessCallbackStep::Blocked();
            if(attachment->object) {
                const auto lower=state->files->Observe(attachment->object);if(!lower)return ProcessCallbackStep::Blocked();
                if(lower->fields.setup==UniqueMethods.setup && lower->fields.data) {
                    const auto read=state->entry->ReadResult(attachment->object);
                    if(read && (!read->image || state->archives->Associate(attachment->object,read->image,&access)!=UniqueArchiveStatus::Ready))
                        return ProcessCallbackStep::Blocked();
                }
            }
            stage=7;return Invoke(0x112d28,{base->serial,type,0});
        }
        state->current=base;return ProcessCallbackStep::Return();
    }
    ProcessCallbackStep SceneLoad(ProcessAccess& access) {
        if(stage==0) {
            // Original positive count returns even if the global file was freed.
            if(scene->row.loaded_count)return ProcessCallbackStep::Return();
            const auto current=state->Archive();
            if(!current || !current->identity || !current->object)return ProcessCallbackStep::Blocked();
            const auto files=state->archives->Find(current->object,std::string_view("File"));
            if(files.status!=UniqueArchiveStatus::Ready && files.status!=UniqueArchiveStatus::Missing)return ProcessCallbackStep::Blocked();
            // Immutable data preflight enforces host extents/16 slots before any
            // unsafe retail out-of-bounds branch. It has no language/I/O effects.
            if(scene->data->DescribeRetainedHeaderRequests(plan)!=UniqueArchiveStatus::Ready)return ProcessCallbackStep::Blocked();
            stage=1;
        }
        for(;;) {
            if(stage==1) {
                if(slot==plan.requests.size())return ProcessCallbackStep::Return();
                const auto& item=plan.requests[slot];
                if(state->archives->Validate(item.path_identity)!=UniqueArchiveStatus::Ready)return ProcessCallbackStep::Blocked();
                auto& destination=scene->row.slots[slot];destination.path=item.path_identity;destination.path_known=true;
                // Count/path precede allocation/localization/read. The original
                // file word is untouched until the TexFile constructor returns.
                ++scene->row.loaded_count;stage=2;
            }
            if(stage==2) {
                const auto selected=state->language->FolderForTexture(plan.requests[slot].archive_path,state->language_state);
                if(selected.status!=LanguagePathStatus::Ready)return ProcessCallbackStep::Blocked();
                if(state->bases->RegisterPath(selected.path,path,&access)!=FileBaseStatus::Ready)return ProcessCallbackStep::Blocked();
                stage=3;
            }
            if(stage==3) {
                if(state->tex->Construct(pending_file,&access)!=TexFileStatus::Ready)return ProcessCallbackStep::Blocked();
                stage=4;
            }
            if(stage==4) {
                const auto child=state->tex->ReadCall(call.process,pending_file,path,2,TexFileRead::Immediate);
                if(!child)return ProcessCallbackStep::Blocked();
                stage=5;return ProcessCallbackStep::Call(*child);
            }
            auto& destination=scene->row.slots[slot];destination.file=pending_file;destination.file_known=true;
            pending_file.reset();path.reset();++slot;stage=1;
        }
    }
    ProcessCallbackStep SceneFree(ProcessAccess&) {
        for(;;) {
            if(stage==0) {
                if(slot==scene->row.loaded_count) {
                    scene->row.loaded_count=0;
                    if(call.target==Free)return ProcessCallbackStep::Return();
                    stage=2;return Invoke(0x2fdce4,{scene->row.identity->serial});
                }
                auto& destination=scene->row.slots[slot];
                if(!destination.file_known)return ProcessCallbackStep::Blocked();
                if(destination.file) {
                    const auto child=state->tex->DestroyCall(call.process,destination.file,true);
                    if(!child)return ProcessCallbackStep::Blocked();
                    stage=1;return ProcessCallbackStep::Call(*child);
                }
                stage=1;
            }
            if(stage==1) {
                auto& destination=scene->row.slots[slot];destination.file.reset();destination.file_known=true;
                destination.path={};destination.path_known=true;++slot;stage=0;continue;
            }
            state->scenes.erase(scene->row.identity->serial);return ProcessCallbackStep::Return();
        }
    }
    ProcessCallbackStep Step(ProcessAccess& access) override {
        auto owner=state->scheduler.lock();if(!owner || !access.BelongsTo(*owner))return ProcessCallbackStep::Blocked();
        if(call.target==Init || call.target==Finalize || call.target==StructDelete)return Global(access);
        if(!scene || state->Get(scene->row.identity)!=scene)return ProcessCallbackStep::Blocked();
        return call.target==Load?SceneLoad(access):SceneFree(access);
    }
};
NativeCoordinateResources::NativeCoordinateResources(std::shared_ptr<State> state):state_(std::move(state)){}
NativeCoordinateResources::~NativeCoordinateResources()=default;
S NativeCoordinateResources::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<NativeFileController> files,std::shared_ptr<NativeFileBase> bases,std::shared_ptr<NativeFileEntry> entry,
    std::shared_ptr<NativeUniqueArchiveObjects> archives,std::shared_ptr<NativeTexFiles> tex,std::shared_ptr<NativeLanguagePaths> language,
    std::function<FileSourceStatus(std::string_view)> exists,std::shared_ptr<NativeCoordinateResources>& output) {
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !files || !files->UsesScheduler(*scheduler) || !bases || !entry ||
        !entry->UsesOwners(*files,*bases) || !archives || !archives->UsesController(*files) || !tex || !tex->UsesOwners(*files,*bases) || !language)
        return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->files=std::move(files);state->bases=std::move(bases);
    state->entry=std::move(entry);state->archives=std::move(archives);state->tex=std::move(tex);state->language=std::move(language);state->exists=std::move(exists);
    auto module=std::shared_ptr<NativeCoordinateResources>(new NativeCoordinateResources(state));
    if(!registry->Register(Targets,module))return S::DuplicateBinding;
    output=std::move(module);return S::Ready;
}
S NativeCoordinateResources::PublishLanguage(NativeLanguageState value,ProcessAccess* access) {
    if(auto result=state_->Mutable(access);result!=S::Ready)return result;
    state_->language_state=std::move(value);return S::Ready;
}
ProcessCall NativeCoordinateResources::InitializeCall(ProcessHandle process){return Service(std::move(process),Init);}
ProcessCall NativeCoordinateResources::FinalizeCall(ProcessHandle process){return Service(std::move(process),Finalize);}
S NativeCoordinateResources::CreateScene(std::optional<std::string_view> name,CoordinateSceneHandle& output,ProcessAccess* access) {
    if(auto status=state_->Mutable(access);status!=S::Ready)return status;
    const auto current=state_->Archive();if(!current || !current->identity || !current->object)return S::Unavailable;
    std::shared_ptr<NativeTextureCoordinateScene> data;
    if(NativeTextureCoordinateScene::Create(state_->archives,current->object,name,data)!=UniqueArchiveStatus::Ready)return S::InvalidRecord;
    if(state_->serial==std::numeric_limits<std::uint32_t>::max())return S::IdentityExhausted;
    auto scene=std::make_shared<State::Scene>();scene->row.identity=std::make_shared<CoordinateSceneIdentity>(++state_->serial);
    scene->row.header=data->header();scene->data=std::move(data);state_->scenes.emplace(scene->row.identity->serial,scene);
    output=scene->row.identity;return S::Ready;
}
std::optional<ProcessCall> NativeCoordinateResources::LoadCall(ProcessHandle process,CoordinateSceneHandle handle) const {
    if(!state_->Live() || !state_->Get(handle))return {};
    return Service(std::move(process),Load,{handle->serial});
}
std::optional<ProcessCall> NativeCoordinateResources::FreeCall(ProcessHandle process,CoordinateSceneHandle handle) const {
    if(!state_->Live() || !state_->Get(handle))return {};
    return Service(std::move(process),Free,{handle->serial});
}
std::optional<ProcessCall> NativeCoordinateResources::DestroyCall(ProcessHandle process,CoordinateSceneHandle handle) const {
    if(!state_->Live() || !state_->Get(handle))return {};
    return Service(std::move(process),Destroy,{handle->serial});
}
std::optional<CoordinateSceneObservation> NativeCoordinateResources::Observe(CoordinateSceneHandle handle) const {
    const auto scene=state_->Get(handle);return state_->Live() && scene?std::optional(scene->row):std::nullopt;
}
std::optional<FileBaseObservation> NativeCoordinateResources::ObserveArchive() const{return state_->Archive();}
UniqueArchiveRecordResult NativeCoordinateResources::Find(std::optional<std::string_view> name) const {
    const auto current=state_->Archive();if(!current)return {UniqueArchiveStatus::Retired,{}};
    if(!current->identity)return {UniqueArchiveStatus::Missing,{}};
    if(!current->object)return {UniqueArchiveStatus::InvalidObject,{}};
    return state_->archives->Find(current->object,name);
}
CoordinateTextureResult NativeCoordinateResources::GetTexture(CoordinateSceneHandle handle,const UniqueArchiveRecord& file) const {
    if(!state_->Live())return {S::Retired,false,{}};
    const auto scene=state_->Get(handle);if(!scene)return {S::InvalidScene,false,{}};
    if(!file)return {S::Ready,false,{}};
    const auto path=state_->archives->Pointer(file,0);
    if(path.status!=UniqueArchiveStatus::Ready)return {S::InvalidRecord,false,{}};
    if(!path.value)return {S::Ready,false,{}};
    const auto name=state_->archives->Pointer(file,4);
    if(name.status!=UniqueArchiveStatus::Ready)return {S::InvalidRecord,false,{}};
    if(!name.value)return {S::Ready,false,{}};
    for(std::uint32_t index=0;index<scene->row.loaded_count;++index) {
        const auto& item=scene->row.slots[index];if(!item.path_known)return {S::Unavailable,false,{}};
        if(!item.path.SamePointer(path.value))continue;
        if(!item.file_known || !item.file)return {S::Unavailable,false,{}};
        const auto text=state_->archives->ReadString(name.value);
        if(text.status!=UniqueArchiveStatus::Ready || !text.value)return {S::InvalidRecord,false,{}};
        return {S::Ready,true,state_->tex->GetTexture(item.file,std::string_view(*text.value))};
    }
    return {S::Ready,false,{}};
}
bool NativeCoordinateResources::UsesOwners(const NativeFileController& files,const NativeFileBase& bases,const NativeTexFiles& tex) const noexcept {
    return state_->files.get()==&files && state_->bases.get()==&bases && state_->tex.get()==&tex;
}
std::unique_ptr<ProcessContinuation> NativeCoordinateResources::Begin(const ProcessCall& call) {
    if(!state_->Live() || call.kind!=ProcessCallKind::Service || call.has_self || call.this_adjustment || !call.process ||
        std::find(Targets.begin(),Targets.end(),call.target)==Targets.end())return {};
    const bool global=call.target==Init || call.target==Finalize;
    if(call.argument_count!=(global?0u:1u))return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->call=call;
    if(call.target==StructDelete) {
        const auto it=state_->struct_files.find(call.arguments[0]);if(it==state_->struct_files.end())return {};
        next->base=it->second;
    } else if(!global) {
        const auto it=state_->scenes.find(call.arguments[0]);if(it==state_->scenes.end())return {};next->scene=it->second;
    }
    return next;
}
}

namespace fates::io::native {
UniqueArchiveWordResult NativeCoordinateResources::ReadU16(const UniqueArchiveRecord& record,std::size_t field) const {
    if(!state_->Live())return {UniqueArchiveStatus::Retired,0};
    return state_->archives->ReadU16(record,field);
}
UniqueArchiveRecordResult NativeCoordinateResources::Pointer(const UniqueArchiveRecord& record,std::size_t field) const {
    if(!state_->Live())return {UniqueArchiveStatus::Retired,{}};
    return state_->archives->Pointer(record,field);
}
}

namespace fates::io::native {bool NativeCoordinateResources::UsesScheduler(const runtime::native::NativeProcessScheduler& scheduler) const noexcept{return state_->scheduler.lock().get()==&scheduler;}}
