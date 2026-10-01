#include "fates/battle/battle_runtime.hpp"
#include "fates/detail/battle_runtime.hpp"

void BattleWorld::Draw(){fates::decomp_detail::BattleWorldDraw(*this);}
void BattleWorld::Tick(){fates::decomp_detail::BattleWorldTick(*this);}
void BattleWorld::Initialize(){fates::decomp_detail::BattleWorldInitialize();}
void BattleWorld::OnPostDraw(){fates::decomp_detail::BattleWorldOnPostDraw(*this);}
BattleUnit* BattleWorld::CreateBattleUnit(int a0){return fates::decomp_detail::BattleWorldCreateBattleUnit(*this, a0);}
void BattleWorld::DeleteBattleUnit(int a0){fates::decomp_detail::BattleWorldDeleteBattleUnit(*this, a0);}
void BattleWorld::DeleteDisplayObjects(){fates::decomp_detail::BattleWorldDeleteDisplayObjects(*this);}
BattleWorld* BattleWorld::Get(){return fates::decomp_detail::BattleWorldGet();}
void BattleWorld::Finalize(){fates::decomp_detail::BattleWorldFinalize();}
void BattleWorld::OnPreDraw(){fates::decomp_detail::BattleWorldOnPreDraw(*this);}
BattleWorld::BattleWorld(){fates::decomp_detail::BattleWorldBattleWorld(*this);}
BattleWorld::~BattleWorld(){fates::decomp_detail::BattleWorldBattleWorld(*this);}
void BattleWorld::OnPostTick(){fates::decomp_detail::BattleWorldOnPostTick(*this);}
void BattleWorld::OnPreTick(){fates::decomp_detail::BattleWorldOnPreTick(*this);}
