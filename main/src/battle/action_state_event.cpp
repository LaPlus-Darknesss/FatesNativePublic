#include "fates/battle/action_states.hpp"
#include "fates/detail/battle_action_runtime.hpp"

void ActionEvWait::Tick() { fates::decomp_detail::BattleActionCall("ActionEvWait::Tick", *this); }
void ActionPostAttackBackStep::Tick() { fates::decomp_detail::BattleActionCall("ActionPostAttackBackStep::Tick", *this); }
void ActionPostAttackBackStep::Enter() { fates::decomp_detail::BattleActionCall("ActionPostAttackBackStep::Enter", *this); }
AnimClip::Type ActionPostAttackBackStep::GetAnim() const { return fates::decomp_detail::BattleActionValue<AnimClip::Type>("ActionPostAttackBackStep::GetAnim", *this); }
void ActionEvEmote::Enter() { fates::decomp_detail::BattleActionCall("ActionEvEmote::Enter", *this); }
void ActionEvRunTo::Tick() { fates::decomp_detail::BattleActionCall("ActionEvRunTo::Tick", *this); }
void ActionEvRunTo::Enter() { fates::decomp_detail::BattleActionCall("ActionEvRunTo::Enter", *this); }
void ActionEvRunTo::Leave() { fates::decomp_detail::BattleActionCall("ActionEvRunTo::Leave", *this); }
const char* ActionEvRunTo::GetAnimName() const { return fates::decomp_detail::BattleActionValue<const char*>("ActionEvRunTo::GetAnimName", *this); }
float ActionEvRunTo::GetTurnRate() const { return fates::decomp_detail::BattleActionValue<float>("ActionEvRunTo::GetTurnRate", *this); }
float ActionEvRunTo::GetDefaultAcc() const { return fates::decomp_detail::BattleActionValue<float>("ActionEvRunTo::GetDefaultAcc", *this); }
bool ActionEvRunTo::IsAllowedToEnd() const { return fates::decomp_detail::BattleActionValue<bool>("ActionEvRunTo::IsAllowedToEnd", *this); }
bool ActionEvRunTo::RemainAtTalkEnd() const { return fates::decomp_detail::BattleActionValue<bool>("ActionEvRunTo::RemainAtTalkEnd", *this); }
float ActionEvRunTo::GetDefaultMaxVel() const { return fates::decomp_detail::BattleActionValue<float>("ActionEvRunTo::GetDefaultMaxVel", *this); }
void ActionEvGazeAt::Enter() { fates::decomp_detail::BattleActionCall("ActionEvGazeAt::Enter", *this); }
ActionEvGazeAt::ActionEvGazeAt(BattleUnit* a0, BattleUnit* a1) { fates::decomp_detail::BattleActionCall("ActionEvGazeAt::ActionEvGazeAt", *this, a0, a1); }
void ActionEvMotion::Tick() { fates::decomp_detail::BattleActionCall("ActionEvMotion::Tick", *this); }
void ActionEvMotion::Enter() { fates::decomp_detail::BattleActionCall("ActionEvMotion::Enter", *this); }
bool ActionEvMotion::RemainAtTalkEnd() const { return fates::decomp_detail::BattleActionValue<bool>("ActionEvMotion::RemainAtTalkEnd", *this); }
void ActionEvTurnTo::Tick() { fates::decomp_detail::BattleActionCall("ActionEvTurnTo::Tick", *this); }
void ActionEvTurnTo::Enter() { fates::decomp_detail::BattleActionCall("ActionEvTurnTo::Enter", *this); }
bool ActionEvTurnTo::RemainAtTalkEnd() const { return fates::decomp_detail::BattleActionValue<bool>("ActionEvTurnTo::RemainAtTalkEnd", *this); }
void ActionBattleToEv::Tick() { fates::decomp_detail::BattleActionCall("ActionBattleToEv::Tick", *this); }
void ActionBattleToEv::Enter() { fates::decomp_detail::BattleActionCall("ActionBattleToEv::Enter", *this); }
void ActionWaitDeform::Tick() { fates::decomp_detail::BattleActionCall("ActionWaitDeform::Tick", *this); }
void ActionEvFrameShift::Enter() { fates::decomp_detail::BattleActionCall("ActionEvFrameShift::Enter", *this); }
void ActionEvGazeLocked::Enter() { fates::decomp_detail::BattleActionCall("ActionEvGazeLocked::Enter", *this); }
void ActionShadowAttack::Tick() { fates::decomp_detail::BattleActionCall("ActionShadowAttack::Tick", *this); }
void ActionShadowAttack::Enter() { fates::decomp_detail::BattleActionCall("ActionShadowAttack::Enter", *this); }
ActionShadowAttack::ActionShadowAttack(BattleUnit* a0, AnimClip::Type a1) { fates::decomp_detail::BattleActionCall("ActionShadowAttack::ActionShadowAttack", *this, a0, a1); }
void ActionEvMotionDirectEnd::Enter() { fates::decomp_detail::BattleActionCall("ActionEvMotionDirectEnd::Enter", *this); }
bool ActionEvBase::DoUpdateDir() const { return fates::decomp_detail::BattleActionValue<bool>("ActionEvBase::DoUpdateDir", *this); }
bool ActionEvBase::RemainAtTalkEnd() const { return fates::decomp_detail::BattleActionValue<bool>("ActionEvBase::RemainAtTalkEnd", *this); }
const char* ActionEvWalkTo::GetAnimName() const { return fates::decomp_detail::BattleActionValue<const char*>("ActionEvWalkTo::GetAnimName", *this); }
float ActionEvWalkTo::GetTurnRate() const { return fates::decomp_detail::BattleActionValue<float>("ActionEvWalkTo::GetTurnRate", *this); }
float ActionEvWalkTo::GetDefaultAcc() const { return fates::decomp_detail::BattleActionValue<float>("ActionEvWalkTo::GetDefaultAcc", *this); }
bool ActionEvWalkTo::IsAllowedToEnd() const { return fates::decomp_detail::BattleActionValue<bool>("ActionEvWalkTo::IsAllowedToEnd", *this); }
float ActionEvWalkTo::GetDefaultMaxVel() const { return fates::decomp_detail::BattleActionValue<float>("ActionEvWalkTo::GetDefaultMaxVel", *this); }
