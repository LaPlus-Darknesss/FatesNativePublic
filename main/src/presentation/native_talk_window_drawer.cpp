#include "fates/presentation/native_talk_window_drawer.hpp"
#include <algorithm>
#include <limits>
#include <map>

namespace fates::presentation::native {
using namespace runtime::native;
using namespace io::native;
using S=TalkDrawerStatus;
namespace {
constexpr std::uint32_t Init=0x1f02c0,Tick=0x1f0524,Finalize=0x1f10e0,Destroy=0x1f120c;
constexpr std::array Targets{Init,Tick,Finalize,Destroy};
constexpr std::array<std::string_view,3> Scenes{"SceneTalkW","SceneMiniTalkW","SceneShopBuyD"};
ProcessCall Service(ProcessHandle process,std::uint32_t target,std::initializer_list<std::uint32_t> args={}) {
    ProcessCall call;call.process=std::move(process);call.kind=ProcessCallKind::Service;call.target=target;
    call.argument_count=static_cast<std::uint8_t>(args.size());std::copy(args.begin(),args.end(),call.arguments.begin());return call;
}
}
struct NativeTalkWindowDrawer::State {
    struct Instance {
        std::uint32_t identity{};bool live{true};
        std::array<FileBaseHandle,2> files;
        std::array<CoordinateSceneHandle,3> scenes;
    };
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<NativeFileController> files;std::shared_ptr<NativeFileBase> bases;
    std::shared_ptr<NativeTexFiles> tex;std::shared_ptr<NativeCoordinateResources> coordinates;
    std::optional<std::uint32_t> shared_texture_word;
    std::uint32_t references{},serial{};std::shared_ptr<Instance> current;
    std::map<std::uint32_t,std::shared_ptr<Instance>> instances;
    bool Live() const {auto owner=scheduler.lock();return owner && owner->root(2);}
    S Mutable(ProcessAccess* access) const {
        auto owner=scheduler.lock();if(!owner || !owner->root(2))return S::Retired;
        if(access)return access->BelongsTo(*owner)?S::Ready:S::MismatchedDomain;
        return owner->busy()?S::Busy:S::Ready;
    }
};
struct NativeTalkWindowDrawer::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;ProcessCall call;std::shared_ptr<State::Instance> instance;
    FilePathHandle path;std::size_t index{};unsigned stage{};
    ProcessCallbackStep Invoke(std::uint32_t target,std::initializer_list<std::uint32_t> args={}) {
        return ProcessCallbackStep::Call(Service(call.process,target,args));
    }
    ProcessCallbackStep Initialize(ProcessAccess& access) {
        if(stage==0) {
            if(state->references) {++state->references;return ProcessCallbackStep::Return();}
            if(state->serial==std::numeric_limits<std::uint32_t>::max())return ProcessCallbackStep::Blocked();
            instance=std::make_shared<State::Instance>();instance->identity=++state->serial;
            state->instances.emplace(instance->identity,instance);stage=1;
        }
        if(stage==1) {
            // The original vector constructor creates BOTH embedded bases before
            // either read; these references remain local until full publication.
            for(auto& file:instance->files)if(!file && state->tex->Construct(file,&access)!=TexFileStatus::Ready)
                return ProcessCallbackStep::Blocked();
            stage=2;
        }
        for(;;) {
            if(stage==2) {
                if(index==2) {index=0;stage=6;}
                else {
                    std::optional<std::string_view> name="ui/TalkWindow2.bch.lz";
                    if(index==1) {
                        if(!state->shared_texture_word)return ProcessCallbackStep::Blocked();
                        name=NativeTextureCoordinateScene::TalkWindowPath(*state->shared_texture_word&0xffu);
                        if(!name)return ProcessCallbackStep::Blocked();
                    }
                    if(state->bases->RegisterPath(*name,path,&access)!=FileBaseStatus::Ready)return ProcessCallbackStep::Blocked();
                    stage=3;
                }
            }
            if(stage==3) {
                const auto child=state->tex->ReadCall(call.process,instance->files[index],path,2,
                    call.arguments[0]?TexFileRead::Asynchronous:TexFileRead::Immediate);
                if(!child)return ProcessCallbackStep::Blocked();
                stage=4;return ProcessCallbackStep::Call(*child);
            }
            if(stage==4) {
                stage=5;
                if(call.arguments[0])return Invoke(0x4e67f4,{instance->files[index]->serial,2});
            }
            if(stage==5) {++index;path.reset();stage=2;continue;}
            if(stage==6) {
                for(;index<Scenes.size();++index) {
                    if(state->coordinates->CreateScene(Scenes[index],instance->scenes[index],&access)!=CoordinateResourceStatus::Ready)
                        return ProcessCallbackStep::Blocked();
                }
                state->current=instance;++state->references;return ProcessCallbackStep::Return();
            }
        }
    }
    ProcessCallbackStep TickFiles() {
        // Reload current on each stage, just like the original global dereference.
        instance=state->current;if(!instance || !instance->live)return ProcessCallbackStep::Blocked();
        if(index==2)return ProcessCallbackStep::Return();
        if(stage==0) {stage=1;return Invoke(0x545df4,{instance->files[index]->serial});}
        return ProcessCallbackStep::Blocked();
    }
    ProcessCallbackStep DestroyInstance() {
        if(!instance || !instance->live)return ProcessCallbackStep::Blocked();
        for(;;) {
            if(stage==0) {
                if(index==2) {index=0;stage=1;continue;}
                return Invoke(0x11cf84,{instance->files[index++]->serial});
            }
            if(stage==1) {
                if(index==3) {index=2;stage=3;continue;}
                if(!instance->scenes[index]) {++index;continue;}
                const auto child=state->coordinates->DestroyCall(call.process,instance->scenes[index]);
                if(!child)return ProcessCallbackStep::Blocked();
                stage=2;return ProcessCallbackStep::Call(*child);
            }
            if(stage==2) {instance->scenes[index++].reset();stage=1;continue;}
            if(stage==3) {
                // __aeabi_vec_dtor runs embedded TexFile destructors in reverse.
                if(index==0) {stage=5;return Invoke(0x2fdce4,{instance->identity});}
                const auto child=state->tex->DestroyCall(call.process,instance->files[index-1],false);
                if(!child)return ProcessCallbackStep::Blocked();
                stage=4;return ProcessCallbackStep::Call(*child);
            }
            if(stage==4) {--index;stage=3;continue;}
            instance->live=false;state->instances.erase(instance->identity);return ProcessCallbackStep::Return();
        }
    }
    ProcessCallbackStep Step(ProcessAccess& access) override {
        auto owner=state->scheduler.lock();if(!owner || !access.BelongsTo(*owner))return ProcessCallbackStep::Blocked();
        if(call.target==Init)return Initialize(access);
        if(call.target==Destroy)return DestroyInstance();
        if(call.target==Finalize) {
            if(stage==0) {
                // Preserve original unsigned wrap, including unmatched Finalize.
                --state->references;if(state->references)return ProcessCallbackStep::Return();
                instance=state->current;stage=1;
                if(instance)return Invoke(Destroy,{instance->identity});
            }
            state->current.reset();return ProcessCallbackStep::Return();
        }
        if(stage==0)return TickFiles();
        if(stage==1) {
            if(access.call_result()) {++index;stage=0;return TickFiles();}
            instance=state->current;if(!instance || !instance->live)return ProcessCallbackStep::Blocked();
            stage=2;return Invoke(0x545dd0,{instance->files[index]->serial});
        }
        if(stage==2) {
            if(access.call_result()) {++index;stage=0;return TickFiles();}
            instance=state->current;if(!instance || !instance->live)return ProcessCallbackStep::Blocked();
            const auto child=state->bases->FinishCall(call.process,instance->files[index]);
            if(!child)return ProcessCallbackStep::Blocked();
            stage=3;return ProcessCallbackStep::Call(*child);
        }
        ++index;stage=0;return TickFiles();
    }
};
NativeTalkWindowDrawer::NativeTalkWindowDrawer(std::shared_ptr<State> state):state_(std::move(state)){}
NativeTalkWindowDrawer::~NativeTalkWindowDrawer()=default;
S NativeTalkWindowDrawer::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<NativeFileController> files,std::shared_ptr<NativeFileBase> bases,std::shared_ptr<NativeTexFiles> tex,
    std::shared_ptr<NativeCoordinateResources> coordinates,std::shared_ptr<NativeTalkWindowDrawer>& output) {
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !files || !files->UsesScheduler(*scheduler) || !bases ||
        !bases->UsesController(*files) || !tex || !tex->UsesOwners(*files,*bases) || !coordinates || !coordinates->UsesOwners(*files,*bases,*tex))
        return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->files=std::move(files);state->bases=std::move(bases);
    state->tex=std::move(tex);state->coordinates=std::move(coordinates);
    auto module=std::shared_ptr<NativeTalkWindowDrawer>(new NativeTalkWindowDrawer(state));
    if(!registry->Register(Targets,module))return S::DuplicateBinding;
    output=std::move(module);return S::Ready;
}
S NativeTalkWindowDrawer::PublishSharedTextureWord(std::optional<std::uint32_t> word,ProcessAccess* access) {
    if(auto result=state_->Mutable(access);result!=S::Ready)return result;
    state_->shared_texture_word=word;return S::Ready;
}
ProcessCall NativeTalkWindowDrawer::InitializeCall(ProcessHandle process,bool async){return Service(std::move(process),Init,{async?1u:0u});}
ProcessCall NativeTalkWindowDrawer::TickLoadAsyncCall(ProcessHandle process){return Service(std::move(process),Tick);}
ProcessCall NativeTalkWindowDrawer::FinalizeCall(ProcessHandle process){return Service(std::move(process),Finalize);}
std::optional<TalkDrawerObservation> NativeTalkWindowDrawer::Observe() const {
    if(!state_->Live())return {};
    TalkDrawerObservation result;result.references=state_->references;result.instance_present=bool(state_->current);
    if(state_->current) {result.instance_live=state_->current->live;result.files=state_->current->files;result.scenes=state_->current->scenes;}
    return result;
}
std::optional<bool> NativeTalkWindowDrawer::FilesReady() const {
    if(!state_->Live() || !state_->current || !state_->current->live)return {};
    for(const auto& file:state_->current->files) {
        const auto base=state_->bases->Observe(file);if(!base)return {};
        if(!base->object)return false;
        const auto row=state_->files->Observe(base->object);
        if(!row || row->life!=FileObjectLife::Live)return {};
        if(!(row->fields.flags&0x04000000u))return false;
    }
    return true;
}
bool NativeTalkWindowDrawer::UsesScheduler(const NativeProcessScheduler& scheduler) const noexcept{return state_->scheduler.lock().get()==&scheduler;}
bool NativeTalkWindowDrawer::UsesOwners(const NativeFileController& files,const NativeFileBase& bases,
    const NativeTexFiles& tex,const NativeCoordinateResources& coordinates) const noexcept {
    return state_->files.get()==&files && state_->bases.get()==&bases && state_->tex.get()==&tex && state_->coordinates.get()==&coordinates;
}
std::unique_ptr<ProcessContinuation> NativeTalkWindowDrawer::Begin(const ProcessCall& call) {
    if(!state_->Live() || call.kind!=ProcessCallKind::Service || call.has_self || call.this_adjustment || !call.process ||
        std::find(Targets.begin(),Targets.end(),call.target)==Targets.end())return {};
    if(call.argument_count!=((call.target==Init || call.target==Destroy)?1u:0u))return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->call=call;
    if(call.target==Destroy) {
        const auto it=state_->instances.find(call.arguments[0]);if(it==state_->instances.end())return {};next->instance=it->second;
    }
    return next;
}
}
