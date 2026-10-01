#pragma once
#include <array>
#include <cstdint>
#include <optional>

namespace fates::map::native {

using UnitHandle = std::uint8_t; // semantic 1-based Unit-pool slot; 0 means no Unit.

struct CursorPersistV0 {
    float x{};
    float y{};
};

struct CursorSeedState {
    float x{};
    float y{};
    float previous_x{};
    float previous_y{};
};

struct MindState {
    UnitHandle unit{};
    UnitHandle original_unit{};
    std::uint8_t x{};
    std::uint8_t y{};
    std::uint8_t aux_4{0xFF};
    std::uint8_t aux_5{0xFF};
    std::uint8_t aux_6{};
    std::uint8_t aux_7{};
    std::uint8_t aux_8{0xFF};
    std::uint8_t aux_9{};
    std::uint8_t target{};
    std::uint8_t aux_a{0xFF};
    std::uint8_t aux_b{0xFF};
    std::uint16_t aux_c{};
    UnitHandle trade_unit{};
    UnitHandle double_trade_unit{};
    std::uint8_t aux_10{0xFF};
    std::uint8_t transient_11{};
    UnitHandle continuous_snapshot{};
    std::uint8_t transient_13{};
    std::uint8_t move_power{};
};

struct ResolvedUnitSelection {
    UnitHandle slot{};
    std::uint8_t x{};
    std::uint8_t y{};
    std::uint8_t move_power{};
};

struct SelectedPosition {
    UnitHandle unit{};
    std::int32_t x{-1};
    std::int32_t y{-1};
};

struct TargetAggregate {
    std::uint16_t count{};
    std::uint8_t usable_item_mask{};
};

struct FreeCursorPrepareState {
    std::uint32_t cursor_flags{};
    bool reset_route{true};
    bool reset_target{true};
    bool game_info_viewer_enabled{true};
    std::uint8_t process_state_4c{};
    bool gradation_enabled{false};
};

enum class FreeCursorExit : std::uint8_t {
    Continue = 0,
    Cancel = 1,
    ErrorOperation = 2,
};

struct TrickCursorPrepareState {
    bool has_trick{};
    std::int32_t cursor_x{};
    std::int32_t cursor_y{};
    std::int32_t focus_x{-1};
    std::int32_t focus_y{-1};
};

struct TurnEndCommit {
    std::uint32_t cursor_flags{};
    std::int8_t saved_x{};
    std::int8_t saved_y{};
    std::uint8_t deploy_panel_a_mode{};
    std::uint8_t deploy_panel_b_mode{};
    bool game_info_enabled{false};
    bool terrain_info_visible{false};
    bool job_intro_enabled{false};
    bool gradation_enabled{false};
};

[[nodiscard]] constexpr std::uint32_t CursorTileIndex32(std::uint32_t x, std::uint32_t y) noexcept {
    return x | (y << 5U);
}
[[nodiscard]] CursorSeedState DeserializeCursorV0(const CursorPersistV0& saved) noexcept;
[[nodiscard]] bool CursorHookMode1(std::int8_t old_move, std::int8_t new_move,
                                   bool old_can_mind_seek, bool new_can_mind_seek) noexcept;
[[nodiscard]] constexpr bool CursorHookMode3(bool old_marked, bool new_marked) noexcept {
    return old_marked && !new_marked;
}
[[nodiscard]] std::uint8_t CursorVisualType(std::uint32_t cursor_flags,
                                           bool selection_context_active,
                                           std::uint8_t route_state,
                                           std::uint8_t target_state) noexcept;

[[nodiscard]] MindState ResetMindForUnit(std::optional<ResolvedUnitSelection> unit) noexcept;
void ResetMindTargets(MindState& state) noexcept;
void ResetMindActionState(MindState& state) noexcept;
void RefreshMindContinuous(MindState& state, std::optional<ResolvedUnitSelection> unit) noexcept;
void MindDoubleChange(MindState& state, UnitHandle unit) noexcept;
[[nodiscard]] constexpr std::optional<UnitHandle> ResolveMindUnitSlot(UnitHandle slot) noexcept {
    return slot == 0 ? std::nullopt : std::optional<UnitHandle>{slot};
}

[[nodiscard]] SelectedPosition SelectExplicitPosition(std::int32_t x, std::int32_t y) noexcept;
[[nodiscard]] SelectedPosition SelectUnitPosition(UnitHandle unit,
                                                  std::int32_t unit_x,
                                                  std::int32_t unit_y,
                                                  bool paired_position_active,
                                                  std::int32_t partner_x,
                                                  std::int32_t partner_y) noexcept;
[[nodiscard]] std::uint8_t BuildAttackItemMask(const std::array<bool,5>& can_equip,
                                               const std::array<bool,5>& excluded) noexcept;
[[nodiscard]] std::uint8_t BuildRodItemMask(const std::array<bool,5>& is_rod,
                                            const std::array<bool,5>& can_equip,
                                            const std::array<bool,5>& excluded) noexcept;
[[nodiscard]] constexpr bool IsOrthogonallyAdjacent(std::int32_t ax, std::int32_t ay,
                                                     std::int32_t bx, std::int32_t by) noexcept {
    const auto dx = ax > bx ? ax - bx : bx - ax;
    const auto dy = ay > by ? ay - by : by - ay;
    return dx + dy == 1;
}
[[nodiscard]] constexpr bool CanTradeResolved(bool candidate_skill_allowed,
                                              bool same_force,
                                              bool candidate_is_excluded_link,
                                              bool selected_has_item,
                                              bool candidate_has_item) noexcept {
    return candidate_skill_allowed && same_force && !candidate_is_excluded_link &&
           (selected_has_item || candidate_has_item);
}
void AppendTargetMask(TargetAggregate& aggregate, std::uint8_t usable_items) noexcept;

[[nodiscard]] constexpr FreeCursorPrepareState PrepareFreeCursor(std::uint32_t flags) noexcept {
    return FreeCursorPrepareState{(flags | 0x3U) & ~0x4U, true, true, true, 0, false};
}
[[nodiscard]] constexpr FreeCursorExit ResolveFreeCursorEarlyExit(bool error_operation,
                                                                  bool cancel_operation) noexcept {
    if (error_operation) return FreeCursorExit::ErrorOperation;
    if (cancel_operation) return FreeCursorExit::Cancel;
    return FreeCursorExit::Continue;
}
[[nodiscard]] TrickCursorPrepareState PrepareTrickCursor(std::int32_t cursor_x,
                                                         std::int32_t cursor_y,
                                                         bool has_trick,
                                                         std::int32_t focus_x,
                                                         std::int32_t focus_y) noexcept;
[[nodiscard]] constexpr bool CannonCursorPrepareIsNoOp() noexcept { return true; }
[[nodiscard]] TurnEndCommit CommitTurnEnd(std::uint32_t flags,
                                          std::int32_t cursor_x,
                                          std::int32_t cursor_y) noexcept;

} // namespace fates::map::native
