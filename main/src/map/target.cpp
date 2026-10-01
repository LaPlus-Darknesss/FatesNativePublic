#include "fates/map/target.hpp"
#include "fates/detail/tactical_map_runtime.hpp"
namespace map {
void Target::Initialize(){
    // PROVEN: retail initializes selected/working coordinates to -1, clears
    // masks/counts, and initializes a bounded target-entry table. Concrete
    // offsets are intentionally not published as a host layout yet.
    fates::decomp_detail::TacticalMapCall("Target.Initialize");
}
Target* Target::Get(){ return fates::decomp_detail::TacticalMapValue<Target*>("Target.Get"); }
void Target::Finalize(){ fates::decomp_detail::TacticalMapCall("Target.Finalize"); }
#define FATES_TARGET_VOID0(Name) void Target::Name(){ fates::decomp_detail::TacticalMapCall("Target." #Name,this); }
FATES_TARGET_VOID0(EnumerateRod)
FATES_TARGET_VOID0(EnumerateTalk)
FATES_TARGET_VOID0(EnumerateClone)
FATES_TARGET_VOID0(EnumerateDance)
FATES_TARGET_VOID0(EnumerateTrade)
FATES_TARGET_VOID0(EnumerateCharge)
FATES_TARGET_VOID0(EnumerateRescue)
FATES_TARGET_VOID0(EnumerateDestroy)
FATES_TARGET_VOID0(EnumerateDoubleOn)
FATES_TARGET_VOID0(EnumerateDoubleOff)
FATES_TARGET_VOID0(EnumerateTransform)
FATES_TARGET_VOID0(EnumerateTranspose)
FATES_TARGET_VOID0(EnumerateDoubleTrade)
FATES_TARGET_VOID0(EnumerateObstacleAcquisition)
FATES_TARGET_VOID0(EnumerateObstacleInstallation)
FATES_TARGET_VOID0(Reset)
FATES_TARGET_VOID0(Enumerate)
#undef FATES_TARGET_VOID0
void Target::EnumerateWarp(int x,int y){ fates::decomp_detail::TacticalMapCall("Target.EnumerateWarp",this,x,y); }
void Target::SetSelectUnit(const Unit* u){ fates::decomp_detail::TacticalMapCall("Target.SetSelectUnit",this,u); }
void Target::EnumerateAttack(Attack::Type t){ fates::decomp_detail::TacticalMapCall("Target.EnumerateAttack",this,t); }
void Target::EnumerateAttack(int x,int y,unsigned short item,Attack::Type t){ fates::decomp_detail::TacticalMapCall("Target.EnumerateAttackXY",this,x,y,item,t); }
void Target::SetSelectPosition(int x,int y){ fates::decomp_detail::TacticalMapCall("Target.SetSelectPosition",this,x,y); }
std::uint32_t Target::PreCheckItemMaskRod(){
    // PROVEN: retail scans the five carried-item slots, accepts equippable rods,
    // and returns a low five-bit eligibility mask; special disvalue-range rods
    // update a separate per-slot mask.
    return fates::decomp_detail::TacticalMapValue<std::uint32_t>("Target.PreCheckItemMaskRod",this);
}
std::uint32_t Target::PreCheckItemMaskAttack(Attack::Type t){
    // PROVEN: retail scans the same five item slots and returns a low five-bit
    // equippable attack mask, with type-specific item-skill exclusions.
    return fates::decomp_detail::TacticalMapValue<std::uint32_t>("Target.PreCheckItemMaskAttack",this,t);
}
void Target::Data::Set(Unit* u,unsigned short id,int v){ fates::decomp_detail::TacticalMapCall("Target.Data.SetUnit",this,u,id,v); }
void Target::Data::Set(int x,int y,unsigned short id,int v){ fates::decomp_detail::TacticalMapCall("Target.Data.SetPosition",this,x,y,id,v); }
bool Target::IsTrade(const Unit* u) const {
    // PROVEN: retail checks a private-skill exclusion mask, same-force relation,
    // non-self selection, and requires at least one side to hold an item.
    return fates::decomp_detail::TacticalMapValue<bool>("Target.IsTrade",this,u);
}
} // namespace map
