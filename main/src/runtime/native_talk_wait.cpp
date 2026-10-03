#include "fates/runtime/native_talk_wait.hpp"
#include <bit>
#include <map>

namespace fates::runtime::native {
namespace {
using S=TalkWaitStatus;
constexpr std::uint32_t Tick=0x4f30bc,Destroy=0x4f3150;
constexpr std::array Targets{Tick,Destroy};
}
struct NativeTalkWait::State {
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<NativeGameSkip> skip;
    std::shared_ptr<TalkWaitClockSource> clocks;
    std::map<std::uint64_t,TalkWaitSnapshot> rows;
    S Mutable(ProcessAccess* access) const {
        const auto owner=scheduler.lock();if(!owner || !owner->root(2))return S::Retired;
        if(access)return access->BelongsTo(*owner)?S::Ready:S::MismatchedDomain;
        return owner->busy()?S::Busy:S::Ready;
    }
    TalkWaitSnapshot* Find(const ProcessHandle& process) {
        if(!process)return nullptr;
        const auto found=rows.find(process->serial);
        return found!=rows.end() && found->second.process==process?&found->second:nullptr;
    }
    S Bind(ProcessAccess* access,ProcessHandle parent,std::int32_t milliseconds,std::uint32_t type,ProcessHandle& out) {
        if(const auto status=Mutable(access);status!=S::Ready)return status;
        const auto owner=scheduler.lock();const auto view=access?access->Observe(parent):owner->Observe(parent);
        if(!view || !view->linked || (view->flags&1u))return S::InvalidParent;
        auto process_type=ProcessType::Base();process_type.methods[0].target=Destroy;process_type.methods[1].target=Tick;
        ProcessHandle next;
        const auto result=access?access->Create(parent,ProcessProgram::Default(),"ProcWait",true,process_type,next):
            owner->Create(parent,ProcessProgram::Default(),"ProcWait",true,process_type,next);
        if(result!=ProcessStatus::Ready)return S::InvalidParent;
        rows.emplace(next->serial,TalkWaitSnapshot{next,ProcessMillisecondsToFrames(milliseconds),static_cast<std::uint8_t>(type),0});
        out=std::move(next);return S::Ready;
    }
};
struct NativeTalkWait::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;
    ProcessCall call;
    unsigned stage{};
    ProcessCallbackStep Step(ProcessAccess& access) override {
        if(state->Mutable(&access)!=S::Ready)return ProcessCallbackStep::Blocked();
        auto* row=state->Find(call.process);if(!row)return ProcessCallbackStep::Blocked();
        if(call.target==Destroy){state->rows.erase(call.process->serial);return ProcessCallbackStep::Return();}
        if(stage==0) {
            if(row->type==0)row->remaining-=access.frame_delta();
            else if(row->type==1) {
                const auto delta=state->clocks?state->clocks->AlternateFrameDelta():std::nullopt;
                if(!delta)return ProcessCallbackStep::Blocked();
                row->remaining-=*delta;
            }
            // Other byte values do not consume either clock in the executable.
            // Retain this stage before querying skip, so an unknown dependency
            // cannot decrement the same counter twice when the callback retries.
            stage=1;
        }
        if(stage==1) {
            const auto current=state->skip->Current();if(!current)return ProcessCallbackStep::Blocked();
            bool skipping=false;
            if(*current) {
                const auto view=state->skip->Observe(*current);if(!view)return ProcessCallbackStep::Blocked();
                skipping=view->state!=0;
            }
            stage=2;
            if(skipping){++row->deletion_requests;return ProcessCallbackStep::Delete(call.process);}
        }
        if(stage==2) {
            stage=3;
            if(std::bit_cast<std::int32_t>(row->remaining)<=0) {
                ++row->deletion_requests;return ProcessCallbackStep::Delete(call.process);
            }
        }
        return ProcessCallbackStep::Return();
    }
};
NativeTalkWait::NativeTalkWait(std::shared_ptr<State> state):state_(std::move(state)){}
NativeTalkWait::~NativeTalkWait()=default;
TalkWaitStatus NativeTalkWait::Create(std::shared_ptr<NativeProcessScheduler> scheduler,
    std::shared_ptr<ProcessCallbackRegistry> registry,std::shared_ptr<NativeGameSkip> skip,
    std::shared_ptr<TalkWaitClockSource> clocks,std::shared_ptr<NativeTalkWait>& out) {
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !skip || !skip->UsesScheduler(*scheduler))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->skip=std::move(skip);state->clocks=std::move(clocks);
    auto next=std::shared_ptr<NativeTalkWait>(new NativeTalkWait(state));
    if(!registry->Register(Targets,next))return S::DuplicateBinding;
    out=std::move(next);return S::Ready;
}
TalkWaitStatus NativeTalkWait::Bind(ProcessHandle parent,std::int32_t ms,std::uint32_t type,ProcessHandle& out) {
    return state_->Bind(nullptr,std::move(parent),ms,type,out);
}
TalkWaitStatus NativeTalkWait::Bind(ProcessAccess& access,ProcessHandle parent,std::int32_t ms,std::uint32_t type,ProcessHandle& out) {
    return state_->Bind(&access,std::move(parent),ms,type,out);
}
TalkWaitStatus NativeTalkWait::RestoreCarried(ProcessHandle process,std::uint32_t remaining,std::uint8_t type) {
    if(const auto status=state_->Mutable(nullptr);status!=S::Ready)return status;
    const auto owner=state_->scheduler.lock();const auto view=owner->Observe(process);auto* row=state_->Find(process);
    if(!row || !view || !view->linked || (view->flags&1u))return S::InvalidHandle;
    row->remaining=remaining;row->type=type;return S::Ready;
}
std::optional<TalkWaitSnapshot> NativeTalkWait::Observe(ProcessHandle process) const {
    const auto owner=state_->scheduler.lock();if(!owner || !owner->root(2) || !owner->Observe(process))return {};
    const auto* row=state_->Find(process);return row?std::optional(*row):std::nullopt;
}
std::vector<ProcessHandle> NativeTalkWait::Processes() const {
    std::vector<ProcessHandle> out;const auto owner=state_->scheduler.lock();if(!owner || !owner->root(2))return out;
    for(const auto& [serial,row]:state_->rows){(void)serial;out.push_back(row.process);}return out;
}
bool NativeTalkWait::UsesScheduler(const NativeProcessScheduler& owner) const noexcept {return state_->scheduler.lock().get()==&owner;}
std::unique_ptr<ProcessContinuation> NativeTalkWait::Begin(const ProcessCall& call) {
    if(!call.process || !call.has_self || call.this_adjustment || call.argument_count || !state_->Find(call.process))return {};
    if(call.target==Destroy){if(call.kind!=ProcessCallKind::Destroy)return {};}
    else if(call.target!=Tick || call.kind!=ProcessCallKind::Descriptor || call.command!=13)return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->call=call;return next;
}
}

namespace fates::runtime::native {
bool NativeTalkWait::UsesGameSkip(const NativeGameSkip& skip) const noexcept {return state_->skip.get()==&skip;}
}
