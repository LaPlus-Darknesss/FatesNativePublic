#include "fates/battle/battle_runtime.hpp"
#include "fates/detail/battle_runtime.hpp"

bool BattleController::IsBlurring(){return fates::decomp_detail::BattleControllerIsBlurring();}
bool BattleController::DetectRunning(){return fates::decomp_detail::BattleControllerDetectRunning();}
void BattleController::Create(ProcInst* a0, map::BattleCalculator* a1){fates::decomp_detail::BattleControllerCreate(a0, a1);}
void BattleController::Wakeup(){fates::decomp_detail::BattleControllerWakeup();}
ProcInst* BattleController::GetProc(){return fates::decomp_detail::BattleControllerGetProc();}
bool BattleController::IsRunning(){return fates::decomp_detail::BattleControllerIsRunning();}
