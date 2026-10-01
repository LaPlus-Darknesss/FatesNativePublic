#include "fates/runtime/native_phase.hpp"
#include <algorithm>
#include <limits>

namespace fates::runtime::native {
SituationTurnEndResult EvaluateSituationTurnEnd(
    const SituationPhaseState& in, fates::chapter::native::SituationOutcomeState flags) noexcept {
    SituationTurnEndResult out{false,false,in,flags};
    if(in.active_force>=3) return out;
    out.valid=true;
    const int limit=in.turn_limit;
    const bool complete=limit>0 && in.active_force==1 && limit<=int(in.turn);
    const bool game_over=limit<0 && in.active_force==0 && -limit<=int(in.turn);
    if(complete || game_over) {
        bool& flag=complete?out.outcome.complete_flag:out.outcome.game_over_flag;
        if(!flag) {flag=true;out.outcome.raw_result=5;}
        return out;
    }
    out.advanced=true;
    auto& s=out.situation;
    ++s.active_force;
    if(s.active_force==3) {
        s.active_force=0;
        // ARM adds then UXTH before comparing against 1000. Preserve wrap even
        // for malformed/out-of-normal-domain u16 input in the pure transform.
        s.turn=std::uint16_t(unsigned(s.turn)+1u);
        if(s.turn>=1000) s.turn=999;
    }
    const bool any_human=std::find(s.control.begin(),s.control.end(),1)!=s.control.end();
    if(!any_human || s.control[s.active_force]==1) s.human_force=s.active_force;
    return out;
}
SituationPhaseState SelectFirstHumanForce(SituationPhaseState s) noexcept {
    for(std::uint8_t f=0;f<3;++f) if(s.control[f]==1) {s.human_force=f;break;}
    return s;
}
namespace {
bool SnapshotValid(const NativeRuntime& r,const SituationPhaseState& s) {
    return r.game.map_active && s.active_force<3 && s.human_force<3 && s.turn>0 && s.turn<=999;
}
PhaseContextStatus Bind(NativeRuntime& r,const SituationPhaseState& s,PhaseAccessStage stage) {
    auto& p=r.game.phase;
    if(p.stage!=PhaseAccessStage::Unbound) return PhaseContextStatus::AlreadyBound;
    if(!SnapshotValid(r,s)) return PhaseContextStatus::InvalidSnapshot;
    if(p.revision==std::numeric_limits<std::uint64_t>::max()) return PhaseContextStatus::RevisionExhausted;
    p.situation=s;p.chapter_index=r.game.campaign.current_chapter_index;
    p.stage=(r.game.outcome.complete_flag || r.game.outcome.game_over_flag)?PhaseAccessStage::Terminal:stage;
    r.game.force_upkeep={};
    ++p.revision;return PhaseContextStatus::Ok;
}
}
PhaseContextStatus BindSituationPhase(NativeRuntime& r,const SituationPhaseState& s) {
    return Bind(r,s,PhaseAccessStage::AwaitingEntryServices);
}
PhaseContextStatus RestorePreparedPhaseSnapshot(NativeRuntime& r,const SituationPhaseState& s,std::uint8_t chapter) {
    if(chapter!=r.game.campaign.current_chapter_index) return PhaseContextStatus::StaleChapter;
    return Bind(r,s,PhaseAccessStage::Ready);
}
PhaseContextStatus RequestPhaseEnd(NativeRuntime& r) {
    auto& p=r.game.phase;
    if(p.stage!=PhaseAccessStage::Ready || !r.game.map_active || r.game.outcome.complete_flag || r.game.outcome.game_over_flag)
        return PhaseContextStatus::NotReady;
    if(p.chapter_index!=r.game.campaign.current_chapter_index) return PhaseContextStatus::StaleChapter;
    if(p.revision==std::numeric_limits<std::uint64_t>::max()) return PhaseContextStatus::RevisionExhausted;
    p.stage=PhaseAccessStage::AwaitingExitServices;++p.revision;
    return PhaseContextStatus::AwaitingExitServices;
}
bool PhaseAllowsPlayerCommands(const NativeRuntime& r,std::uint8_t force) noexcept {
    const auto& p=r.game.phase;
    if(p.stage==PhaseAccessStage::Unbound) return true; // Existing explicit diagnostic harness.
    return p.stage==PhaseAccessStage::Ready && r.game.map_active &&
        !r.game.outcome.complete_flag && !r.game.outcome.game_over_flag &&
        p.chapter_index==r.game.campaign.current_chapter_index && p.situation.active_force<2 &&
        p.situation.active_force==force && p.situation.control[force]==1;
}
} // namespace fates::runtime::native
