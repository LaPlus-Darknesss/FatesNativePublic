#include "fates/map/mind.hpp"
#include "fates/detail/tactical_map_runtime.hpp"

namespace map {
void Mind::Initialize(){ fates::decomp_detail::TacticalMapCall("Mind.Initialize"); }
Mind* Mind::Get(){ return fates::decomp_detail::TacticalMapValue<Mind*>("Mind.Get"); }
void Mind::Finalize(){ fates::decomp_detail::TacticalMapCall("Mind.Finalize"); }
void Mind::Continuous(){ fates::decomp_detail::TacticalMapCall("Mind.Continuous", this); }
void Mind::ResetTarget(){ fates::decomp_detail::TacticalMapCall("Mind.ResetTarget", this); }
void Mind::DoubleChange(const Unit* unit){ fates::decomp_detail::TacticalMapCall("Mind.DoubleChange", this, unit); }
void Mind::Reset(const Unit* unit){ fates::decomp_detail::TacticalMapCall("Mind.Reset", this, unit); }
void Mind::ResetMind(){ fates::decomp_detail::TacticalMapCall("Mind.ResetMind", this); }
const Unit* Mind::GetTradeUnit() const { return fates::decomp_detail::TacticalMapValue<const Unit*>("Mind.GetTradeUnit", this); }
const Unit* Mind::GetTargetUnit() const { return fates::decomp_detail::TacticalMapValue<const Unit*>("Mind.GetTargetUnit", this); }
const Unit* Mind::GetDoubleTradeUnit() const { return fates::decomp_detail::TacticalMapValue<const Unit*>("Mind.GetDoubleTradeUnit", this); }
const Unit* Mind::GetUnit() const { return fates::decomp_detail::TacticalMapValue<const Unit*>("Mind.GetUnit", this); }
} // namespace map
