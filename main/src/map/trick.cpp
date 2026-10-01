#include "fates/map/trick.hpp"
#include "fates/detail/tactical_map_runtime.hpp"
namespace map {
void Trick::Initialize(){ fates::decomp_detail::TacticalMapCall("Trick.Initialize"); }
void Trick::Deserialize(Stream* s){ fates::decomp_detail::TacticalMapCall("Trick.Deserialize",s); }
Trick* Trick::Get(){ return fates::decomp_detail::TacticalMapValue<Trick*>("Trick.Get"); }
void Trick::Setup(){ fates::decomp_detail::TacticalMapCall("Trick.Setup",this); }
void Trick::Regist(bool recreate){ fates::decomp_detail::TacticalMapCall("Trick.Regist",this,recreate); }
void Trick::Cleanup(){ fates::decomp_detail::TacticalMapCall("Trick.Cleanup",this); }
void Trick::Finalize(){ fates::decomp_detail::TacticalMapCall("Trick.Finalize"); }
void Trick::Unregist(){ fates::decomp_detail::TacticalMapCall("Trick.Unregist",this); }
void Trick::Serialize(Stream* s) const { fates::decomp_detail::TacticalMapCall("Trick.Serialize",this,s); }
} // namespace map
