#include "fates/runtime/native_phase_exit.hpp"
#include "fates/runtime/native_phase.hpp"
#include <limits>

namespace fates::runtime::native {
namespace {
using S=PhaseExitStatus;
using Stage=PhaseExitStage;
using Route=fates::chapter::native::MapSequenceEndRoute;
bool Same(const TacticalPhaseContext& a,const TacticalPhaseContext& b) noexcept {
    const auto& x=a.situation;const auto& y=b.situation;
    return a.revision==b.revision && a.stage==b.stage && a.chapter_index==b.chapter_index &&
        x.control==y.control && x.active_force==y.active_force && x.human_force==y.human_force &&
        x.turn==y.turn && x.turn_limit==y.turn_limit;
}
}
PhaseExitStatus NativePhaseExit::Create(std::shared_ptr<NativeRuntime> runtime,std::unique_ptr<NativePhaseExit>& out) {
    if(!runtime)return S::NullRuntime;
    const auto& p=runtime->game.phase;
    if(!runtime->game.map_active || p.stage!=PhaseAccessStage::AwaitingExitServices ||
        p.situation.active_force>=3 || p.situation.human_force>=3 ||
        p.chapter_index!=runtime->game.campaign.current_chapter_index)return S::InvalidPhase;
    // Host publication admission precedes any Unit write. Retail u16 turn wrap
    // remains in the existing reducer; this is only the host lifetime counter.
    if(p.revision==std::numeric_limits<std::uint64_t>::max())return S::RevisionExhausted;
    auto next=std::unique_ptr<NativePhaseExit>(new NativePhaseExit);
    next->runtime_=std::move(runtime);next->phase_=p;next->player_=next->runtime_->PlayerEvents();
    if(auto status=next->Validate();status!=S::Ready)return status;
    out=std::move(next);return S::Ready;
}
PhaseExitStatus NativePhaseExit::Validate() const noexcept {
    const auto& g=runtime_->game;
    if(!g.map_active || g.campaign.current_chapter_index!=phase_.chapter_index || !Same(g.phase,phase_))return S::StaleContext;
    if(!player_ || !player_->active() || runtime_->PlayerEvents()!=player_)return S::StalePlayerState;
    return S::Ready;
}
EventFlagStatus NativePhaseExit::Flag(EventFlagOperation operation,std::string_view name,EventFlagResult& out) {
    const auto& bank=player_->flags();
    if(!bank)return EventFlagStatus::MissingBank;
    PreparedEventFlagOperation plan;
    if(auto status=bank->Prepare(operation,name,plan);status!=EventFlagStatus::Ok)return status;
    return bank->Commit(plan,out);
}
PhaseExitObservation NativePhaseExit::Run() {
    auto& o=observation_;
    if(o.stage==Stage::Finished)return o;
    if(auto status=Validate();status!=S::Ready){o.status=status;return o;}
    auto& g=runtime_->game;
    if(o.stage==Stage::Upkeep) {
        o.upkeep=ApplyCurrentForceTurnUpkeep(*runtime_,ForceTurnOperation::End,phase_.revision);
        if(o.upkeep.status!=ForceTurnStatus::Ok){o.status=S::UpkeepBlocked;return o;}
        o.stage=Stage::Danger;
    }
    if(o.stage==Stage::Danger) {
        o.danger=fates::map::native::RefreshCurrentDangerImage(*runtime_);
        if(o.danger.status!=fates::map::native::DangerStatus::Ok){o.status=S::DangerBlocked;return o;}
        o.stage=Stage::Situation;
    }
    if(o.stage==Stage::Situation) {
        const auto next=EvaluateSituationTurnEnd(phase_.situation,{});
        if(next.advanced) {
            g.phase.situation=next.situation;o.advanced=true;
        } else {
            const auto name=phase_.situation.turn_limit>0?"S_Complete":"S_GameOver";
            EventFlagResult value;
            o.flags=Flag(EventFlagOperation::Get,name,value);
            if(o.flags!=EventFlagStatus::Ok){o.status=S::FlagsBlocked;return o;}
            if(*value.integer()==0) {
                o.flags=Flag(EventFlagOperation::Set,name,value);
                if(o.flags!=EventFlagStatus::Ok){o.status=S::FlagsBlocked;return o;}
                // Set on a missing name does NOT register it. Retail still
                // writes raw result5, then Sequence rereads the live bank.
                g.outcome.raw_result=5;
            }
        }
        o.stage=Stage::Branch;
    }
    if(o.stage==Stage::Branch && !o.advanced) {
        EventFlagResult value;
        o.flags=Flag(EventFlagOperation::Get,"S_GameOver",value);
        if(o.flags!=EventFlagStatus::Ok){o.status=S::FlagsBlocked;return o;}
        g.outcome.game_over_flag=*value.integer()!=0;
        if(g.outcome.game_over_flag)o.route=Route::GameOverLabel7;
        else {
            o.flags=Flag(EventFlagOperation::Get,"S_Complete",value);
            if(o.flags!=EventFlagStatus::Ok){o.status=S::FlagsBlocked;return o;}
            g.outcome.complete_flag=*value.integer()!=0;
            if(g.outcome.complete_flag)o.route=Route::CompleteLabel6;
        }
    }
    // Danger intentionally describes the pre-advance Situation. Its dependency
    // guard will reject it after this publication; do not reorder retail by
    // silently refreshing it a second time for the new human force.
    g.phase.stage=o.route==Route::Continue?PhaseAccessStage::AwaitingEntryServices:PhaseAccessStage::Terminal;
    ++g.phase.revision;
    o.stage=Stage::Finished;o.status=S::Complete;return o;
}
}
