#include "fates/map/sequence_battle.hpp"
#include "fates/detail/map_battle_resolution_runtime.hpp"
namespace map::battle_sequence_detail {
namespace { inline void P(const char* n){fates::decomp_detail::MapBattleResolutionCall(n);} }
void BattleProcess::AddReliance(Unit* a,Unit* b,int points){
    // PROVEN: delegates the actual chapter Support/Reliance gain to Unit and,
    // when a positive gain occurs, records each participant once in the local
    // battle-effect list while capacity remains.
    fates::decomp_detail::MapBattleResolutionCall("SequenceBattle.AddReliance",a,b,points);
}
void BattleProcess::AfterBattle(){
    // PROVEN: marks the battle post-action state, applies a retail equipped-skill
    // consequence to the opposing side, and can jump to the post-battle effect branch.
    P("SequenceBattle.AfterBattle");
}
void BattleProcess::ItemExpend(){
    // PROVEN: durability/consumption is resolved after the action using the
    // equipped item and retail skill exceptions. Item storage stays Unit-owned.
    P("SequenceBattle.ItemExpend");
}
void BattleProcess::ItemDrop(){
    // PROVEN: selects the winner/loser, consumes the next eligible dropped
    // inventory slot from the loser, then routes acquisition through visible
    // ItemGain or silent acquisition depending on the winner's force policy.
    P("SequenceBattle.ItemDrop");
}
void BattleProcess::RelianceEffect(){P("SequenceBattle.RelianceEffect");}
void BattleProcess::HpChangeAfterBattle(){P("SequenceBattle.HpChangeAfterBattle");}
void BattleProcess::CheerAfterBattleOf(){P("SequenceBattle.CheerAfterBattleOf");}
void BattleProcess::CheerAfterBattleDf(){P("SequenceBattle.CheerAfterBattleDf");}
void BattleProcess::EnhanceAfterBattle(){P("SequenceBattle.EnhanceAfterBattle");}
void BattleProcess::EnhanceAfterBattleImpl(bool partner){
    // PROVEN: iterates surviving battle sides and applies post-battle Unit
    // enhancement to the actor and, when present/eligible, its paired partner.
    fates::decomp_detail::MapBattleResolutionCall("SequenceBattle.EnhanceAfterBattleImpl",partner);
}
void BattleProcess::EnhanceAfterBattleOne(Unit* u,const unit::Item* item,int side,bool actor,bool partner){fates::decomp_detail::MapBattleResolutionCall("SequenceBattle.EnhanceAfterBattleOne",u,item,side,actor,partner);}
void BattleProcess::EnhanceAfterBattleCalculateEffect(){P("SequenceBattle.EnhanceAfterBattleCalculateEffect");}
void BattleProcess::Grow(){
    // Tier-A formula boundary: growth/EXP/stat outcomes are already resolved by
    // Unit/BattleCalculator helpers. Preserve integer/RNG ordering there rather
    // than duplicating a guessed formula inside the sequence layer.
    P("SequenceBattle.Grow");
}
} // namespace map::battle_sequence_detail
