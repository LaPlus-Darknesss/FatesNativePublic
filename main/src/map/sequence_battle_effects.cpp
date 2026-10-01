#include "fates/map/sequence_battle.hpp"
#include "fates/detail/map_battle_resolution_runtime.hpp"
namespace map::battle_sequence_detail {
namespace { inline void E(const char* n){fates::decomp_detail::MapBattleResolutionCall(n);} }
#define ESTEP(Name) void BattleProcess::Name(){E("SequenceBattle." #Name);}
ESTEP(GaleEffect) ESTEP(PlayEffect) ESTEP(GaleReflect) ESTEP(Craftmanship)
ESTEP(FreezeEffect) ESTEP(GetRichQuick) ESTEP(PoisonEffect) ESTEP(PoisonOfWitch)
ESTEP(PoisonReflect) ESTEP(LifeAbsorption) ESTEP(WeaknessEffect) ESTEP(DrainCapability)
ESTEP(KillingInstinct) ESTEP(CraftmanshipEffect) ESTEP(GetRichQuickEffect)
ESTEP(CraftmanshipReflect) ESTEP(GetRichQuickReflect) ESTEP(PoisonOfWitchEffect)
ESTEP(LifeAbsorptionEffect) ESTEP(PoisonOfWitchReflect) ESTEP(DrainCapabilityEffect)
ESTEP(KillingInstinctEffect) ESTEP(LifeAbsorptionReflect) ESTEP(CheerAfterBattleDualDf)
ESTEP(CheerAfterBattleDualOf) ESTEP(DrainCapabilityReflect) ESTEP(KillingInstinctReflect)
ESTEP(WeaknessCalculateEffect) ESTEP(Gale) ESTEP(Freeze) ESTEP(Poison)
ESTEP(WeaknessReflect) ESTEP(HitEffect)
#undef ESTEP
void BattleProcess::GetSealEffect(int side){fates::decomp_detail::MapBattleResolutionCall("SequenceBattle.GetSealEffect",side);}
void BattleProcess::GetPoisonEffect(int side){fates::decomp_detail::MapBattleResolutionCall("SequenceBattle.GetPoisonEffect",side);}
void BattleProcess::CheerAfterBattleImpl(int side){fates::decomp_detail::MapBattleResolutionCall("SequenceBattle.CheerAfterBattleImpl",side);}
void BattleProcess::CalculateEffectHiddenOne(BattleCalculator* c,int side){fates::decomp_detail::MapBattleResolutionCall("SequenceBattle.CalculateEffectHiddenOne",c,side);}
void BattleProcess::GetPoisonEffectReverseSide(int side){fates::decomp_detail::MapBattleResolutionCall("SequenceBattle.GetPoisonEffectReverseSide",side);}
void BattleProcess::SetSeal(int side){
    // PROVEN: seal/debuff application is merged into Unit capability state using
    // the retail MergeSeal rule; exact capability fields remain owned by Unit.
    fates::decomp_detail::MapBattleResolutionCall("SequenceBattle.SetSeal",side);
}
} // namespace map::battle_sequence_detail
