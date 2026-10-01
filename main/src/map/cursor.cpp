#include "fates/map/cursor.hpp"
#include "fates/detail/tactical_map_runtime.hpp"
namespace map {
ICamera* Cursor::GetCamera(){ return fates::decomp_detail::TacticalMapValue<ICamera*>("Cursor.GetCamera"); }
const Terrain* Cursor::GetTerrain(int x,int y){ return fates::decomp_detail::TacticalMapValue<const Terrain*>("Cursor.GetTerrain",x,y); }
void Cursor::Initialize(){ fates::decomp_detail::TacticalMapCall("Cursor.Initialize"); }
void Cursor::Deserialize(Stream* s){ fates::decomp_detail::TacticalMapCall("Cursor.Deserialize",s); }
void Cursor::TickDistance(){ fates::decomp_detail::TacticalMapCall("Cursor.TickDistance"); }
Cursor* Cursor::Get(){ return fates::decomp_detail::TacticalMapValue<Cursor*>("Cursor.Get"); }
bool Cursor::IsHook(int mode,int x,int y,int px,int py){ return fates::decomp_detail::TacticalMapValue<bool>("Cursor.IsHook",this,mode,x,y,px,py); }
void Cursor::Finalize(){ fates::decomp_detail::TacticalMapCall("Cursor.Finalize"); }
void Cursor::TickType(){ fates::decomp_detail::TacticalMapCall("Cursor.TickType",this); }
void Cursor::Serialize(Stream* s) const { fates::decomp_detail::TacticalMapCall("Cursor.Serialize",this,s); }
} // namespace map
