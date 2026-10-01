#include "fates/map/world.hpp"
#include "fates/detail/tactical_map_runtime.hpp"
namespace map {
void World::Initialize(){ fates::decomp_detail::TacticalMapCall("World.Initialize"); }
World::World(){ fates::decomp_detail::TacticalMapCall("World.Ctor",this); }
World::~World(){
    // PROVEN ownership shape: retail conditionally frees the bound field-world,
    // cleans Trick/Cursor-facing state, detaches four scene nodes, then destroys
    // four owned node objects. Vendor node layout stays behind the adapter.
    fates::decomp_detail::TacticalMapCall("World.Dtor",this);
}
void World::HideIcon(){ fates::decomp_detail::TacticalMapCall("World.HideIcon",this); }
void World::ShowIcon(){ fates::decomp_detail::TacticalMapCall("World.ShowIcon",this); }
void World::HideBalloon(){ fates::decomp_detail::TacticalMapCall("World.HideBalloon",this); }
void World::ShowBalloon(){ fates::decomp_detail::TacticalMapCall("World.ShowBalloon",this); }
void World::UpdateHeight(){ fates::decomp_detail::TacticalMapCall("World.UpdateHeight",this); }
float World::GetBalloonScale(){ return fates::decomp_detail::TacticalMapValue<float>("World.GetBalloonScale",this); }
void World::Bind(){ fates::decomp_detail::TacticalMapCall("World.Bind",this); }
void World::Draw(){ fates::decomp_detail::TacticalMapCall("World.Draw",this); }
void World::Tick(){ fates::decomp_detail::TacticalMapCall("World.Tick",this); }
void World::Setup(){ fates::decomp_detail::TacticalMapCall("World.Setup",this); }
void World::Unbind(){ fates::decomp_detail::TacticalMapCall("World.Unbind",this); }
void World::Cleanup(){ fates::decomp_detail::TacticalMapCall("World.Cleanup",this); }
void World::Finalize(){ fates::decomp_detail::TacticalMapCall("World.Finalize"); }
void World::ShowUnit(){ fates::decomp_detail::TacticalMapCall("World.ShowUnit",this); }
void World::HideUnit(){ fates::decomp_detail::TacticalMapCall("World.HideUnit",this); }
} // namespace map
