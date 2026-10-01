#pragma once
#include "fates/ai/native_ai_action.hpp"
#include "fates/ai/native_ai_dual_planner.hpp"
namespace fates::ai::native {
// Read-only mechanical projection, NOT action authorization. The caller must
// apply the policy/Power0/weapon-order contract before accepting a candidate.
AiCandidatePreviewStatus ProjectSharedOrdinaryBattleCandidate(
    const fates::runtime::native::NativeRuntime&, const AiCandidatePreviewRequest&,
    AiOrdinaryTacticalCandidateInput&, fates::battle::native::BattlePreviewResult* = nullptr);
// Native mechanical service for AIDual. Unknown/unsupported stays nullopt;
// a proved zero-count source returns real zero facts. Does not queue Mind.
std::optional<AiDualBattleFacts> ProjectSharedDualBattleFacts(
    const fates::runtime::native::NativeRuntime&,const AiDualPreviewRequest&,
    fates::battle::native::BattlePreviewResult* = nullptr);
// Shared ordinary candidate preview backed by the same native battle preview used
// by ExecuteOrdinaryBattle. The provider itself never consumes battle RNG or mutates state.
AiCandidatePreviewProvider MakeSharedOrdinaryBattlePreviewProvider(
    bool retail_enumeration_order_exact=false) noexcept;
} // namespace fates::ai::native
