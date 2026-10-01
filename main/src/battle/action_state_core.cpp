#include "fates/battle/action_states.hpp"
#include "fates/detail/battle_action_runtime.hpp"

void ActionSync::Tick() { fates::decomp_detail::BattleActionCall("ActionSync::Tick", *this); }
void ActionSync::Enter() { fates::decomp_detail::BattleActionCall("ActionSync::Enter", *this); }
void ActionDance::Tick() { fates::decomp_detail::BattleActionCall("ActionDance::Tick", *this); }
void ActionDance::Enter() { fates::decomp_detail::BattleActionCall("ActionDance::Enter", *this); }
AnimClip::Type ActionDance::GetAnim() const { return fates::decomp_detail::BattleActionValue<AnimClip::Type>("ActionDance::GetAnim", *this); }
void ActionRound::Enter() { fates::decomp_detail::BattleActionCall("ActionRound::Enter", *this); }
void ActionSkill::Tick() { fates::decomp_detail::BattleActionCall("ActionSkill::Tick", *this); }
ActionSkill::ActionSkill(BattleUnit* a0, const BattlePhase* a1) { fates::decomp_detail::BattleActionCall("ActionSkill::ActionSkill", *this, a0, a1); }
AnimClip::Type ActionSkill::GetAnim() const { return fates::decomp_detail::BattleActionValue<AnimClip::Type>("ActionSkill::GetAnim", *this); }
void ActionAttack::RealImpact() { fates::decomp_detail::BattleActionCall("ActionAttack::RealImpact", *this); }
void ActionAttack::DummyImpact() { fates::decomp_detail::BattleActionCall("ActionAttack::DummyImpact", *this); }
void ActionAttack::ImpactCommon(AnimClip::Type a0) { fates::decomp_detail::BattleActionCall("ActionAttack::ImpactCommon", *this, a0); }
AnimClip::Type ActionAttack::SuggestProperAttack(const BattleUnit* a0, BattlePhase* a1) { return fates::decomp_detail::BattleActionValue<AnimClip::Type>("ActionAttack::SuggestProperAttack", *this, a0, a1); }
void ActionAttack::Tick() { fates::decomp_detail::BattleActionCall("ActionAttack::Tick", *this); }
void ActionAttack::Enter() { fates::decomp_detail::BattleActionCall("ActionAttack::Enter", *this); }
void ActionAttack::Leave() { fates::decomp_detail::BattleActionCall("ActionAttack::Leave", *this); }
bool ActionAttack::IsAttackState() const { return fates::decomp_detail::BattleActionValue<bool>("ActionAttack::IsAttackState", *this); }
AnimClip::Type ActionAttack::GetAnim() const { return fates::decomp_detail::BattleActionValue<AnimClip::Type>("ActionAttack::GetAnim", *this); }
void ActionDeform::Tick() { fates::decomp_detail::BattleActionCall("ActionDeform::Tick", *this); }
void ActionDeform::Enter() { fates::decomp_detail::BattleActionCall("ActionDeform::Enter", *this); }
bool IActionState::IsAnimFinished() { return fates::decomp_detail::BattleActionValue<bool>("IActionState::IsAnimFinished", *this); }
void IActionState::Tick() { fates::decomp_detail::BattleActionCall("IActionState::Tick", *this); }
void IActionState::Enter() { fates::decomp_detail::BattleActionCall("IActionState::Enter", *this); }
IActionState::IActionState(BattleUnit* a0, const BattlePhase* a1) { fates::decomp_detail::BattleActionCall("IActionState::IActionState", *this, a0, a1); }
bool IActionState::DoUpdateDir() const { return fates::decomp_detail::BattleActionValue<bool>("IActionState::DoUpdateDir", *this); }
bool IActionState::IsAttackState() const { return fates::decomp_detail::BattleActionValue<bool>("IActionState::IsAttackState", *this); }
bool IActionState::IsAllowedToEnd() const { return fates::decomp_detail::BattleActionValue<bool>("IActionState::IsAllowedToEnd", *this); }
AnimClip::Type IActionState::GetAnim() const { return fates::decomp_detail::BattleActionValue<AnimClip::Type>("IActionState::GetAnim", *this); }
void ActionStealth::Enter() { fates::decomp_detail::BattleActionCall("ActionStealth::Enter", *this); }
void ActionApproach::Tick() { fates::decomp_detail::BattleActionCall("ActionApproach::Tick", *this); }
void ActionApproach::Enter() { fates::decomp_detail::BattleActionCall("ActionApproach::Enter", *this); }
void ActionGreeting::Tick() { fates::decomp_detail::BattleActionCall("ActionGreeting::Tick", *this); }
void ActionGreeting::Enter() { fates::decomp_detail::BattleActionCall("ActionGreeting::Enter", *this); }
ActionGreeting::ActionGreeting(BattleUnit* a0, const BattlePhase* a1) { fates::decomp_detail::BattleActionCall("ActionGreeting::ActionGreeting", *this, a0, a1); }
AnimClip::Type ActionGreeting::GetAnim() const { return fates::decomp_detail::BattleActionValue<AnimClip::Type>("ActionGreeting::GetAnim", *this); }
void ActionStateMachine::Tick() { fates::decomp_detail::BattleActionCall("ActionStateMachine::Tick", *this); }
void ActionStateMachine::Clear() { fates::decomp_detail::BattleActionCall("ActionStateMachine::Clear", *this); }
void ActionCounterAttack::Tick() { fates::decomp_detail::BattleActionCall("ActionCounterAttack::Tick", *this); }
void ActionCounterAttack::Enter() { fates::decomp_detail::BattleActionCall("ActionCounterAttack::Enter", *this); }
void ActionAny::Tick() { fates::decomp_detail::BattleActionCall("ActionAny::Tick", *this); }
AnimClip::Type ActionAny::GetAnim() const { return fates::decomp_detail::BattleActionValue<AnimClip::Type>("ActionAny::GetAnim", *this); }
void ActionRod::Tick() { fates::decomp_detail::BattleActionCall("ActionRod::Tick", *this); }
ActionRod::ActionRod(BattleUnit* a0, const BattlePhase* a1) { fates::decomp_detail::BattleActionCall("ActionRod::ActionRod", *this, a0, a1); }
AnimClip::Type ActionRod::GetAnim() const { return fates::decomp_detail::BattleActionValue<AnimClip::Type>("ActionRod::GetAnim", *this); }
void ActionWin::Tick() { fates::decomp_detail::BattleActionCall("ActionWin::Tick", *this); }
void ActionWin::Enter() { fates::decomp_detail::BattleActionCall("ActionWin::Enter", *this); }
AnimClip::Type ActionWin::GetAnim() const { return fates::decomp_detail::BattleActionValue<AnimClip::Type>("ActionWin::GetAnim", *this); }
