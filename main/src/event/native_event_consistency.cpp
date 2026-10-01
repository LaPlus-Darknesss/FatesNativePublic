#include "fates/event/native_event_consistency.hpp"
#include "fates/runtime/native_runtime.hpp"
#include <array>
namespace fates::event::native {
using namespace runtime::native;
std::shared_ptr<const ProcessProgram> TalkManagerProcessProgram() {
    static const auto p=ProcessProgram::Create({
        {4,0,0,0,0},{11,0,0,0x1e58cc,0},{13,0,0,0x1e4e20,0},
        {4,1,0,0,0},{11,0,0,0x1e62b0,0},{4,2,0,0,0},{13,0,0,8,1},{3,6,0,0,0},
        {4,3,0,0,0},{13,0,0,20,1},{3,2,0,0,0},{4,4,0,0,0},{1,0,0,0,0},{3,2,0,0,0},
        {4,5,0,0,0},{10,0,0,0x167808,0},{11,0,0,0x1e58ec,0},{11,0,0,0x1e5d9c,0},
        {3,7,0,0,0},{4,6,0,0,0},{11,0,0,0x1e5d9c,0},{15,0,0,0x3cf6c4,0},
        {15,1,0,0x3cf6c4,0},{13,0,0,24,1},{4,7,0,0,0},{11,0,0,0x1e5ff0,0},
        {15,0,0,0x3d2940,0},{}});
    return p;
}
bool EventTypeSkipsActorConsistency(std::uint32_t type) noexcept {
    // Comparisons use the full argument word; high values never wrap to a byte.
    return type==4 || type==13 || type==21 || type==23 || type==24 || type==26 ||
        type==27 || (type>=28 && type<=32);
}
struct NativeEventConsistency::State {
    std::shared_ptr<NativeRuntime> runtime;
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::optional<map::native::ActorVisualResult> result;
};
struct NativeEventConsistency::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;std::uint32_t type{};
    unsigned stage{},thread{};ProcessHandle cursor;
    ProcessCallbackStep Step(ProcessAccess& access) override {
        const auto scheduler=state->scheduler.lock();
        if(!scheduler || !access.BelongsTo(*scheduler))return ProcessCallbackStep::Blocked();
        const auto program=TalkManagerProcessProgram();
        if(stage==0) {
            state->result.reset();
            if(!scheduler->FindByProgram(program))stage=3;
            else {cursor=scheduler->root(0);stage=1;}
        }
        // Actual KillByDesc order, including resuming FindNext from the same
        // marked node after its real recursive Dispose callbacks have returned.
        if(stage==2) {cursor=scheduler->FindNext(cursor);stage=1;}
        if(stage==1) {
            while(!cursor && ++thread<3)cursor=scheduler->root(thread);
            if(thread>=3)stage=3;
            else {
                const auto p=access.Observe(cursor);if(!p)return ProcessCallbackStep::Blocked();
                stage=2;
                if(p->program==program && !(p->flags&1))return ProcessCallbackStep::Delete(cursor);
                return ProcessCallbackStep::Continue();
            }
        }
        if(EventTypeSkipsActorConsistency(type))return ProcessCallbackStep::Return();
        const auto result=map::native::ApplyCurrentActorConsistency(*state->runtime);
        state->result=result;
        return result.status==map::native::ActorVisualStatus::Ok?ProcessCallbackStep::Return():ProcessCallbackStep::Blocked();
    }
};
NativeEventConsistency::NativeEventConsistency(std::shared_ptr<State> s):state_(std::move(s)){}
NativeEventConsistency::~NativeEventConsistency()=default;
EventConsistencyStatus NativeEventConsistency::Create(std::shared_ptr<NativeRuntime> runtime,
    std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<NativeEventConsistency>& out) {
    using S=EventConsistencyStatus;
    if(!runtime)return S::NullRuntime;
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()))return S::MismatchedDomain;
    auto s=std::make_shared<State>();s->runtime=std::move(runtime);s->scheduler=scheduler;
    auto next=std::shared_ptr<NativeEventConsistency>(new NativeEventConsistency(s));
    constexpr std::array targets{0x4242c8u};
    if(!registry->Register(targets,next))return S::DuplicateBinding;
    out=std::move(next);return S::Ready;
}
std::unique_ptr<ProcessContinuation> NativeEventConsistency::Begin(const ProcessCall& c) {
    const auto s=state_->scheduler.lock();
    if(!s || !s->root(2) || c.kind!=ProcessCallKind::Service || c.target!=0x4242c8 ||
       c.has_self || c.this_adjustment || c.argument_count!=1)return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->type=c.arguments[0];return next;
}
std::optional<map::native::ActorVisualResult> NativeEventConsistency::last_actor_result() const {return state_->result;}
ProcessCall NativeEventConsistency::Call(ProcessHandle h,std::uint32_t type) {
    ProcessCall c;c.process=std::move(h);c.kind=ProcessCallKind::Service;c.target=0x4242c8;
    c.arguments[0]=type;c.argument_count=1;return c;
}
}
