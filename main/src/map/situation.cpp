#include "fates/map/situation.hpp"
#include "fates/detail/tactical_map_runtime.hpp"

namespace map {
void Situation::Initialize(){ fates::decomp_detail::TacticalMapCall("Situation.Initialize"); }
void Situation::Finalize(){ fates::decomp_detail::TacticalMapCall("Situation.Finalize"); }
void Situation::Deserialize(Stream* s){ fates::decomp_detail::TacticalMapCall("Situation.Deserialize",this,s); }
void Situation::Serialize(Stream* s) const { fates::decomp_detail::TacticalMapCall("Situation.Serialize",this,s); }
void Situation::SetComplete(WinLoseResult::Type r){ fates::decomp_detail::TacticalMapCall("Situation.SetComplete",this,r); }
void Situation::SetGameOver(WinLoseResult::Type r){ fates::decomp_detail::TacticalMapCall("Situation.SetGameOver",this,r); }
void Situation::GameEndCheck(){ fates::decomp_detail::TacticalMapCall("Situation.GameEndCheck",this); }
void Situation::ResetRestTime(){ fates::decomp_detail::TacticalMapCall("Situation.ResetRestTime",this); }
void Situation::SetHumanForceFirst(){ fates::decomp_detail::TacticalMapCall("Situation.SetHumanForceFirst",this); }
void Situation::GameEndCheckUnitDead(const Unit* u){ fates::decomp_detail::TacticalMapCall("Situation.GameEndCheckUnitDead",this,u); }
void Situation::UpdateValidLinkExchange(){ fates::decomp_detail::TacticalMapCall("Situation.UpdateValidLinkExchange",this); }
void Situation::CalculateCastleEnemyBattleScore(){ fates::decomp_detail::TacticalMapCall("Situation.CalculateCastleEnemyBattleScore",this); }
void Situation::TurnEnd(){ fates::decomp_detail::TacticalMapCall("Situation.TurnEnd",this); }

bool Situation::CanGainExp(Force::Type f) const {
    // PROVEN: force values outside the retail first-three range fail; an
    // additional situation-state byte can suppress gain. Exact field ownership
    // remains opaque, so host storage is not invented here.
    return fates::decomp_detail::TacticalMapValue<bool>("Situation.CanGainExp",this,f);
}
bool Situation::IsComplete() const { return fates::decomp_detail::TacticalMapValue<bool>("Situation.IsComplete",this); }
bool Situation::IsGameOver() const { return fates::decomp_detail::TacticalMapValue<bool>("Situation.IsGameOver",this); }
bool Situation::IsShowTurn() const { return fates::decomp_detail::TacticalMapValue<bool>("Situation.IsShowTurn",this); }
int Situation::GetRestTime() const {
    // PROVEN: retail computes a remaining-frame value with a saturating zero
    // floor from stored duration/start-time state.
    return fates::decomp_detail::TacticalMapValue<int>("Situation.GetRestTime",this);
}
bool Situation::IsEntrustAI() const { return fates::decomp_detail::TacticalMapValue<bool>("Situation.IsEntrustAI",this); }
bool Situation::IsRecordDead(Force::Type f) const { return fates::decomp_detail::TacticalMapValue<bool>("Situation.IsRecordDead",this,f); }
bool Situation::IsRecordKill(Force::Type f) const { return fates::decomp_detail::TacticalMapValue<bool>("Situation.IsRecordKill",this,f); }
bool Situation::CanGainReliance(Force::Type f) const {
    // CORROBORATED naming: Reliance is the internal Support system. Retail
    // policy is kept exact behind the adapter rather than re-expressed from
    // player-facing support rules.
    return fates::decomp_detail::TacticalMapValue<bool>("Situation.CanGainReliance",this,f);
}
bool Situation::IsErrorOperation() const { return fates::decomp_detail::TacticalMapValue<bool>("Situation.IsErrorOperation",this); }
bool Situation::IsCancelOperation() const { return fates::decomp_detail::TacticalMapValue<bool>("Situation.IsCancelOperation",this); }
int Situation::GetMessageWaitFrame() const {
    // PROVEN: versus configuration changes the retail wait to 300 frames;
    // otherwise it is zero.
    return fates::decomp_detail::TacticalMapValue<int>("Situation.GetMessageWaitFrame",this);
}
bool Situation::IsValidLinkExchange() const { return fates::decomp_detail::TacticalMapValue<bool>("Situation.IsValidLinkExchange",this); }
int Situation::CalculateDoragonVein() const { return fates::decomp_detail::TacticalMapValue<int>("Situation.CalculateDoragonVein",this); }
bool Situation::CanDual(Force::Type f) const { return fates::decomp_detail::TacticalMapValue<bool>("Situation.CanDual",this,f); }
bool Situation::IsCasual(bool includeMapRestriction) const { return fates::decomp_detail::TacticalMapValue<bool>("Situation.IsCasual",this,includeMapRestriction); }
bool Situation::IsVersus() const {
    // PROVEN semantic rule: retail returns whether versus::Config exists.
    return fates::decomp_detail::TacticalMapValue<bool>("Situation.IsVersus",this);
}
bool Situation::IsPhoenix() const { return fates::decomp_detail::TacticalMapValue<bool>("Situation.IsPhoenix",this); }
} // namespace map
