#include "fates/ai/native_ai_phase.hpp"
#include "fates/ai/native_ai_shared_battle_preview.hpp"
#include <algorithm>
#include <array>
namespace fates::ai::native {
std::vector<AiPhaseOrderEntry> BuildStablePhaseOrder(std::span<const AiPhaseActorInput> actors) { std::vector<AiPhaseOrderEntry> out;out.reserve(actors.size());for(const auto&a:actors)out.push_back({a.unit_slot,ResolvePriorityScore(a.priority_byte,a.move_power)});std::stable_sort(out.begin(),out.end(),[](const auto&a,const auto&b){return a.score>b.score;});return out; }
AiPhaseActorBindResult BindPhaseActorsFromRuntimeAi(const fates::runtime::native::NativeRuntime& runtime,std::span<const AiPhaseRuntimeActorInput> seeds) {
    AiPhaseActorBindResult out{};out.actors.reserve(seeds.size());
    for(const auto& seed:seeds) {
        if(seed.unit_slot>=runtime.game.units.size()) { out.status=AiPhaseActorBindStatus::InvalidUnit;out.actors.clear();return out; }
        const auto& unit=runtime.game.units[seed.unit_slot];
        if(!unit.ai.runtime_tuning_bound) { out.status=AiPhaseActorBindStatus::MissingRuntimeTuning;out.actors.clear();return out; }
        out.actors.push_back({seed.unit_slot,unit.ai.priority,seed.move_power});
    }
    return out;
}
AiPhaseResult ExecuteOrderedAttackPhase(fates::runtime::native::NativeRuntime& runtime,std::span<const AiPhaseActorInput> actors,const AiPhaseContext context) {
    using fates::runtime::native::PhaseAccessStage;
    AiPhaseResult out{};const auto& phase=runtime.game.phase;
    if(phase.stage!=PhaseAccessStage::Unbound) {
        if(!runtime.game.map_active || runtime.game.outcome.complete_flag || runtime.game.outcome.game_over_flag || phase.stage==PhaseAccessStage::Terminal)
            out.phase_access=AiPhaseAccessStatus::Terminal;
        else if(phase.chapter_index!=runtime.game.campaign.current_chapter_index)out.phase_access=AiPhaseAccessStatus::StaleChapter;
        else if(phase.stage!=PhaseAccessStage::Ready)out.phase_access=AiPhaseAccessStatus::AwaitingServices;
        else if(phase.situation.active_force>=3)out.phase_access=AiPhaseAccessStatus::WrongForce;
        else if(phase.situation.control[phase.situation.active_force]==1)out.phase_access=AiPhaseAccessStatus::HumanControlled;
        else if(phase.situation.turn!=context.current_turn)out.phase_access=AiPhaseAccessStatus::TurnMismatch;
        if(out.phase_access!=AiPhaseAccessStatus::Allowed){out.status=AiPhaseStatus::PhaseContextRejected;return out;}
    }
    // Preflight the WHOLE batch. A stale/duplicate/wrong-force later seed must
    // not be discovered after earlier actors have already changed the world.
    std::array<bool,250> seen{};
    for(const auto& a:actors) {
        if(a.unit_slot>=runtime.game.units.size() || seen[a.unit_slot]){out.status=AiPhaseStatus::InvalidActorSet;return out;}
        seen[a.unit_slot]=true;
        const auto& u=runtime.game.units[a.unit_slot];
        if(phase.stage!=PhaseAccessStage::Unbound && u.occupied && u.force_type!=phase.situation.active_force) {
            out.phase_access=AiPhaseAccessStatus::WrongForce;out.status=AiPhaseStatus::PhaseContextRejected;return out;
        }
    }
    const auto gb=runtime.game.rng.game,ab=runtime.game.rng.ai;
    for(const auto& e:BuildStablePhaseOrder(actors)) {
        auto& u=runtime.game.units[e.unit_slot];
        if(!u.occupied || !fates::runtime::native::IsTacticalForce(u.force_type) || u.defeated || u.action_committed)continue;
        auto action=ExecuteConfiguredAiAction(runtime,e.unit_slot,context.current_turn,context.candidate_preview);
        if(action.status==AiActionStatus::InactiveAction){out.inactive_unit_slots.push_back(e.unit_slot);continue;}
        out.steps.push_back({e.unit_slot,action});
        if(action.status!=AiActionStatus::Ok){out.status=AiPhaseStatus::ActionRejected;break;}
    }
    out.game_rng_draws=runtime.game.rng.game-gb;out.ai_rng_draws=runtime.game.rng.ai-ab;return out;
}
AiPhaseResult ExecuteOrderedAttackPhaseWithSharedOrdinaryPreview(fates::runtime::native::NativeRuntime& runtime,std::span<const AiPhaseActorInput> actors,const std::uint16_t current_turn,const bool retail_enumeration_order_exact){auto provider=MakeSharedOrdinaryBattlePreviewProvider(retail_enumeration_order_exact);return ExecuteOrderedAttackPhase(runtime,actors,{current_turn,&provider});}
AiPhaseResult ExecuteOrderedAttackPhaseFromRuntimeAiWithSharedOrdinaryPreview(fates::runtime::native::NativeRuntime& runtime,std::span<const AiPhaseRuntimeActorInput> seeds,const std::uint16_t current_turn,const bool retail_enumeration_order_exact){
    const auto bound=BindPhaseActorsFromRuntimeAi(runtime,seeds);
    if(bound.status!=AiPhaseActorBindStatus::Ok) { AiPhaseResult out{};out.status=AiPhaseStatus::ActionRejected;out.actor_binding_status=bound.status;return out; }
    auto out=ExecuteOrderedAttackPhaseWithSharedOrdinaryPreview(runtime,bound.actors,current_turn,retail_enumeration_order_exact);
    out.actor_binding_status=AiPhaseActorBindStatus::Ok;return out;
}
AiPhaseResult ExecuteOrderedEverytimePhase(fates::runtime::native::NativeRuntime& runtime,std::span<const AiPhaseActorInput> actors){return ExecuteOrderedAttackPhase(runtime,actors,{0,nullptr});}
} // namespace fates::ai::native
