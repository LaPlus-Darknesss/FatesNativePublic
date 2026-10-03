#include "fates/runtime/native_talk_manager_storage.hpp"
#include "fates/event/native_event_consistency.hpp"
#include <map>

namespace fates::runtime::native {
using namespace presentation::native;
using S=TalkControlStatus;
namespace {
constexpr std::uint32_t Construct=0x1e62e0,Destroy=0x1e64e0,IsExist=0x1e5fd8,CreateFaces=0x196478;
constexpr std::array Targets{Construct,Destroy,IsExist};
ProcessCall Service(ProcessHandle manager,std::uint32_t target) {
    ProcessCall call;call.process=std::move(manager);call.kind=ProcessCallKind::Service;call.target=target;return call;
}
}
struct NativeTalkManagerStorage::State {
    struct Row {
        TalkManagerStorageObservation value;
        std::shared_ptr<NativeTalkText> text;
    };
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<NativeTalkControlContext> managers;
    std::shared_ptr<NativeTalkWindow> windows;
    std::shared_ptr<NativeTalkBackground> backgrounds;
    std::shared_ptr<NativeTalkInitialize> initialize;
    std::shared_ptr<NativeMapBinder> binder;
    std::shared_ptr<NativeGameSkip> skip;
    std::shared_ptr<NativeFadeSystem> fade;
    const NativeArchiveIdentifiers* identifiers{};NativeMessageLookup* messages{};
    NativeUnitNames* names{};NativeTalkPlayer* player{};NativeTalkTokens* tokens{};
    std::map<std::uint64_t,std::shared_ptr<Row>> rows;
    std::optional<ProcessHandle> current;
    ~State(){for(auto& [serial,row]:rows){(void)serial;if(row->text)row->text->Retire();}}
    bool Live()const{const auto s=scheduler.lock();return s && s->root(2);}
    S Mutable(ProcessAccess* access)const{
        const auto s=scheduler.lock();if(!s || !s->root(2))return S::Retired;
        if(access)return access->BelongsTo(*s)?S::Ready:S::MismatchedDomain;
        return s->busy()?S::Busy:S::Ready;
    }
    std::shared_ptr<Row> Get(ProcessHandle handle)const{
        if(!handle || !Live())return {};
        const auto it=rows.find(handle->serial);return it!=rows.end() && it->second->value.manager==handle?it->second:nullptr;
    }
};
struct NativeTalkManagerStorage::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;std::shared_ptr<State::Row> row;ProcessCall call;
    unsigned stage{},index{};
    ProcessCallbackStep Block(){return ProcessCallbackStep::Blocked();}
    ProcessCallbackStep ConstructBody(ProcessAccess& access) {
        for(;;){
            if(stage==0){
                row->text=std::make_shared<NativeTalkText>(*state->identifiers,*state->messages,*state->names,*state->player,*state->tokens);
                row->value.text=row->text;stage=1;
            }
            if(stage==1){
                while(index<3){
                    if(state->windows->Construct(row->value.windows[index],&access)!=TalkWindowStatus::Ready)return Block();
                    ++index;row->value.windows_constructed=static_cast<std::uint8_t>(index);
                }
                stage=2;
            }
            if(stage==2){
                if(state->backgrounds->Construct(row->value.background,&access)!=TalkBackgroundStatus::Ready)return Block();
                stage=3;
            }
            if(stage==3){
                if(state->managers->ConstructMembers(call.process,row->value.windows,row->value.background,access)!=S::Ready)return Block();
                row->value.context_committed=true;row->value.disposer_live=true;stage=4;
            }
            if(stage==4){
                if(state->initialize->AttachExpander(call.process,row->text,&access)!=S::Ready)return Block();
                // Retail publishes BEFORE creating FaceManager and observing skip/binder.
                // Construction of another allocation may overwrite this pointer;
                // that does not destroy the earlier allocation.
                state->current=call.process;stage=5;
                return ProcessCallbackStep::Call(Service(call.process,CreateFaces));
            }
            if(stage==5){
                const auto current=state->skip->Current();if(!current)return Block();
                if(*current){const auto skip=state->skip->Observe(*current);if(!skip)return Block();
                    if(state->managers->WriteSavedSkipFlags(call.process,skip->flags&~4u,access)!=S::Ready)return Block();}
                // Known absence preserves the zero written by the earlier prefix.
                stage=6;
            }
            if(stage==6){
                const auto current=state->binder->Current();if(!current)return Block();
                if(*current){
                    const auto reread=state->binder->Current();if(!reread || !*reread)return Block();
                    bool transitioned{};if(state->binder->Bind(access,*reread,transitioned)!=MapBinderStatus::Ready)return Block();
                }
                stage=7;
            }
            if(stage==7){
                const auto current=state->skip->Current();if(!current)return Block();
                bool skipping=false;if(*current){const auto skip=state->skip->Observe(*current);if(!skip)return Block();skipping=skip->state!=0;}
                FadeColor color;
                if(skipping){
                    const auto reread=state->skip->Current();if(!reread || !*reread)return Block();
                    const auto skip=state->skip->Observe(*reread);if(!skip || !skip->saved_goals[0])return Block();color=*skip->saved_goals[0];
                }else{const auto fade=state->fade->ObserveChannel(0);if(!fade)return Block();color=fade->color;}
                TalkLifecycleUpdate update;update.ending_fade=static_cast<std::uint8_t>(color[3]!=255?0u:color[0]==0?1u:2u);
                if(state->managers->WriteLifecycleState(call.process,update,access)!=S::Ready)return Block();
                row->value.constructor_complete=true;return ProcessCallbackStep::Return();
            }
        }
    }
    ProcessCallbackStep DestroyBody(ProcessAccess& access) {
        const auto owner=state->scheduler.lock();if(!owner)return Block();
        const auto service_context=owner->root(2); // Same live cleanup context as NativeProcEvent.
        for(;;){
            if(stage==0){
                row->value.destroying=true;
                const auto current=state->binder->Current();if(!current)return Block();
                if(*current){const auto reread=state->binder->Current();if(!reread || !*reread)return Block();bool transitioned{};
                    if(state->binder->Unbind(access,*reread,transitioned)!=MapBinderStatus::Ready)return Block();}
                stage=1;
            }
            if(stage==1){
                const auto current=state->skip->Current();if(!current)return Block();
                if(*current){const auto manager=state->managers->Observe(call.process);if(!manager || !manager->saved_skip_flags)return Block();
                    if(state->skip->MergeFlags(access,*current,*manager->saved_skip_flags)!=GameSkipStatus::Ready)return Block();}
                stage=2;
            }
            if(stage==2){
                const auto manager=state->managers->Observe(call.process);if(!manager || !manager->auxiliary)return Block();
                stage=3;if(*manager->auxiliary)return ProcessCallbackStep::Delete(*manager->auxiliary);
            }
            if(stage==3){
                if(state->managers->ClearOwnedAuxiliary(call.process,access)!=S::Ready)return Block();
                // Unconditional original null store; do not add current==self.
                state->current=ProcessHandle{};row->value.disposer_live=false;stage=4;
            }
            if(stage==4){
                const auto nested=state->backgrounds->DestroyCall(service_context,row->value.background);if(!nested)return Block();
                stage=5;return ProcessCallbackStep::Call(*nested);
            }
            if(stage==5){index=3;stage=6;}
            if(stage==6){
                if(index==0){stage=8;continue;}
                const auto nested=state->windows->DestroyCall(service_context,row->value.windows[index-1]);if(!nested)return Block();
                stage=7;return ProcessCallbackStep::Call(*nested);
            }
            if(stage==7){--index;++row->value.windows_destroyed;stage=6;continue;}
            if(stage==8){
                if(state->initialize->DetachExpander(call.process,&access)!=S::Ready)return Block();
                row->text->Retire();
                if(state->managers->RetireOwnedMembers(call.process,access)!=S::Ready)return Block();
                state->rows.erase(call.process->serial);
                // Existing scheduler owns base destructor/allocation removal after
                // this derived destructor returns. Do not free its node here.
                return ProcessCallbackStep::Return();
            }
        }
    }
    ProcessCallbackStep Step(ProcessAccess& access)override {
        if(state->Mutable(&access)!=S::Ready)return Block();
        if(call.target==IsExist){if(!state->current)return Block();return ProcessCallbackStep::Return(*state->current?1u:0u);}
        const auto process=access.Observe(call.process);if(!process)return Block();
        if(call.target==Construct){if((!process->linked && !process->constructing) || (process->flags&1u))return Block();return ConstructBody(access);}
        if(process->linked || !(process->flags&1u))return Block();
        return DestroyBody(access);
    }
};
NativeTalkManagerStorage::NativeTalkManagerStorage(std::shared_ptr<State> state):state_(std::move(state)){}
NativeTalkManagerStorage::~NativeTalkManagerStorage()=default;
S NativeTalkManagerStorage::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<NativeTalkControlContext> managers,std::shared_ptr<NativeTalkWindow> windows,std::shared_ptr<NativeTalkBackground> backgrounds,
    std::shared_ptr<NativeTalkInitialize> initialize,std::shared_ptr<NativeMapBinder> binder,std::shared_ptr<NativeGameSkip> skip,
    std::shared_ptr<NativeFadeSystem> fade,const NativeArchiveIdentifiers& identifiers,NativeMessageLookup& messages,
    NativeUnitNames& names,NativeTalkPlayer& player,NativeTalkTokens& tokens,std::shared_ptr<NativeTalkManagerStorage>& out) {
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !managers || !windows || !backgrounds || !initialize || !binder || !skip || !fade ||
        !managers->UsesScheduler(*scheduler) || !managers->UsesOwners(*windows,*backgrounds) || !initialize->UsesScheduler(*scheduler) ||
        !initialize->UsesOwners(*managers,*windows,tokens) || !binder->UsesScheduler(*scheduler) || !skip->UsesScheduler(*scheduler) ||
        !skip->UsesFadeSystem(*fade))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->managers=std::move(managers);state->windows=std::move(windows);
    state->backgrounds=std::move(backgrounds);state->initialize=std::move(initialize);state->binder=std::move(binder);state->skip=std::move(skip);state->fade=std::move(fade);
    state->identifiers=&identifiers;state->messages=&messages;state->names=&names;state->player=&player;state->tokens=&tokens;
    auto next=std::shared_ptr<NativeTalkManagerStorage>(new NativeTalkManagerStorage(state));if(!registry->Register(Targets,next))return S::DuplicateBinding;
    out=std::move(next);return S::Ready;
}
ProcessCall NativeTalkManagerStorage::ConstructCall(ProcessHandle manager){return Service(std::move(manager),Construct);}
ProcessCall NativeTalkManagerStorage::IsExistCall(ProcessHandle context){return Service(std::move(context),IsExist);}
std::optional<TalkManagerStorageObservation> NativeTalkManagerStorage::Observe(ProcessHandle manager)const {
    const auto row=state_->Get(manager);return row?std::optional{row->value}:std::nullopt;
}
std::optional<ProcessHandle> NativeTalkManagerStorage::Current()const{return state_->Live()?state_->current:std::optional<ProcessHandle>{};}
S NativeTalkManagerStorage::PublishAbsent(ProcessAccess* access){if(auto status=state_->Mutable(access);status!=S::Ready)return status;state_->current=ProcessHandle{};return S::Ready;}
bool NativeTalkManagerStorage::UsesScheduler(const NativeProcessScheduler& scheduler)const noexcept{return state_->scheduler.lock().get()==&scheduler;}
std::unique_ptr<ProcessContinuation> NativeTalkManagerStorage::Begin(const ProcessCall& call) {
    const auto owner=state_->scheduler.lock();if(!owner || !state_->Live() || call.this_adjustment || call.argument_count)return {};
    const auto process=owner->Observe(call.process);if(!process)return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->call=call;
    if(call.target==Construct){
        const bool detached=process->constructing && !process->linked && !process->program;
        const bool attached=process->linked && !process->constructing && process->program==event::native::TalkManagerProcessProgram();
        if(call.kind!=ProcessCallKind::Service || call.has_self || (!detached && !attached) || (process->flags&1u) ||
            !owner->HasType(call.process,NativeTalkInitialize::ManagerType()) ||
            state_->Get(call.process) || state_->managers->Observe(call.process))return {};
        next->row=std::make_shared<State::Row>();next->row->value.manager=call.process;state_->rows.emplace(call.process->serial,next->row);
    }else if(call.target==Destroy){
        next->row=state_->Get(call.process);
        if(call.kind!=ProcessCallKind::Destroy || !call.has_self || process->linked || !(process->flags&1u) ||
            !next->row || !next->row->value.constructor_complete)return {};
    }else if(call.target==IsExist){if(call.kind!=ProcessCallKind::Service || call.has_self)return {};}
    else return {};
    return next;
}
S NativeTalkManagerStorage::PublishConstructed(ProcessHandle manager,ProcessAccess& access) {
    if(auto status=state_->Mutable(&access);status!=S::Ready)return status;
    const auto row=state_->Get(manager);const auto process=access.Observe(manager);
    if(!row || !row->value.constructor_complete || row->value.destroying || !process ||
        !process->linked || process->constructing || (process->flags&1u))return S::InvalidHandle;
    state_->current=std::move(manager);return S::Ready;
}
bool NativeTalkManagerStorage::UsesContext(const NativeTalkControlContext& context)const noexcept {
    return state_->managers.get()==&context;
}

const NativeRuntime& NativeTalkManagerStorage::runtime()const noexcept{return state_->names->runtime();}
bool NativeTalkManagerStorage::UsesInitializer(const NativeTalkInitialize& owner)const noexcept{return state_->initialize.get()==&owner;}

}
