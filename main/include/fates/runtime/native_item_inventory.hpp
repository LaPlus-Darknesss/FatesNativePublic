#pragma once
#include "fates/runtime/native_definition_store.hpp"
#include "fates/runtime/native_game_state.hpp"
#include <optional>
#include <span>
namespace fates::runtime::native {
bool ItemIsWeaponExact(std::uint8_t weapon_group) noexcept;
std::uint8_t ItemRefineRankExact(UnitItemState,bool weapon) noexcept;
std::uint8_t ItemEnduranceExact(UnitItemState,bool weapon) noexcept;
UnitItemState SetItemRefineRankExact(UnitItemState,bool weapon,std::int32_t rank) noexcept;
UnitItemState SetItemEnduranceExact(UnitItemState,bool weapon,std::int32_t endurance) noexcept;
UnitItemState NewItemInstanceExact(std::uint16_t id,bool weapon,std::uint8_t uses) noexcept;
std::int32_t EquippedInventoryIndexExact(const std::array<UnitItemState,5>&) noexcept;
// Original ItemEquip's inventory movement with explicitly resolved CanItemEquip
// answers. -1 scans in inventory order. Copies both words and preserves other
// flags, including a second pre-existing equipped bit. Not an eligibility owner.
bool EquipInventoryExact(std::array<UnitItemState,5>&,std::int32_t index,
    const std::array<bool,5>& can_equip) noexcept;
struct ItemCombatValues {
    std::int32_t power{},hit{},critical{};
    bool refinement_applied{};
    std::uint8_t table{},rank{};
};
// Nullopt is missing definition/table memory, not zero contribution. The original
// getters consume unsigned power/forge bytes and signed base hit/critical shorts.
std::optional<ItemCombatValues> ProjectItemCombatValues(const DefinitionStore&,UnitItemState);
enum class InventoryStatus : std::uint8_t { Ok,Unbound,StaleIdentity,InvalidUnit,MissingDefinition,InvalidIndex,Rejected };
InventoryStatus ValidateCurrentInventory(const UnitState&) noexcept;
// Explicit carried snapshot restoration. Does not infer save data, eligibility,
// uses, refinement, drop state or constructor history from an item ID.
InventoryStatus RestoreCurrentInventory(const DefinitionStore&,UnitState&,
    const std::array<UnitItemState,5>&);
// Stages the original inventory writer; caller supplies authoritative equip
// results. Updates only current inventory and its equipped-ID compatibility view.
InventoryStatus EquipCurrentInventory(UnitState&,std::int32_t,const std::array<bool,5>&);
// Existing ID-only callers retain their explicit unrefined calculation contract
// when no snapshot is bound. A bound but inconsistent snapshot never falls back.
std::optional<std::array<UnitItemState,5>> ReadCalculationInventory(const UnitState&);
std::optional<UnitItemState> ReadCalculationEquippedItem(const UnitState&);
}
