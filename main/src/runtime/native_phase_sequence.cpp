#include "fates/runtime/native_phase_sequence.hpp"
#include "fates/runtime/native_force_order.hpp"
#include <algorithm>
#include <limits>

namespace fates::runtime::native {
namespace {
using S=PhaseSequenceStatus;
using E=event::native::ProcEventStatus;
constexpr std::uint32_t TurnEnd=0x3a4f0c,TurnBegin=0x3a54d8,Persistent=0x3a4898;
constexpr std::uint32_t GameEndBranch=0x3a4bd4,TurnSkip=0x3a5438,TurnBranch=0x3a48f4;
constexpr std::uint32_t TurnScroll=0x3a4b24;
constexpr std::array<std::uint32_t,4> Triggers{0x430acc,0x4225b0,0x4325b0,0x432534};
constexpr std::array Members{TurnEnd,TurnBegin,GameEndBranch,TurnSkip,TurnBranch,TurnScroll};
constexpr std::array Targets{Triggers[0],Triggers[1],Triggers[2],Triggers[3],TurnEnd,TurnBegin,Persistent,GameEndBranch,TurnSkip,TurnBranch};
bool SamePhase(const TacticalPhaseContext& a,const TacticalPhaseContext& b) noexcept {
    const auto& x=a.situation;const auto& y=b.situation;
    return a.revision==b.revision && a.stage==b.stage && a.chapter_index==b.chapter_index &&
        x.control==y.control && x.active_force==y.active_force && x.human_force==y.human_force &&
        x.turn==y.turn && x.turn_limit==y.turn_limit;
}
}
struct NativePhaseSequence::State {
    std::shared_ptr<NativeRuntime> runtime;
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<event::native::NativeProcEvent> event;
    std::shared_ptr<map::native::NativeMapCursor> cursor;
    PhaseSequenceObservation observation;
    std::optional<bool> versus;
    std::uint8_t chapter{};
    bool Valid() const {
        const auto& g=runtime->game;
        return g.map_active && g.campaign.current_chapter_index==chapter && g.phase.chapter_index==chapter &&
            g.phase.stage!=PhaseAccessStage::Unbound && g.phase.situation.active_force<3 && g.phase.situation.human_force<3;
    }
};
struct NativePhaseSequence::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;ProcessCall call;std::unique_ptr<NativePhaseExit> exit;
    TacticalPhaseContext phase;
    std::shared_ptr<NativePlayerEventState> player;
    std::optional<std::uint64_t> deployment_selection;
    ProcessCallbackStep Block(S status) {state->observation.status=status;return ProcessCallbackStep::Blocked();}
    ProcessCallbackStep Entry() {
        auto& r=*state->runtime;auto& o=state->observation;
        if(phase.stage!=PhaseAccessStage::AwaitingEntryServices || !SamePhase(r.game.phase,phase))return Block(S::StaleContext);
        // The cached Clarity lookup precedes Deploy::TurnReset in the original.
        // The upkeep owner resolves the same immutable definition for its loop.
        const auto* skill=r.definitions.FindSkill("SEID_\x90\x53\x93\xaa\x96\xc5\x8b\x70");
        if(!skill || skill->id>32767u) {
            o.entry_upkeep=ForceTurnResult{ForceTurnStatus::MissingDefinition,0xffffu,0};return Block(S::EntryBlocked);
        }
        using D=map::native::DeploymentWorkspaceStatus;
        if(!deployment_selection) {
            o.deployment_status=map::native::ResetCurrentDeploymentWorkspace(r,phase.revision);
            if(*o.deployment_status!=D::Ok)return Block(S::EntryBlocked);
            deployment_selection=map::native::ReadCurrentDeploymentWorkspace(r).selection_revision;
        } else {
            const auto selected=map::native::ReadCurrentDeploymentWorkspace(r);
            if(selected.status!=D::Ok || selected.selection_revision!=*deployment_selection)return Block(S::StaleContext);
        }
        o.entry_upkeep=ApplyCurrentForceTurnUpkeep(r,ForceTurnOperation::Begin,phase.revision);
        if(o.entry_upkeep->status!=ForceTurnStatus::Ok)return Block(S::EntryBlocked);
        // Entry scripts, effects and dispatch are still ahead in the real program.
        o.status=S::Ready;return ProcessCallbackStep::Return();
    }
    ProcessCallbackStep Decision(ProcessAccess& access) {
        auto& g=state->runtime->game;auto& o=state->observation;
        if(phase.stage!=PhaseAccessStage::AwaitingEntryServices || !SamePhase(g.phase,phase))return Block(S::StaleContext);
        std::optional<std::uint32_t> label;
        bool game_over=false,complete=false;
        if(call.target==GameEndBranch) {
            if(!player || !player->active() || player!=state->runtime->PlayerEvents())return Block(S::StaleContext);
            const auto& bank=player->flags();
            if(!bank){o.flags_status=EventFlagStatus::MissingBank;return Block(S::FlagsBlocked);}
            const auto get=[&](const char* name,bool& value) {
                PreparedEventFlagOperation plan;EventFlagResult result;
                auto status=bank->Prepare(EventFlagOperation::Get,name,plan);
                if(status==EventFlagStatus::Ok)status=bank->Commit(plan,result);
                o.flags_status=status;if(status!=EventFlagStatus::Ok)return false;
                value=*result.integer()!=0;return true;
            };
            if(!get("S_GameOver",game_over))return Block(S::FlagsBlocked);
            if(game_over)label=7;
            else {
                if(!get("S_Complete",complete))return Block(S::FlagsBlocked);
                if(complete)label=6;
            }
        } else {
            const auto force=phase.situation.active_force,control=phase.situation.control[force];
            if(call.target==TurnSkip) {
                // Human control never reaches the Force count. All linked Units
                // count, regardless of HP, position, flags or command lifetime.
                if(control!=1) {
                    const auto* order=GetVerifiedForceOrder(g,force);
                    if(!order)return Block(S::ForceOrderRequired);
                    if(order->count==0)label=5;
                }
            } else label=control>=1 && control<=3?std::uint32_t(control)+1:5u;
        }
        const bool publish=label && *label>=5;
        if(publish && phase.revision==std::numeric_limits<std::uint64_t>::max())return Block(S::RevisionExhausted);
        if(label && access.Jump(call.process,*label)!=ProcessStatus::Ready)return Block(S::InvalidProcess);
        if(call.target==GameEndBranch) {
            g.outcome.game_over_flag=game_over;
            if(!game_over)g.outcome.complete_flag=complete;
        }
        if(publish) {
            // Real event destruction was checked before this context change.
            // Common-exit selection does not imply entry or gameplay Ready.
            g.phase.stage=*label==5?PhaseAccessStage::AwaitingExitServices:PhaseAccessStage::Terminal;
            ++g.phase.revision;
        }
        o.branch_label=label;o.status=S::Ready;return ProcessCallbackStep::Return();
    }
    ProcessCallbackStep Step(ProcessAccess& access) override {
        auto scheduler=state->scheduler.lock();
        if(!scheduler || !access.BelongsTo(*scheduler) || !state->Valid())return Block(S::StaleContext);
        if(call.target==Persistent) {
            if(!state->versus)return Block(S::UnknownVersusState);
            if(*state->versus)return Block(S::VersusSenderRequired);
            state->observation.status=S::Ready;return ProcessCallbackStep::Return();
        }
        const auto current=state->event->Current();
        if(!current || !state->event->UsesRuntime(*state->runtime))return Block(S::StaleContext);
        if(call.target==TurnScroll) {
            if(*current)return Block(S::AwaitingEventDestruction);
            if(phase.stage!=PhaseAccessStage::AwaitingEntryServices || !SamePhase(state->runtime->game.phase,phase))return Block(S::StaleContext);
            if(!state->cursor)return Block(S::CursorBlocked);
            map::native::TurnCursorSelection selected;
            const auto status=state->cursor->TurnScroll(access,selected);selected.status=status;
            state->observation.cursor=std::move(selected);
            if(status!=map::native::MapCursorStatus::Ready)return Block(S::CursorBlocked);
            state->observation.status=S::Ready;return ProcessCallbackStep::Return();
        }
        if(call.target==TurnBegin) {
            if(*current)return Block(S::AwaitingEventDestruction);
            return Entry();
        }
        if(call.target==GameEndBranch || call.target==TurnSkip || call.target==TurnBranch) {
            if(*current)return Block(S::AwaitingEventDestruction);
            return Decision(access);
        }
        if(call.target==TurnEnd) {
            // Marking a child dead releases its parent's block BEFORE Sweep.
            // Context mutation must not invalidate the still-retained event VM
            // and cleanup owner. A wrong host frame protocol is a real barrier.
            if(*current)return Block(S::AwaitingEventDestruction);
            if(!exit) {
                const auto status=NativePhaseExit::Create(state->runtime,exit);
                if(status!=PhaseExitStatus::Ready) {
                    PhaseExitObservation observation;observation.status=status;state->observation.exit=observation;
                    return Block(S::ExitBlocked);
                }
            }
            const auto result=exit->Run();state->observation.exit=result;
            if(result.status!=PhaseExitStatus::Complete)return Block(S::ExitBlocked);
            if(result.route!=fates::chapter::native::MapSequenceEndRoute::Continue &&
                access.Jump(call.process,static_cast<std::uint32_t>(result.route))!=ProcessStatus::Ready)return Block(S::InvalidProcess);
            state->observation.status=S::Ready;return ProcessCallbackStep::Return();
        }
        if(!*current) {
            std::shared_ptr<event::native::NativePhaseEventQueries> queries;
            std::shared_ptr<event::native::NativeEventFlagCommands> flags;
            if(event::native::NativePhaseEventQueries::Bind(state->runtime,queries)!=event::native::PhaseQueryStatus::Ok ||
                event::native::NativeEventFlagCommands::Bind(state->runtime,flags)!=EventFlagStatus::Ok)return Block(S::StaleContext);
            const auto bound=state->event->BindContextServices(access,std::move(queries),std::move(flags));
            if(bound!=E::Ready){state->observation.event_status=bound;return Block(S::EventBlocked);}
        }
        for(std::uint32_t i=0;i<Triggers.size();++i)if(call.target==Triggers[i]) {
            ProcessHandle child;
            const auto result=state->event->CreateTyped(access,call.process,16+i,event::native::ProcEventInspector::Phase,child);
            state->observation.event_status=result;
            if(result!=E::Ready && result!=E::NoMatch && result!=E::AlreadyActive)return Block(S::EventBlocked);
            state->observation.status=S::Ready;return ProcessCallbackStep::Return(result==E::Ready?1:0);
        }
        return Block(S::InvalidProcess);
    }
};
NativePhaseSequence::NativePhaseSequence(std::shared_ptr<State> s):state_(std::move(s)){}
NativePhaseSequence::~NativePhaseSequence()=default;
S NativePhaseSequence::Create(std::shared_ptr<NativeRuntime> runtime,std::shared_ptr<NativeProcessScheduler> scheduler,
    std::shared_ptr<ProcessCallbackRegistry> registry,std::shared_ptr<event::native::NativeProcEvent> event,
    std::shared_ptr<NativePhaseSequence>& out,std::shared_ptr<map::native::NativeMapCursor> cursor) {
    if(!runtime)return S::NullRuntime;
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(scheduler->busy())return S::Busy;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !event || !event->UsesScheduler(*scheduler) ||
        !event->UsesRuntime(*runtime))return S::MismatchedDomain;
    if(cursor && (!cursor->UsesRuntime(*runtime) || !cursor->UsesScheduler(*scheduler)))return S::MismatchedDomain;
    auto s=std::make_shared<State>();s->runtime=std::move(runtime);s->scheduler=scheduler;s->event=std::move(event);
    s->cursor=std::move(cursor);
    auto next=std::shared_ptr<NativePhaseSequence>(new NativePhaseSequence(s));
    std::vector<std::uint32_t> targets(Targets.begin(),Targets.end());if(s->cursor)targets.push_back(TurnScroll);
    if(!registry->Register(targets,next))return S::DuplicateBinding;
    out=std::move(next);return S::Ready;
}
S NativePhaseSequence::BindCarried(ProcessHandle h,std::optional<bool> versus) {
    auto scheduler=state_->scheduler.lock();if(!scheduler)return S::InvalidProcess;
    if(scheduler->busy())return S::Busy;
    const auto p=scheduler->Observe(h);
    if(!p || !p->linked || p->root || (p->flags&1) || p->program!=Program() ||
        p->persistent_target!=12 || p->persistent_adjustment!=1 || !scheduler->HasType(h,Type()))return S::InvalidProcess;
    if(state_->observation.process && scheduler->Observe(state_->observation.process))return S::AlreadyBound;
    const auto& g=state_->runtime->game;
    if(!g.map_active || g.phase.stage==PhaseAccessStage::Unbound || g.phase.chapter_index!=g.campaign.current_chapter_index ||
        g.phase.situation.active_force>=3 || g.phase.situation.human_force>=3)return S::StaleContext;
    state_->observation={};state_->observation.process=std::move(h);state_->versus=versus;
    state_->chapter=g.phase.chapter_index;return S::Ready;
}
PhaseSequenceObservation NativePhaseSequence::Observe() const {return state_->observation;}
ProcessType NativePhaseSequence::Type() {
    auto type=ProcessType::Base();type.methods[0].target=0x3a55dc;type.methods[2].target=Persistent;return type;
}
std::unique_ptr<ProcessContinuation> NativePhaseSequence::Begin(const ProcessCall& c) {
    if(c.process!=state_->observation.process || !c.process || c.this_adjustment || c.argument_count)return {};
    if(c.target==Persistent) {if(c.kind!=ProcessCallKind::Persistent || !c.has_self)return {};}
    else {
        if(c.kind!=ProcessCallKind::Descriptor)return {};
        const bool member=std::find(Members.begin(),Members.end(),c.target)!=Members.end();
        // Descriptor8 passes its ProcInst* to a free trigger function;11 passes
        // it as the member receiver. Both retain the same process argument.
        if(c.command!=(member?11:8) || !c.has_self)return {};
        if(!member && std::find(Triggers.begin(),Triggers.end(),c.target)==Triggers.end())return {};
    }
    auto out=std::make_unique<Continuation>();out->state=state_;out->call=c;
    out->phase=state_->runtime->game.phase;out->player=state_->runtime->PlayerEvents();return out;
}
// All60 records are retained, including unowned entry/audio/child services.
std::shared_ptr<const ProcessProgram> NativePhaseSequence::Program() {
    static const auto program=ProcessProgram::Create({
        {0x00000008,0x00000000,0x00000000,0x00397550,0x00000000},
        {0x00000004,0x00000000,0x00000000,0x00000000,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x003a54d8,0x00000000},
        {0x00000008,0x00000000,0x00000000,0x00430acc,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x003a4bd4,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x003a5438,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x003a4b24,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x003a4bac,0x00000000},
        {0x00000011,0x000000fa,0x00000000,0x00398e2c,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x003a5480,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x003a4948,0x00000000},
        {0x00000011,0x00000fa0,0x00000000,0x00398e58,0x00000000},
        {0x00000011,0x00000fa0,0x00000000,0x00398fbc,0x00000000},
        {0x00000008,0x00000000,0x00000000,0x0035e480,0x00000000},
        {0x00000008,0x00000000,0x00000000,0x003617d8,0x00000000},
        {0x00000008,0x00000000,0x00000000,0x004225b0,0x00000000},
        {0x00000008,0x00000000,0x00000000,0x0036b918,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x003a4db4,0x00000000},
        {0x00000008,0x00000000,0x00000000,0x004325b0,0x00000000},
        {0x00000011,0x00000fa0,0x00000000,0x00398fbc,0x00000000},
        {0x00000004,0x00000001,0x00000000,0x00000000,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x003a48f4,0x00000000},
        {0x00000004,0x00000002,0x00000000,0x00000000,0x00000000},
        {0x00000008,0x00000000,0x00000000,0x00354604,0x00000000},
        {0x00000003,0x00000005,0x00000000,0x00000000,0x00000000},
        {0x00000004,0x00000003,0x00000000,0x00000000,0x00000000},
        {0x00000008,0x00000000,0x00000000,0x0034bc40,0x00000000},
        {0x00000003,0x00000005,0x00000000,0x00000000,0x00000000},
        {0x00000004,0x00000004,0x00000000,0x00000000,0x00000000},
        {0x00000008,0x00000000,0x00000000,0x0034e4a0,0x00000000},
        {0x00000003,0x00000005,0x00000000,0x00000000,0x00000000},
        {0x00000004,0x00000005,0x00000000,0x00000000,0x00000000},
        {0x00000011,0x000000fa,0x00000000,0x00398e2c,0x00000000},
        {0x00000008,0x00000000,0x00000000,0x00432534,0x00000000},
        {0x00000011,0x00000fa0,0x00000000,0x00398fbc,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x003a4f0c,0x00000000},
        {0x00000003,0x00000000,0x00000000,0x00000000,0x00000000},
        {0x00000004,0x00000006,0x00000000,0x00000000,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x003a47b8,0x00000000},
        {0x00000011,0x000000fa,0x00000000,0x00399164,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x003a4ffc,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x00371928,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x003a4e40,0x00000000},
        {0x00000008,0x00000000,0x00000000,0x00430cb0,0x00000000},
        {0x00000003,0x00000008,0x00000000,0x00000000,0x00000000},
        {0x00000004,0x00000007,0x00000000,0x00000000,0x00000000},
        {0x00000005,0x000001f4,0x00000000,0x00000000,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x003a47b8,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x003a5434,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x003a4c50,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x003a4e40,0x00000000},
        {0x00000008,0x00000000,0x00000000,0x00430ce8,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x003a4c2c,0x00000000},
        {0x00000003,0x00000009,0x00000000,0x00000000,0x00000000},
        {0x00000004,0x00000008,0x00000000,0x00000000,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x003a4e40,0x00000000},
        {0x00000011,0x000000fa,0x00000000,0x00399164,0x00000000},
        {0x00000004,0x00000009,0x00000000,0x00000000,0x00000000},
        {0x00000008,0x00000000,0x00000000,0x00176e20,0x00000000},
        {0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
    });return program;
}
}
