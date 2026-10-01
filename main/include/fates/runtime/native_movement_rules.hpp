#pragma once
#include "fates/runtime/native_definition_store.hpp"
#include <optional>
namespace fates::runtime::native {
struct NativeRuntime;
struct UnitState;
struct HeldEnhancementServices {
    virtual ~HeldEnhancementServices()=default;
    virtual std::optional<bool> PersonIsDownload()=0;
    virtual std::optional<std::int8_t> ItemWeaponGroup()=0;
    virtual std::optional<bool> CanEquipWithStaffAndCurrentExp()=0;
};
// Cached IsItemEnhanceHave body. A matching personal restriction is sufficient;
// otherwise nonweapons (group 8) qualify, then CanItemEquip(true,true) decides.
// Services run only when reached. Unknown data is never interpreted as false.
std::optional<bool> HasHeldEnhancementExact(std::uint64_t merged_private_flags,
    std::uint32_t public_flags,std::uint64_t item_flags,bool include_special,
    const MovementRuleDefinitions&,HeldEnhancementServices&);
struct CostFreeServices {
    virtual ~CostFreeServices()=default;
    virtual std::optional<bool> HasAmphibiousSkill()=0;
    virtual std::optional<std::uint64_t> InventoryItemFlags(unsigned index)=0;
    virtual std::optional<bool> HasSpecialHeldEnhancement(unsigned index)=0;
};
// IsCostFree queries Amphibious only for Shooter, then five inventory slots in
// order. Only Fujin-marked items enter the held-enhancement predicate.
std::optional<bool> IsMovementCostFreeExact(std::uint16_t category,
    const MovementRuleDefinitions&,CostFreeServices&);
int TerrainCostFromUnitExact(std::int8_t class_cost,std::int8_t base_cost,
    std::uint64_t merged_private_flags,std::uint64_t personal_cost_mask) noexcept;
std::optional<bool> ProjectCurrentHeldEnhancement(const NativeRuntime&,const UnitState&,
    const ItemDefinition&,bool include_special);
std::optional<bool> ProjectCurrentMovementCostFree(const NativeRuntime&,const UnitState&);
// Private Unit/Person/Job prohibition only; does not query inventory or IsCostFree.
std::optional<bool> ProjectCurrentMovementProhibition(const NativeRuntime&,const UnitState&);
std::optional<bool> ProjectCurrentMovementBaseCostFallback(const NativeRuntime&,const UnitState&);
std::optional<int> ProjectCurrentTerrainCost(const NativeRuntime&,const UnitState&,std::uint8_t cost_index);
struct CurrentMovementRules {
    std::uint8_t class_cost_row{};
    bool movement_prohibited{},base_cost_fallback{},cost_free{};
};
// Resolved current inputs for future Deploy/placement callers. Does not select
// a movement budget, build a field, admit an action or publish a position.
std::optional<CurrentMovementRules> ProjectCurrentMovementRules(const NativeRuntime&,const UnitState&);
}
