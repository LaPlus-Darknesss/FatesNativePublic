#include "fates/runtime/native_talk_manager_factory.hpp"
#include "fates/event/native_event_consistency.hpp"
#include <limits>
#include <map>

namespace fates::runtime::native {
using S=TalkControlStatus;
namespace {constexpr std::uint32_t CreateBind=0x1e56a0;}
struct NativeTalkManagerFactory::State {
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<NativeTalkManagerStorage> storage;
    std::shared_ptr<NativeTalkControlContext> managers;
    std::shared_ptr<NativeTalkLifecycle> lifecycle;
    std::map<std::uint32_t,std::shared_ptr<TalkManagerFactoryObservation>> requests;
    std::uint32_t serial{};
    S Mutable(ProcessAccess* access)const {
        const auto owner=scheduler.lock();if(!owner || !owner->root(2))return S::Retired;
        if(access)return access->BelongsTo(*owner)?S::Ready:S::MismatchedDomain;
        return owner->busy()?S::Busy:S::Ready;
    }
    std::shared_ptr<TalkManagerFactoryObservation> Get(TalkManagerFactoryRequest id)const {
        const auto owner=scheduler.lock();if(!id || !owner || !owner->root(2))return {};
        const auto it=requests.find(id->serial);
        return it!=requests.end() && it->second->identity==id?it->second:nullptr;
    }
};
struct NativeTalkManagerFactory::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;
    std::shared_ptr<TalkManagerFactoryObservation> request;
    unsigned stage{};
    ProcessHandle auxiliary_parent;
    ProcessCallbackStep Block(S why=S::Unavailable) {
        request->status=why;return ProcessCallbackStep::Blocked();
    }
    ProcessCallbackStep Done(ProcessHandle handle) {
        request->manager=std::move(handle);request->completed=true;request->status=S::Ready;
        // The actual pointer is the owned ProcessHandle in the result, not a
        // truncated host pointer forced into a guest-width scalar return value.
        return ProcessCallbackStep::Return();
    }
    ProcessCallbackStep Step(ProcessAccess& access)override {
        if(state->Mutable(&access)!=S::Ready)return Block(S::Retired);
        for(;;) {
            switch(stage) {
            case 0: {
                const auto current=state->storage->Current();if(!current)return Block();
                if(*current) {
                    if(!access.Observe(*current))return Block(S::InvalidHandle);
                    request->reused=true;return Done(*current);
                }
                stage=1;break;
            }
            case 1:
                if(access.ConstructUnattached(NativeTalkInitialize::ManagerType(),request->manager)!=ProcessStatus::Ready)return Block();
                request->allocated=true;stage=2;
                return ProcessCallbackStep::Call(NativeTalkManagerStorage::ConstructCall(request->manager));
            case 2: {
                const auto storage=state->storage->Observe(request->manager);
                if(!storage || !storage->constructor_complete)return Block();
                if(access.AttachConstructed(request->manager,request->parent,event::native::TalkManagerProcessProgram(),
                    "ProcTalkManager",false)!=ProcessStatus::Ready)return Block(S::InvalidParent);
                request->attached=true;stage=3;break;
            }
            case 3:
                if(state->storage->PublishConstructed(request->manager,access)!=S::Ready)return Block();
                stage=4;break;
            case 4: {
                const auto manager=state->managers->Observe(request->manager);
                if(!manager || !manager->auxiliary)return Block();
                if(*manager->auxiliary){request->auxiliary=*manager->auxiliary;stage=7;break;}
                const auto process=access.Observe(request->manager);if(!process || !process->parent)return Block(S::InvalidParent);
                auxiliary_parent=process->parent;stage=5;break;
            }
            case 5:
                if(state->lifecycle->BindAuxiliary(access,auxiliary_parent,request->auxiliary)!=S::Ready)return Block();
                request->auxiliary_created=true;stage=6;break;
            case 6: {
                TalkLifecycleUpdate update;update.auxiliary=request->auxiliary;
                if(state->managers->WriteLifecycleState(request->manager,update,access)!=S::Ready)return Block();
                stage=7;break;
            }
            default: {
                const auto current=state->storage->Current();
                if(!current || !*current || !access.Observe(*current))return Block(S::InvalidHandle);
                return Done(*current);
            }
            }
        }
    }
};
NativeTalkManagerFactory::NativeTalkManagerFactory(std::shared_ptr<State> state):state_(std::move(state)){}
NativeTalkManagerFactory::~NativeTalkManagerFactory()=default;
S NativeTalkManagerFactory::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<NativeTalkManagerStorage> storage,std::shared_ptr<NativeTalkControlContext> managers,
    std::shared_ptr<NativeTalkLifecycle> lifecycle,std::shared_ptr<NativeTalkManagerFactory>& out) {
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !storage || !managers || !lifecycle ||
        !storage->UsesScheduler(*scheduler) || !storage->UsesContext(*managers) || !managers->UsesScheduler(*scheduler) ||
        !lifecycle->UsesScheduler(*scheduler) || !lifecycle->UsesContext(*managers))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->storage=std::move(storage);
    state->managers=std::move(managers);state->lifecycle=std::move(lifecycle);
    auto result=std::shared_ptr<NativeTalkManagerFactory>(new NativeTalkManagerFactory(std::move(state)));
    if(!registry->Register(std::array{CreateBind},result))return S::DuplicateBinding;
    out=std::move(result);return S::Ready;
}
S NativeTalkManagerFactory::Prepare(ProcessHandle caller,ProcessHandle parent,TalkManagerFactoryRequest& out,ProcessAccess* access) {
    if(auto status=state_->Mutable(access);status!=S::Ready)return status;
    const auto owner=state_->scheduler.lock();const auto current=owner->Observe(caller);
    if(!current || (current->flags&1u) || (!current->linked && !(access && current->constructing)))return S::InvalidParent;
    if(state_->serial==std::numeric_limits<std::uint32_t>::max())return S::IdentityExhausted;
    auto request=std::make_shared<TalkManagerFactoryObservation>();
    request->identity=std::make_shared<const TalkManagerFactoryIdentity>(TalkManagerFactoryIdentity{++state_->serial});
    request->caller=std::move(caller);request->parent=std::move(parent);out=request->identity;
    state_->requests.emplace(out->serial,std::move(request));return S::Ready;
}
std::optional<ProcessCall> NativeTalkManagerFactory::Call(TalkManagerFactoryRequest id)const {
    const auto row=state_->Get(id);if(!row || row->completed || row->started)return {};
    ProcessCall call;call.process=row->caller;call.kind=ProcessCallKind::Service;call.target=CreateBind;
    call.arguments[0]=id->serial;call.argument_count=1;return call;
}
std::optional<TalkManagerFactoryObservation> NativeTalkManagerFactory::Observe(TalkManagerFactoryRequest id)const {
    const auto row=state_->Get(id);return row?std::optional{*row}:std::nullopt;
}
S NativeTalkManagerFactory::Release(TalkManagerFactoryRequest id,ProcessAccess* access) {
    if(auto status=state_->Mutable(access);status!=S::Ready)return status;
    const auto row=state_->Get(id);if(!row)return S::InvalidHandle;
    if(row->started && !row->completed)return S::Busy;
    state_->requests.erase(id->serial);return S::Ready;
}
bool NativeTalkManagerFactory::UsesScheduler(const NativeProcessScheduler& scheduler)const noexcept {
    return state_->scheduler.lock().get()==&scheduler;
}
std::unique_ptr<ProcessContinuation> NativeTalkManagerFactory::Begin(const ProcessCall& call) {
    if(call.kind!=ProcessCallKind::Service || call.has_self || call.this_adjustment || call.target!=CreateBind || call.argument_count!=1)return {};
    const auto it=state_->requests.find(call.arguments[0]);
    if(it==state_->requests.end() || it->second->caller!=call.process || it->second->completed || it->second->started)return {};
    const auto owner=state_->scheduler.lock();const auto process=owner?owner->Observe(call.process):std::nullopt;
    if(!process || (!process->linked && !process->constructing) || (process->flags&1u))return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->request=it->second;
    next->request->started=true;return next;
}
const NativeRuntime& NativeTalkManagerFactory::runtime()const noexcept{return state_->storage->runtime();}
bool NativeTalkManagerFactory::UsesOwners(const NativeTalkControlContext& context,const NativeTalkInitialize& initialize)const noexcept {
    return state_->managers.get()==&context && state_->storage->UsesInitializer(initialize);
}

}
