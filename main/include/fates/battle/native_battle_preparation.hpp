#pragma once
#include "fates/battle/native_battle_conditions.hpp"
namespace fates::battle::native {
struct BattlePreparationState {
    BattleDualView view{};
    std::array<std::int32_t,4> resolved_item_indices{{-1,-1,-1,-1}};
    std::array<std::optional<std::uint8_t>,4> terrain_ids{};
    std::int32_t distance{};
    // Original byte+0x808. Its movement-flag producer is owned; downstream
    // meaning is not yet proved. In particular this is NOT counter permission.
    std::uint8_t movement_rule_byte{1};
    BattleSideConditions Side(std::uint8_t i) const;
    void SetSide(std::uint8_t i,const BattleSideConditions&);
    bool operator==(const BattlePreparationState&) const=default;
};
struct BattlePreparationServices {
    virtual ~BattlePreparationServices()=default;
    virtual BattleConditionResult CompleteSide(const BattlePreparationState&,std::uint8_t)=0;
    virtual std::optional<bool> ItemNegatesTerrain(fates::runtime::native::UnitItemState)=0;
    virtual std::optional<bool> MovementForbidden(std::uint16_t)=0;
    virtual std::optional<std::uint16_t> Clone(std::uint16_t)=0;
};
enum class BattlePreparationStatus : std::uint8_t {
    Ok,UnresolvedSide,UnresolvedTerrainRule,UnresolvedMovementRule,UnresolvedClone,InvalidUnit,UnresolvedSources
};
struct BattlePreparationResult {
    BattlePreparationStatus status{BattlePreparationStatus::Ok};
    BattlePreparationState state{};
    BattleConditionStatus side_status{BattleConditionStatus::Ok};
    BattleDualPreparationStatus source_status{BattleDualPreparationStatus::Ok};
};
// Full cached-symbol outer ComplementConditions body, with explicit services.
// Failures retain the entire original input. No static cache lifecycle or RNG.
BattlePreparationResult CompleteBattleConditionsExact(const BattlePreparationState&,BattlePreparationServices&);
// ComplementDual's source writes, then COMPLETE side conditions for 2 then 3.
BattlePreparationResult CompleteBattleSourcesExact(const BattlePreparationState&,BattleDualPreparationServices&,BattlePreparationServices&);
BattlePreparationResult PrepareBattleContextExact(const BattlePreparationState&,BattleDualPreparationServices&,BattlePreparationServices&);
std::int32_t BattleDistanceExact(std::int32_t x0,std::int32_t y0,std::int32_t x1,std::int32_t y1) noexcept;
bool MovementForbiddenExact(std::uint64_t unit,std::uint64_t person,std::uint64_t job) noexcept;
std::optional<bool> ProjectBattleItemTerrainRule(const fates::runtime::native::DefinitionStore&,fates::runtime::native::UnitItemState);
std::optional<bool> ProjectBattleMovementRule(const fates::runtime::native::NativeRuntime&,std::uint16_t);
// Current inventory, including held lookup. Clone query remains unresolved.
BattlePreparationResult ProjectCurrentBattleConditions(const fates::runtime::native::NativeRuntime&,const BattlePreparationState&);
using BattleCalculationItems=std::array<std::optional<fates::runtime::native::UnitItemState>,4>;
// Explicit selected calculation items (or the already-proved equipped view).
// Does not infer current inventory indices for an unbound ID-only caller.
BattlePreparationResult ProjectBattleCalculationConditions(const fates::runtime::native::NativeRuntime&,const BattlePreparationState&,const BattleCalculationItems&);
BattlePreparationResult ProjectBattleCalculationSources(const fates::runtime::native::NativeRuntime&,const BattlePreparationState&,const BattleCalculationItems&,BattleDualPreparationServices&);
}
