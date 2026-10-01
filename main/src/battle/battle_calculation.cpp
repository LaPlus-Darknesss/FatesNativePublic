#include "fates/battle/battle_calculation.hpp"
#include "fates/detail/gameplay_data_runtime.hpp"

namespace map {
BattleInfo::Side::Side() { fates::decomp_detail::GameplayDataCall("BattleInfo.Side.Construct",this); }
void BattleInfo::Side::CalculateDetail(const Side* a0){fates::decomp_detail::GameplayDataCall("BattleInfo.Side.CalculateDetail",this,a0);}
void BattleInfo::Side::CalculateEfficacy(const Side* a0){fates::decomp_detail::GameplayDataCall("BattleInfo.Side.CalculateEfficacy",this,a0);}
void BattleInfo::Side::CalculateDetailHit(const Side* a0){fates::decomp_detail::GameplayDataCall("BattleInfo.Side.CalculateDetailHit",this,a0);}
void BattleInfo::Side::CalculateDetailAvoid(const Side* a0){fates::decomp_detail::GameplayDataCall("BattleInfo.Side.CalculateDetailAvoid",this,a0);}
void BattleInfo::Side::ComplementConditions(const Flag* a0){fates::decomp_detail::GameplayDataCall("BattleInfo.Side.ComplementConditions",this,a0);}
void BattleInfo::Side::CalculateDetailAttack(const Side* a0){fates::decomp_detail::GameplayDataCall("BattleInfo.Side.CalculateDetailAttack",this,a0);}
void BattleInfo::Side::CalculateDetailDefense(const Side* a0){fates::decomp_detail::GameplayDataCall("BattleInfo.Side.CalculateDetailDefense",this,a0);}
int BattleInfo::Side::GetSimpleHit(const Side* a0) const{return fates::decomp_detail::GameplayDataValue<int>("BattleInfo.Side.GetSimpleHit",this,a0);}
int BattleInfo::Side::GetSimplePower(const Side* a0,int a1,int a2) const{return fates::decomp_detail::GameplayDataValue<int>("BattleInfo.Side.GetSimplePower",this,a0,a1,a2);}
int BattleInfo::Side::GetSimpleTimes(const Side* a0,int a1,const Flag* a2) const{return fates::decomp_detail::GameplayDataValue<int>("BattleInfo.Side.GetSimpleTimes",this,a0,a1,a2);}
void BattleInfo::CalculateDual(){fates::decomp_detail::GameplayDataCall("BattleInfo.CalculateDual",this);}
void BattleInfo::ComplementDual(){fates::decomp_detail::GameplayDataCall("BattleInfo.ComplementDual",this);}
void BattleInfo::CalculateDetail(){fates::decomp_detail::GameplayDataCall("BattleInfo.CalculateDetail",this);}
void BattleInfo::CalculateSimple(){fates::decomp_detail::GameplayDataCall("BattleInfo.CalculateSimple",this);}
void BattleInfo::ComplementConditions(){fates::decomp_detail::GameplayDataCall("BattleInfo.ComplementConditions",this);}
void BattleInfo::CalculateFaceExpression(){fates::decomp_detail::GameplayDataCall("BattleInfo.CalculateFaceExpression",this);}
void BattleInfo::Clear(){fates::decomp_detail::GameplayDataCall("BattleInfo.Clear",this);}
void BattleInfo::Calculate(){fates::decomp_detail::GameplayDataCall("BattleInfo.Calculate",this);}
BattleCalculator::Status::Status() : value(0) {}
BattleCalculator::BattleCalculator(BattleInfo* a0){fates::decomp_detail::GameplayDataCall("BattleCalculator.Construct",this,a0);}
BattleCalculator::~BattleCalculator(){
    // Retail walks/deletes owned Scene nodes here. The Scene node layout remains
    // opaque, but ownership is explicit at this source boundary.
    fates::decomp_detail::GameplayDataCall("BattleCalculator.DestroyScenes",this);
}
void BattleCalculator::CalculateRod(){fates::decomp_detail::GameplayDataCall("BattleCalculator.CalculateRod",this);}
BattleCalculator::Scene* BattleCalculator::CreateNewScene(){return fates::decomp_detail::GameplayDataValue<Scene*>("BattleCalculator.CreateNewScene",this);}
bool BattleCalculator::CalculateAttack(BattleSide::Type a0){return fates::decomp_detail::GameplayDataValue<bool>("BattleCalculator.CalculateAttack",this,a0);}
void BattleCalculator::InitializeProgress(){fates::decomp_detail::GameplayDataCall("BattleCalculator.InitializeProgress",this);}
bool BattleCalculator::CalculateAttackSingle(BattleSide::Type a0,int a1){return fates::decomp_detail::GameplayDataValue<bool>("BattleCalculator.CalculateAttackSingle",this,a0,a1);}
void BattleCalculator::Calculate(){fates::decomp_detail::GameplayDataCall("BattleCalculator.Calculate",this);}
Unit* BattleCalculator::GetDeadUnit(const Scene* a0) const{return fates::decomp_detail::GameplayDataValue<Unit*>("BattleCalculator.GetDeadUnit",this,a0);}
int BattleCalculator::GetAttackContinuousCount(const Scene* a0) const{return fates::decomp_detail::GameplayDataValue<int>("BattleCalculator.GetAttackContinuousCount",this,a0);}
}
