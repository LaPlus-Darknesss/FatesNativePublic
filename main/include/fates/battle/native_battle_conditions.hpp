#pragma once
#include "fates/battle/native_battle_dual_preparation.hpp"
#include "fates/runtime/native_runtime.hpp"
#include "fates/runtime/native_item_state.hpp"
#include <optional>
namespace fates::battle::native {
struct BattleSideConditions {
    BattleDualSide side{};
    std::int32_t resolved_item_index{-1};
    std::optional<std::uint8_t> terrain_id{};
    bool operator==(const BattleSideConditions&) const=default;
};
struct BattleConditionItemFacts {
    bool magic{},staff{},obstruct_staff{},long_range{},absorb_half{},attack_twice{};
};
struct BattleConditionTerrain {std::optional<std::uint8_t> id{};};
// An empty optional is unresolved, distinct from a known empty item/index/terrain.
// Services are queried lazily in original order. Cached symbol resolution is an
// input boundary; no initialization guard, allocation or original pointer owner.
struct BattleSideConditionServices {
    virtual ~BattleSideConditionServices()=default;
    virtual std::optional<std::array<std::int8_t,2>> Coordinates(std::uint16_t)=0;
    virtual std::optional<std::int32_t> EquippedIndex(std::uint16_t)=0;
    virtual std::optional<fates::runtime::native::UnitItemState> InventoryItem(std::uint16_t,std::int32_t)=0;
    virtual std::optional<BattleConditionItemFacts> ItemFacts(fates::runtime::native::UnitItemState)=0;
    virtual std::optional<std::int32_t> HeldIndex(std::uint16_t)=0;
    virtual std::optional<BattleConditionTerrain> TerrainAt(std::int32_t,std::int32_t)=0;
    virtual std::optional<std::uint16_t> Category(std::uint16_t)=0;
    virtual std::optional<bool> HasWingShield(std::uint16_t)=0;
};
enum class BattleConditionStatus : std::uint8_t {Ok,UnresolvedCoordinates,UnresolvedEquipment,UnresolvedItem,UnresolvedHeldIndex,UnresolvedTerrain,UnresolvedCategory,UnresolvedSkill,InvalidUnit};
struct BattleConditionResult {
    BattleConditionStatus status{BattleConditionStatus::Ok};
    BattleSideConditions value{};
};
BattleConditionResult CompleteBattleSideConditionsExact(const BattleSideConditions&,
    std::uint32_t battle_flags,BattleSideConditionServices&);
// Original Unit::GetCategory's three private-flag transforms. Definition flags
// and current Unit private flags are merged; dragon removal follows addition.
std::uint16_t ResolveUnitCategoryExact(std::uint16_t job_category,
    std::uint64_t unit_flags,std::uint64_t person_flags,std::uint64_t job_flags) noexcept;
std::int32_t HeldInventoryIndexExact(const std::array<fates::runtime::native::UnitItemState,5>&,
    const std::array<std::uint8_t,5>& groups,const std::array<bool,5>& staff_eligible) noexcept;
std::optional<BattleConditionItemFacts> ProjectBattleConditionItemFacts(
    const fates::runtime::native::DefinitionStore&,fates::runtime::native::UnitItemState);
std::optional<std::uint16_t> ProjectUnitBattleCategory(const fates::runtime::native::NativeRuntime&,
    const fates::runtime::native::UnitState&);
// Uses current bound inventory when the original routine requests an item/index.
// Prefilled item/terrain inputs keep their original precedence. Slot5..9 refuse.
BattleConditionResult ProjectCurrentBattleSideConditions(const fates::runtime::native::NativeRuntime&,
    const BattleSideConditions&,std::uint32_t battle_flags=0);
// Existing preview already selected a calculation item (including known empty).
// It supplies that private calculation slot explicitly, not an inferred held or
// equipped inventory index. Used by both current and ID-only calculation scopes.
BattleConditionResult ProjectSelectedBattleSideConditions(const fates::runtime::native::NativeRuntime&,
    const fates::runtime::native::UnitState&,fates::runtime::native::UnitItemState,
    std::uint32_t side_flags,std::uint32_t battle_flags=0);
// Explicit private calculation selection with already-propagated side context.
// The selected instance is bound to this side's nonnegative requested index.
BattleConditionResult ProjectSelectedBattleSideConditions(const fates::runtime::native::NativeRuntime&,
    const fates::runtime::native::UnitState&,fates::runtime::native::UnitItemState,
    const BattleSideConditions&,std::uint32_t battle_flags=0);
}
