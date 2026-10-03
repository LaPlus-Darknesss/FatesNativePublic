#include "fates/event/native_event_talk.hpp"
#include "fates/event/native_proc_event.hpp"
#include <limits>
#include <map>

namespace fates::event::native {
using namespace runtime::native;
using S=TalkControlStatus;
namespace {constexpr std::uint32_t Talk=0x3af36c,NoShadow=0x3abc48;}
struct NativeEventTalk::State {
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::weak_ptr<NativeProcEvent> events;bool bound{};
    std::shared_ptr<NativeTalkManagerFactory> factory;
    std::shared_ptr<NativeTalkInitialize> initialize;
    std::shared_ptr<NativeTalkControlContext> context;
    std::map<std::uint32_t,std::shared_ptr<EventTalkObservation>> requests;
    std::uint32_t serial{};
    S Mutable(ProcessAccess* access)const {
        const auto owner=scheduler.lock();if(!owner || !owner->root(2))return S::Retired;
        if(access)return access->BelongsTo(*owner)?S::Ready:S::MismatchedDomain;
        return owner->busy()?S::Busy:S::Ready;
    }
    std::shared_ptr<EventTalkObservation> Get(EventTalkRequest id)const {
        if(!id)return {};
        const auto it=requests.find(id->serial);
        return it!=requests.end() && it->second->identity==id?it->second:nullptr;
    }
};
struct NativeEventTalk::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;std::shared_ptr<EventTalkObservation> request;
    TalkManagerFactoryRequest factory;TalkInitializeRequest initialize;unsigned stage{};
    ~Continuation()override {if(initialize)state->initialize->Forget(initialize);}
    ProcessCallbackStep Block(S status=S::Unavailable){request->status=status;return ProcessCallbackStep::Blocked();}
    ProcessCallbackStep Step(ProcessAccess& access)override {
        if(state->Mutable(&access)!=S::Ready)return Block(S::MismatchedDomain);
        for(;;)switch(stage){
        case 0:{
            const auto events=state->events.lock();if(!events)return Block(S::Retired);
            const auto current=events->Current();if(!current)return Block();
            // The argument is the original current-event value, including known
            // null. The factory's existing-global branch does not inspect it.
            request->parent=*current;
            const auto status=state->factory->Prepare(request->caller,request->parent,factory,&access);
            if(status!=S::Ready)return Block(status);
            const auto call=state->factory->Call(factory);if(!call)return Block();
            stage=1;return ProcessCallbackStep::Call(*call);
        }
        case 1:{
            const auto result=state->factory->Observe(factory);if(!result || !result->completed)return Block();
            request->manager=result->manager;
            if(state->factory->Release(factory,&access)!=S::Ready)return Block();
            factory.reset();stage=2;break;
        }
        case 2:
            if(request->no_shadow && state->context->WriteShadowEnabled(request->manager,std::uint8_t{0},access)!=S::Ready)return Block();
            stage=3;break;
        case 3:{
            const auto status=state->initialize->PrepareIdentifier(request->manager,request->message_identifier,initialize,&access);
            if(status!=S::Ready)return Block(status);
            const auto call=state->initialize->Call(initialize);if(!call)return Block();
            stage=4;return ProcessCallbackStep::Call(*call);
        }
        case 4:{
            const auto result=state->initialize->Observe(initialize);if(!result || !result->completed)return Block();
            if(state->initialize->Release(initialize,&access)!=S::Ready)return Block();
            initialize.reset();stage=5;break;
        }
        default:{
            const auto events=state->events.lock();if(!events)return Block(S::Retired);
            const auto current=events->Current();if(!current || !*current)return Block();
            request->final_event=*current;const auto process=access.Observe(*current);if(!process)return Block(S::InvalidHandle);
            request->request_yield=process->blocking_children!=0;request->completed=true;request->status=S::Ready;
            return ProcessCallbackStep::Return();
        }
        }
    }
};
NativeEventTalk::NativeEventTalk(std::shared_ptr<State> state):state_(std::move(state)){}
NativeEventTalk::~NativeEventTalk()=default;
S NativeEventTalk::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<NativeTalkManagerFactory> factory,std::shared_ptr<NativeTalkInitialize> initialize,
    std::shared_ptr<NativeTalkControlContext> context,std::shared_ptr<NativeEventTalk>& out){
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !factory || !initialize || !context ||
       !factory->UsesScheduler(*scheduler) || !initialize->UsesScheduler(*scheduler) || !context->UsesScheduler(*scheduler) ||
       !factory->UsesOwners(*context,*initialize))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->factory=std::move(factory);
    state->initialize=std::move(initialize);state->context=std::move(context);
    auto next=std::shared_ptr<NativeEventTalk>(new NativeEventTalk(std::move(state)));
    if(!registry->Register(std::array{Talk,NoShadow},next))return S::DuplicateBinding;
    out=std::move(next);return S::Ready;
}
bool NativeEventTalk::CanBindEvent()const noexcept{return !state_->bound;}
S NativeEventTalk::BindEvent(const std::shared_ptr<NativeProcEvent>& events){
    if(auto status=state_->Mutable(nullptr);status!=S::Ready)return status;
    const auto owner=state_->scheduler.lock();if(!events || !events->UsesScheduler(*owner))return S::MismatchedDomain;
    if(state_->bound)return S::DuplicateBinding;
    state_->events=events;state_->bound=true;return S::Ready;
}
S NativeEventTalk::Prepare(ProcessHandle caller,std::string_view name,bool no_shadow,EventTalkRequest& out,ProcessAccess* access){
    if(auto status=state_->Mutable(access);status!=S::Ready)return status;
    const auto owner=state_->scheduler.lock();const auto proc=owner->Observe(caller);
    if(!proc || !proc->linked || (proc->flags&1u))return S::InvalidParent;
    if(state_->events.expired())return S::Unavailable;
    if(name.find('\0')!=std::string_view::npos)return S::InvalidSource;
    if(state_->serial==std::numeric_limits<std::uint32_t>::max())return S::IdentityExhausted;
    auto row=std::make_shared<EventTalkObservation>();row->identity=std::make_shared<const EventTalkIdentity>(EventTalkIdentity{++state_->serial});
    row->caller=std::move(caller);row->message_identifier=std::string(name);row->no_shadow=no_shadow;
    state_->requests.emplace(row->identity->serial,row);out=row->identity;return S::Ready;
}
std::optional<ProcessCall> NativeEventTalk::Call(EventTalkRequest id)const{
    const auto row=state_->Get(id);if(!row || row->started || row->completed)return {};
    ProcessCall call;call.process=row->caller;call.kind=ProcessCallKind::Service;call.target=row->no_shadow?NoShadow:Talk;
    call.arguments[0]=id->serial;call.argument_count=1;return call;
}
std::optional<EventTalkObservation> NativeEventTalk::Observe(EventTalkRequest id)const{
    const auto owner=state_->scheduler.lock();if(!owner || !owner->root(2))return {};
    const auto row=state_->Get(id);return row?std::optional{*row}:std::nullopt;
}
void NativeEventTalk::Forget(EventTalkRequest id)noexcept{if(state_->Get(id))state_->requests.erase(id->serial);}
bool NativeEventTalk::UsesScheduler(const NativeProcessScheduler& owner)const noexcept{return state_->scheduler.lock().get()==&owner;}
bool NativeEventTalk::UsesRuntime(const NativeRuntime& runtime)const noexcept{return &state_->factory->runtime()==&runtime;}
std::unique_ptr<ProcessContinuation> NativeEventTalk::Begin(const ProcessCall& call){
    if(call.kind!=ProcessCallKind::Service || call.has_self || call.this_adjustment || call.argument_count!=1 ||
       (call.target!=Talk && call.target!=NoShadow))return {};
    const auto it=state_->requests.find(call.arguments[0]);if(it==state_->requests.end())return {};
    const auto row=it->second;const auto owner=state_->scheduler.lock();const auto proc=owner?owner->Observe(call.process):std::nullopt;
    if(row->caller!=call.process || row->started || row->completed || (call.target==NoShadow)!=row->no_shadow ||
       !proc || !proc->linked || (proc->flags&1u))return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->request=row;row->started=true;return next;
}
}
