#include "fates/runtime/native_talk_key_wait.hpp"
#include <algorithm>
#include <limits>
#include <map>

namespace fates::runtime::native {
using namespace presentation::native;
using S=TalkKeyWaitStatus;
namespace {
constexpr std::uint32_t Dispose=0x1a6574,Tick=0x1a6468,Destroy=0x1a6530;
constexpr std::array Targets{Dispose,Tick,Destroy};
ProcessCall Service(ProcessHandle process,std::uint32_t target,std::initializer_list<std::uint32_t> args={}) {
    ProcessCall call;call.process=std::move(process);call.kind=ProcessCallKind::Service;call.target=target;
    call.argument_count=static_cast<std::uint8_t>(args.size());std::copy(args.begin(),args.end(),call.arguments.begin());return call;
}
}
struct NativeTalkKeyWait::State {
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<NativeGameSkip> skip;std::shared_ptr<GameSkipInputSource> input;
    std::shared_ptr<NativeTalkWindow> windows;std::shared_ptr<NativeTalkWindowEffects> effects;
    std::shared_ptr<TalkKeyWaitManagerState> managers;std::shared_ptr<NativeTalkLog> log;
    std::map<std::uint64_t,TalkKeyWaitSnapshot> rows;
    S Mutable(ProcessAccess* access) const {
        const auto owner=scheduler.lock();if(!owner || !owner->root(2))return S::Retired;
        if(access)return access->BelongsTo(*owner)?S::Ready:S::MismatchedDomain;
        return owner->busy()?S::Busy:S::Ready;
    }
    TalkKeyWaitSnapshot* Find(ProcessHandle handle) {
        if(!handle)return nullptr;
        const auto found=rows.find(handle->serial);return found!=rows.end() && found->second.process==handle?&found->second:nullptr;
    }
};
struct NativeTalkKeyWait::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;ProcessCall call;unsigned stage{};std::uint32_t trigger{};
    ProcessCallbackStep WindowFlag(ProcessAccess&,ProcessHandle manager,bool value) {
        const auto current=state->managers->CurrentWindow(manager);
        if(!current || !*current || !state->windows->Observe(*current))return ProcessCallbackStep::Blocked();
        const auto child=state->effects->Call(manager,*current,TalkWindowEffect::SetKeyWait,value?1:0);
        if(!child)return ProcessCallbackStep::Blocked();
        ++stage;return ProcessCallbackStep::Call(*child);
    }
    ProcessCallbackStep DisposeStep(ProcessAccess& access) {
        if(stage==0) {
            const auto view=access.Observe(call.process);
            if(!view || !view->linked || (view->flags&1u))return ProcessCallbackStep::Blocked();
            const auto skip=state->managers->Skip(call.process);if(!skip)return ProcessCallbackStep::Blocked();
            if(*skip){stage=3;return ProcessCallbackStep::Call(Service(call.process,0x41f6c4,{100}));}
            stage=1;
        }
        if(stage==1)return WindowFlag(access,call.process,true);
        if(stage==2) {
            auto type=ProcessType::Base();type.methods[0].target=Destroy;type.methods[1].target=Tick;
            ProcessHandle child;
            // Original unlinked allocation/constructor prefix precedes SetKeyWait;
            // scheduler publication follows it. No missing allocation is treated
            // as a completed child. The carried counter is exactly1.
            if(access.Create(call.process,ProcessProgram::Default(),"TalkKeyWait",true,type,child)!=ProcessStatus::Ready)
                return ProcessCallbackStep::Blocked();
            state->rows.emplace(child->serial,TalkKeyWaitSnapshot{child,call.process,1,0,0,0,0});stage=3;
        }
        return ProcessCallbackStep::Return(2);
    }
    ProcessCallbackStep Step(ProcessAccess& access) override {
        if(state->Mutable(&access)!=S::Ready)return ProcessCallbackStep::Blocked();
        if(call.target==Dispose)return DisposeStep(access);
        auto* row=state->Find(call.process);if(!row)return ProcessCallbackStep::Blocked();
        if(call.target==Destroy) {
            // The false SetKeyWait path is only the already-owned byte store.
            // Call that concrete storage owner under this destructor's access;
            // do not issue a new service on a manager already marked Delete.
            // This preserves parent teardown without weakening scheduler admission.
            const auto current=state->managers->CurrentWindow(row->manager);
            if(!current || !*current)return ProcessCallbackStep::Blocked();
            const auto window=state->windows->Observe(*current);if(!window)return ProcessCallbackStep::Blocked();
            if(state->windows->RestoreEffectFlags(*current,window->active,window->first_message,0,&access)!=TalkWindowStatus::Ready)return ProcessCallbackStep::Blocked();
            state->rows.erase(call.process->serial);return ProcessCallbackStep::Return();
        }
        if(stage==0) {
            --row->counter;stage=1;
            // SUBS/BPL tests the resulting sign bit, including32-bit wrap. This
            // is one decrement PER CALLBACK, not the scheduler's frame delta.
            if(!(row->counter&0x80000000u))return ProcessCallbackStep::Return();
        }
        if(stage==1) {
            const auto current=state->skip->Current();if(!current)return ProcessCallbackStep::Blocked();
            bool skipping=false;
            if(*current){const auto record=state->skip->Observe(*current);if(!record)return ProcessCallbackStep::Blocked();skipping=record->state!=0;}
            stage=skipping?2u:3u;
        }
        if(stage==2) {
            // Original StartSkip is Jump(manager,label5), not a skip-byte write.
            if(access.Jump(row->manager,5)!=ProcessStatus::Ready)return ProcessCallbackStep::Blocked();
            ++row->skip_jumps;stage=7;++row->deletion_requests;return ProcessCallbackStep::Delete(call.process);
        }
        if(stage==3) {
            const auto keys=state->input->TriggerButtons();if(!keys)return ProcessCallbackStep::Blocked();
            trigger=*keys;
            if(trigger&1u) {
                stage=4;++row->confirm_calls;
                return ProcessCallbackStep::Call(Service(call.process,0x4200fc,{0x1a6510}));
            }
            if(!(trigger&0x100u))return ProcessCallbackStep::Return();
            stage=5;
        }
        if(stage==4){stage=7;++row->deletion_requests;return ProcessCallbackStep::Delete(call.process);}
        if(stage==5) {
            const auto tutorial=state->input->Tutorial();if(!tutorial)return ProcessCallbackStep::Blocked();
            if(tutorial->present && !tutorial->mode)return ProcessCallbackStep::Return();
            const auto log=state->log->Observe();const auto id=log?log->identity->serial:std::uint64_t{0};
            if(id>std::numeric_limits<std::uint32_t>::max())return ProcessCallbackStep::Blocked();
            stage=6;++row->log_viewer_calls;
            // A log viewer blocks THIS key-wait child. Its callback's return does
            // not delete the key-wait or make the manager's key wait complete.
            return ProcessCallbackStep::Call(Service(call.process,0x1cb1f8,{static_cast<std::uint32_t>(id),1}));
        }
        return ProcessCallbackStep::Return();
    }
};
NativeTalkKeyWait::NativeTalkKeyWait(std::shared_ptr<State> state):state_(std::move(state)){}
NativeTalkKeyWait::~NativeTalkKeyWait()=default;
S NativeTalkKeyWait::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<NativeGameSkip> skip,std::shared_ptr<GameSkipInputSource> input,std::shared_ptr<NativeTalkWindow> windows,
    std::shared_ptr<NativeTalkWindowEffects> effects,std::shared_ptr<TalkKeyWaitManagerState> managers,
    std::shared_ptr<NativeTalkLog> log,std::shared_ptr<NativeTalkKeyWait>& out) {
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !skip || !skip->UsesScheduler(*scheduler) || !input || !skip->UsesInput(*input)
        || !windows || !windows->UsesScheduler(*scheduler) || !effects || !effects->UsesScheduler(*scheduler)
        || !managers || !managers->UsesScheduler(*scheduler) || !effects->UsesOwners(*windows,*managers) || !log)return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->skip=std::move(skip);state->input=std::move(input);
    state->windows=std::move(windows);state->effects=std::move(effects);state->managers=std::move(managers);state->log=std::move(log);
    auto next=std::shared_ptr<NativeTalkKeyWait>(new NativeTalkKeyWait(state));
    if(!registry->Register(Targets,next))return S::DuplicateBinding;
    out=std::move(next);return S::Ready;
}
std::optional<ProcessCall> NativeTalkKeyWait::DisposeCall(ProcessHandle manager) const {
    const auto owner=state_->scheduler.lock();if(!owner || !owner->root(2))return {};
    const auto view=owner->Observe(manager);if(!view || !view->linked || (view->flags&1u))return {};
    return Service(std::move(manager),Dispose);
}
S NativeTalkKeyWait::RestoreCounter(ProcessHandle process,std::uint32_t counter) {
    if(const auto result=state_->Mutable(nullptr);result!=S::Ready)return result;
    const auto owner=state_->scheduler.lock();const auto view=owner->Observe(process);auto* row=state_->Find(process);
    if(!row || !view || !view->linked || (view->flags&1u))return S::InvalidHandle;
    row->counter=counter;return S::Ready;
}
std::optional<TalkKeyWaitSnapshot> NativeTalkKeyWait::Observe(ProcessHandle process) const {
    const auto owner=state_->scheduler.lock();if(!owner || !owner->root(2) || !owner->Observe(process))return {};
    const auto* row=state_->Find(process);return row?std::optional{*row}:std::nullopt;
}
std::vector<ProcessHandle> NativeTalkKeyWait::Processes() const {
    std::vector<ProcessHandle> result;const auto owner=state_->scheduler.lock();if(!owner || !owner->root(2))return result;
    for(const auto& [id,row]:state_->rows){(void)id;result.push_back(row.process);}return result;
}
bool NativeTalkKeyWait::UsesScheduler(const NativeProcessScheduler& owner) const noexcept{return state_->scheduler.lock().get()==&owner;}
std::unique_ptr<ProcessContinuation> NativeTalkKeyWait::Begin(const ProcessCall& call) {
    if(!call.process || call.argument_count || call.this_adjustment)return {};
    if(call.target==Dispose){if(call.kind!=ProcessCallKind::Service || call.has_self || !DisposeCall(call.process))return {};}
    else if(!call.has_self || !state_->Find(call.process))return {};
    else if(call.target==Destroy){if(call.kind!=ProcessCallKind::Destroy)return {};}
    else if(call.target!=Tick || call.kind!=ProcessCallKind::Descriptor || call.command!=13)return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->call=call;return next;
}
}

namespace fates::runtime::native {
bool NativeTalkKeyWait::UsesOwners(const TalkKeyWaitManagerState& managers,const NativeTalkLog& log,const presentation::native::NativeTalkWindow& windows) const noexcept {
    return state_->managers.get()==&managers && state_->log.get()==&log && state_->windows.get()==&windows;
}
}
