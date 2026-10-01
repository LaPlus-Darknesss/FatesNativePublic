#include "fates/battle/battle_runtime.hpp"
#include "fates/detail/battle_runtime.hpp"

void BattleUtil::SuspendMap(){fates::decomp_detail::BattleUtilSuspendMap();}
int BattleUtil::GetBossType(){return fates::decomp_detail::BattleUtilGetBossType();}
void BattleUtil::HideMapIcon(){fates::decomp_detail::BattleUtilHideMapIcon();}
void BattleUtil::HideMapUnit(){fates::decomp_detail::BattleUtilHideMapUnit();}
void BattleUtil::ShowMapIcon(){fates::decomp_detail::BattleUtilShowMapIcon();}
void BattleUtil::ShowMapUnit(){fates::decomp_detail::BattleUtilShowMapUnit();}
bool BattleUtil::IsBossBattle(){return fates::decomp_detail::BattleUtilIsBossBattle();}
bool BattleUtil::IsBossUsable(){return fates::decomp_detail::BattleUtilIsBossUsable();}
nn::math::VEC3 BattleUtil::GetZoomTarget(const nn::math::VEC3& a0){return fates::decomp_detail::BattleUtilGetZoomTarget(a0);}
void BattleUtil::ResetUnitIcon(bool a0){fates::decomp_detail::BattleUtilResetUnitIcon(a0);}
void BattleUtil::BattleFinalize(){fates::decomp_detail::BattleUtilBattleFinalize();}
bool BattleUtil::IsChangeBattle(){return fates::decomp_detail::BattleUtilIsChangeBattle();}
void BattleUtil::OpenBattleArea(const Unit* a0){fates::decomp_detail::BattleUtilOpenBattleArea(a0);}
int BattleUtil::GetBossUnitSide(){return fates::decomp_detail::BattleUtilGetBossUnitSide();}
void BattleUtil::ComebackUnitIcon(float a0, float a1){fates::decomp_detail::BattleUtilComebackUnitIcon(a0, a1);}
void BattleUtil::UpdateBattleArea(){fates::decomp_detail::BattleUtilUpdateBattleArea();}
void BattleUtil::DeleteBattleUnits(){fates::decomp_detail::BattleUtilDeleteBattleUnits();}
void BattleUtil::EncounterUnitIcon(float a0, float a1){fates::decomp_detail::BattleUtilEncounterUnitIcon(a0, a1);}
void BattleUtil::ResetBattleStance(){fates::decomp_detail::BattleUtilResetBattleStance();}
int BattleUtil::GetCurrentUnitSide(){return fates::decomp_detail::BattleUtilGetCurrentUnitSide();}
void BattleUtil::ResetDistanceStance(float a0){fates::decomp_detail::BattleUtilResetDistanceStance(a0);}
bool BattleUtil::IsLoadingBattleUnits(){return fates::decomp_detail::BattleUtilIsLoadingBattleUnits();}
void BattleUtil::BindMap(){fates::decomp_detail::BattleUtilBindMap();}
void BattleUtil::ResumeMap(){fates::decomp_detail::BattleUtilResumeMap();}
void BattleUtil::UnbindMap(){fates::decomp_detail::BattleUtilUnbindMap();}
void BattleUtil::BattleInitialize(){fates::decomp_detail::BattleUtilBattleInitialize();}
void BattleUtil::ResetBattleArea(){fates::decomp_detail::BattleUtilResetBattleArea();}
