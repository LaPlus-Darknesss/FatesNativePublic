#include "fates/battle/action_states.hpp"
#include "fates/detail/battle_action_runtime.hpp"

void ActionDualRun::Tick() { fates::decomp_detail::BattleActionCall("ActionDualRun::Tick", *this); }
bool ActionDualRun::DoUpdateDir() const { return fates::decomp_detail::BattleActionValue<bool>("ActionDualRun::DoUpdateDir", *this); }
AnimClip::Type ActionDualRun::GetAnim() const { return fates::decomp_detail::BattleActionValue<AnimClip::Type>("ActionDualRun::GetAnim", *this); }
void ActionDualGuard::Tick() { fates::decomp_detail::BattleActionCall("ActionDualGuard::Tick", *this); }
void ActionDualGuard::Enter() { fates::decomp_detail::BattleActionCall("ActionDualGuard::Enter", *this); }
void ActionDualGuard::Leave() { fates::decomp_detail::BattleActionCall("ActionDualGuard::Leave", *this); }
ActionDualGuard::ActionDualGuard(BattleUnit* a0, const BattlePhase* a1) { fates::decomp_detail::BattleActionCall("ActionDualGuard::ActionDualGuard", *this, a0, a1); }
void ActionDualSupport::Enter() { fates::decomp_detail::BattleActionCall("ActionDualSupport::Enter", *this); }
void ActionDualSupport::Leave() { fates::decomp_detail::BattleActionCall("ActionDualSupport::Leave", *this); }
AnimClip::Type ActionDualSupport::GetAnim() const { return fates::decomp_detail::BattleActionValue<AnimClip::Type>("ActionDualSupport::GetAnim", *this); }
void ActionDualWaiting::Tick() { fates::decomp_detail::BattleActionCall("ActionDualWaiting::Tick", *this); }
void ActionDualWaiting::Enter() { fates::decomp_detail::BattleActionCall("ActionDualWaiting::Enter", *this); }
bool ActionDualWaiting::IsAllowedToEnd() const { return fates::decomp_detail::BattleActionValue<bool>("ActionDualWaiting::IsAllowedToEnd", *this); }
AnimClip::Type ActionDualWaiting::GetAnim() const { return fates::decomp_detail::BattleActionValue<AnimClip::Type>("ActionDualWaiting::GetAnim", *this); }
void ActionDualBackStep::Enter() { fates::decomp_detail::BattleActionCall("ActionDualBackStep::Enter", *this); }
bool ActionDualBackStep::DoUpdateDir() const { return fates::decomp_detail::BattleActionValue<bool>("ActionDualBackStep::DoUpdateDir", *this); }
AnimClip::Type ActionDualBackStep::GetAnim() const { return fates::decomp_detail::BattleActionValue<AnimClip::Type>("ActionDualBackStep::GetAnim", *this); }
void ActionDualChildEscape::Tick() { fates::decomp_detail::BattleActionCall("ActionDualChildEscape::Tick", *this); }
void ActionDualChildEscape::Enter() { fates::decomp_detail::BattleActionCall("ActionDualChildEscape::Enter", *this); }
void ActionDualAttackRequest::Tick() { fates::decomp_detail::BattleActionCall("ActionDualAttackRequest::Tick", *this); }
void ActionDualAttackRequest::Enter() { fates::decomp_detail::BattleActionCall("ActionDualAttackRequest::Enter", *this); }
