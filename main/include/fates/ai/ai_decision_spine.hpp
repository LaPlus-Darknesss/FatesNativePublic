#pragma once
#include <cstdint>

namespace fates::ai::reconstruction {

enum class RetailOperation : std::uint8_t {
    AIOrder_AttackHigh, AIOrder_Processing, AIOrder_CheerNoMove, AIOrder_EntrustHeal,
    AIOrder_EntrustMove, AIOrder_EnumerateAttack, AIOrder_SortDescend, AIOrder_AttackMiddle,
    AIOrder_UpdateTarget, AIOrder_EntrustAttack, AIOrder_EnumerateMove, AIOrder_AttackCrossfire,
    AIOrder_AttackLongRange, AIOrder_EnumerateEntrust, AIOrder_EntrustUpdateIdle,
    AIOrder_AttackInterference, AIOrder_EntrustHeroRushMove, AIOrder_CheerInsteadOfAttack,
    AIOrder_EnumerateAttackLongRange, AIOrder_EnumerateAttackInterference, AIOrder_Mind,
    AIOrder_Move, AIOrder_Next, AIOrder_Cause, AIOrder_GetUnit, AIOrder_TurnEnd, AIOrder_Priority,
    AIOrder_AllowIdle, AIOrder_AttackLow, AIOrder_CheerMove, AIOrder_GaleFixed, AIOrder_Construct,
    AIThink_HasHealRod, AIThink_IsMoveNear, AIThink_IsMoveOver, AIThink_IsMoveOver2,
    AIThink_Processing, AIThink_GetDualScore, AIThink_UpdateTarget, AIThink_GetAttackRange,
    AIThink_GetAttackScore, AIThink_GetDestroyScore, AIThink_GetHealRodRange,
    AIThink_GetSidePosition, AIThink_GetTerrainScore, AIThink_IsActiveCommand,
    AIThink_UpdateTargetOne, AIThink_GetItemIndexHeal, AIThink_GetMovePowerSlow,
    AIThink_HasActiveCommand, AIThink_IsEscapePosition, AIThink_ProcessingActive,
    AIThink_ProcessingResult, AIThink_GetAttackPosition, AIThink_IsDualSupportUnit,
    AIThink_ProcessingEntrust, AIThink_GetDestroyPosition, AIThink_GetHealRodPosition,
    AIThink_IsAttackPermission, AIThink_IsAttackPermission2, AIThink_Action, AIThink_MoveTo,
    AIThink_Update, AIThink_AttackTo
};

struct RuntimeBoundary {
    virtual ~RuntimeBoundary() = default;
    virtual void Invoke(RetailOperation operation) = 0;
};

// Exact retail symbol identity lives in the canonical registry/evidence. These readable
// reconstructed entry points deliberately avoid publishing guessed retail object layouts.
void AIOrder_AttackHigh(RuntimeBoundary& boundary);
void AIOrder_Processing(RuntimeBoundary& boundary);
void AIOrder_CheerNoMove(RuntimeBoundary& boundary);
void AIOrder_EntrustHeal(RuntimeBoundary& boundary);
void AIOrder_EntrustMove(RuntimeBoundary& boundary);
void AIOrder_EnumerateAttack(RuntimeBoundary& boundary);
void AIOrder_SortDescend(RuntimeBoundary& boundary);
void AIOrder_AttackMiddle(RuntimeBoundary& boundary);
void AIOrder_UpdateTarget(RuntimeBoundary& boundary);
void AIOrder_EntrustAttack(RuntimeBoundary& boundary);
void AIOrder_EnumerateMove(RuntimeBoundary& boundary);
void AIOrder_AttackCrossfire(RuntimeBoundary& boundary);
void AIOrder_AttackLongRange(RuntimeBoundary& boundary);
void AIOrder_EnumerateEntrust(RuntimeBoundary& boundary);
void AIOrder_EntrustUpdateIdle(RuntimeBoundary& boundary);
void AIOrder_AttackInterference(RuntimeBoundary& boundary);
void AIOrder_EntrustHeroRushMove(RuntimeBoundary& boundary);
void AIOrder_CheerInsteadOfAttack(RuntimeBoundary& boundary);
void AIOrder_EnumerateAttackLongRange(RuntimeBoundary& boundary);
void AIOrder_EnumerateAttackInterference(RuntimeBoundary& boundary);
void AIOrder_Mind(RuntimeBoundary& boundary);
void AIOrder_Move(RuntimeBoundary& boundary);
void AIOrder_Next(RuntimeBoundary& boundary);
void AIOrder_Cause(RuntimeBoundary& boundary);
void AIOrder_GetUnit(RuntimeBoundary& boundary);
void AIOrder_TurnEnd(RuntimeBoundary& boundary);
void AIOrder_Priority(RuntimeBoundary& boundary);
void AIOrder_AllowIdle(RuntimeBoundary& boundary);
void AIOrder_AttackLow(RuntimeBoundary& boundary);
void AIOrder_CheerMove(RuntimeBoundary& boundary);
void AIOrder_GaleFixed(RuntimeBoundary& boundary);
void AIOrder_Construct(RuntimeBoundary& boundary);
void AIThink_HasHealRod(RuntimeBoundary& boundary);
void AIThink_IsMoveNear(RuntimeBoundary& boundary);
void AIThink_IsMoveOver(RuntimeBoundary& boundary);
void AIThink_IsMoveOver2(RuntimeBoundary& boundary);
void AIThink_Processing(RuntimeBoundary& boundary);
void AIThink_GetDualScore(RuntimeBoundary& boundary);
void AIThink_UpdateTarget(RuntimeBoundary& boundary);
void AIThink_GetAttackRange(RuntimeBoundary& boundary);
void AIThink_GetAttackScore(RuntimeBoundary& boundary);
void AIThink_GetDestroyScore(RuntimeBoundary& boundary);
void AIThink_GetHealRodRange(RuntimeBoundary& boundary);
void AIThink_GetSidePosition(RuntimeBoundary& boundary);
void AIThink_GetTerrainScore(RuntimeBoundary& boundary);
void AIThink_IsActiveCommand(RuntimeBoundary& boundary);
void AIThink_UpdateTargetOne(RuntimeBoundary& boundary);
void AIThink_GetItemIndexHeal(RuntimeBoundary& boundary);
void AIThink_GetMovePowerSlow(RuntimeBoundary& boundary);
void AIThink_HasActiveCommand(RuntimeBoundary& boundary);
void AIThink_IsEscapePosition(RuntimeBoundary& boundary);
void AIThink_ProcessingActive(RuntimeBoundary& boundary);
void AIThink_ProcessingResult(RuntimeBoundary& boundary);
void AIThink_GetAttackPosition(RuntimeBoundary& boundary);
void AIThink_IsDualSupportUnit(RuntimeBoundary& boundary);
void AIThink_ProcessingEntrust(RuntimeBoundary& boundary);
void AIThink_GetDestroyPosition(RuntimeBoundary& boundary);
void AIThink_GetHealRodPosition(RuntimeBoundary& boundary);
void AIThink_IsAttackPermission(RuntimeBoundary& boundary);
void AIThink_IsAttackPermission2(RuntimeBoundary& boundary);
void AIThink_Action(RuntimeBoundary& boundary);
void AIThink_MoveTo(RuntimeBoundary& boundary);
void AIThink_Update(RuntimeBoundary& boundary);
void AIThink_AttackTo(RuntimeBoundary& boundary);

} // namespace fates::ai::reconstruction
