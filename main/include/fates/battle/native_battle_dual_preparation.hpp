#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <vector>
namespace fates::battle::native {
inline constexpr std::uint16_t kNoBattleDualUnit=0xffffu;
// Private semantic snapshot. Indices identify facts supplied by the caller;
// they are not executable pointers or a claim that live clone state is owned.
struct BattleDualUnit {
    std::uint32_t public_flags{};
    std::uint16_t partner{kNoBattleDualUnit}, clone{kNoBattleDualUnit};
    std::int8_t x{},y{};
    bool operator==(const BattleDualUnit&) const = default;
};
struct BattleDualSide {
    std::uint16_t unit{kNoBattleDualUnit}, source{kNoBattleDualUnit};
    std::int32_t requested_item_index{-1};
    // unit::Item assignment copies two halfwords, including its opaque state.
    std::array<std::uint16_t,2> item{};
    std::int32_t x{-1},y{-1},reliance_level{};
    std::uint32_t flags{};
    bool operator==(const BattleDualSide&) const = default;
};
struct BattleDualView {
    std::array<BattleDualSide,4> sides{};
    std::vector<BattleDualUnit> units{};
    std::uint32_t battle_flags{};
    bool operator==(const BattleDualView&) const = default;
};
enum class BattleDualPreparationStatus : std::uint8_t {
    Ok, InvalidUnitGraph, UnresolvedSelection, UnresolvedReliance
};
struct BattleDualPreparationServices {
    virtual ~BattleDualPreparationServices()=default;
    // nullopt means unknown. kNoBattleDualUnit means a proved empty selection.
    // The view includes earlier primary writes, matching original call order.
    virtual std::optional<std::uint16_t> SelectLocalSource(
        const BattleDualView&,std::uint8_t primary)=0;
    virtual std::optional<std::int32_t> RelianceLevel(
        const BattleDualView&,std::uint8_t primary,std::uint16_t source)=0;
};
struct BattleDualPreparationResult {
    BattleDualPreparationStatus status{BattleDualPreparationStatus::InvalidUnitGraph};
    BattleDualView view{};
    // ComplementDual calls Side::ComplementConditions for 2 then 3 after all
    // source writes. This owner returns that obligation; item/terrain/skill
    // completion must still run before detail calculation. Early skip has none.
    std::array<std::uint8_t,2> conditions_order{{2,3}};
    std::uint8_t conditions_count{};
};
BattleDualPreparationResult PrepareBattleDualSourcesExact(
    const BattleDualView&,BattleDualPreparationServices&);
// Only Side::ComplementConditions' coordinate prefix, before item resolution.
// Negative axes fall back independently; side flag8 suppresses both axes.
BattleDualPreparationStatus CompleteBattleSidePositionExact(
    BattleDualView&,std::uint8_t side);
void CompleteBattleSidePositionExact(std::int32_t& x,std::int32_t& y,
    std::uint32_t side_flags,std::int8_t unit_x,std::int8_t unit_y) noexcept;
struct BattleCalculationPairFlags {
    BattleDualPreparationStatus status{BattleDualPreparationStatus::InvalidUnitGraph};
    std::vector<std::uint32_t> during_calculation{},after_cleanup{};
};
// BattleInfo::Calculate's two pair-flag loops. The caller applies these only
// to its private calculation image. Cleanup clears bit0x200000, including an
// already-set bit; it is not restoration of the original flags. No live writes.
BattleCalculationPairFlags ProjectBattleCalculationPairFlagsExact(const BattleDualView&);
// Stat getters choose public flag2 normally and flag4 under temporary0x200000.
// This selects which member receives pair capability bonuses; it does not merely
// suppress them. Partner presence is a separately validated semantic reference.
bool BattlePairCapabilityAppliesExact(std::uint32_t public_flags,bool partner_present) noexcept;
std::int32_t ResolveBattlePairCapabilityExact(std::int32_t base,std::int32_t pair_bonus,
    std::uint32_t public_flags,bool partner_present) noexcept;
}
