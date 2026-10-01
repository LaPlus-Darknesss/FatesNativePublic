#include "fates/map/native_interaction_semantics.hpp"

namespace fates::map::native {

CursorSeedState DeserializeCursorV0(const CursorPersistV0& saved) noexcept {
    return CursorSeedState{saved.x, saved.y, saved.x, saved.y};
}

bool CursorHookMode1(const std::int8_t old_move, const std::int8_t new_move,
                     const bool old_can_mind_seek, const bool new_can_mind_seek) noexcept {
    if (old_move >= 0) {
        if (new_move >= 0) return false;
        return !new_can_mind_seek;
    }
    if (!old_can_mind_seek) return false;
    return !new_can_mind_seek;
}

std::uint8_t CursorVisualType(const std::uint32_t cursor_flags,
                              const bool selection_context_active,
                              const std::uint8_t route_state,
                              const std::uint8_t target_state) noexcept {
    if ((cursor_flags & 0x8U) != 0U) {
        return (cursor_flags & 0x10U) != 0U ? 1U : 0U;
    }
    if (!selection_context_active) return 0U;
    std::uint8_t state = route_state != 0U ? route_state : 0U;
    if (target_state != 0U) state = target_state;
    switch (state) {
        case 2: return 5;
        case 3:
        case 4: return 1;
        case 5: return 2;
        case 0x0E: return 6;
        case 0x13:
        case 0x14:
        case 0x15:
        case 0x16: return 4;
        case 0x26:
        case 0x27: return 2;
        case 0x28: return 7;
        default: return 0;
    }
}

MindState ResetMindForUnit(const std::optional<ResolvedUnitSelection> unit) noexcept {
    MindState state{};
    if (unit) {
        state.unit = unit->slot;
        state.original_unit = unit->slot;
        state.x = unit->x;
        state.y = unit->y;
        state.move_power = unit->move_power;
    }
    ResetMindActionState(state);
    return state;
}

void ResetMindTargets(MindState& state) noexcept {
    state.target = 0;
    state.aux_9 = 0; // intentionally named through the semantic state object, not retail offset.
    state.aux_a = 0xFF;
    state.aux_b = 0xFF;
    state.aux_c = 0;
    state.aux_10 = 0xFF;
}

void ResetMindActionState(MindState& state) noexcept {
    state.aux_4 = 0xFF;
    state.aux_5 = 0xFF;
    state.aux_6 = 0;
    state.aux_7 = 0;
    state.aux_8 = 0xFF;
    state.target = 0;
    state.aux_a = 0xFF;
    state.aux_b = 0xFF;
    state.aux_c = 0;
    state.trade_unit = 0;
    state.double_trade_unit = 0;
    state.aux_10 = 0xFF;
    state.transient_11 = 0;
    state.continuous_snapshot = 0;
    state.transient_13 = 0;
}

void RefreshMindContinuous(MindState& state, const std::optional<ResolvedUnitSelection> unit) noexcept {
    state.continuous_snapshot = state.unit;
    state.move_power = unit ? unit->move_power : 0;
}

void MindDoubleChange(MindState& state, const UnitHandle unit) noexcept {
    state.unit = unit;
}

SelectedPosition SelectExplicitPosition(const std::int32_t x, const std::int32_t y) noexcept {
    return SelectedPosition{0, x, y};
}

SelectedPosition SelectUnitPosition(const UnitHandle unit,
                                    const std::int32_t unit_x,
                                    const std::int32_t unit_y,
                                    const bool paired_position_active,
                                    const std::int32_t partner_x,
                                    const std::int32_t partner_y) noexcept {
    return SelectedPosition{unit,
                            paired_position_active ? partner_x : unit_x,
                            paired_position_active ? partner_y : unit_y};
}

std::uint8_t BuildAttackItemMask(const std::array<bool,5>& can_equip,
                                 const std::array<bool,5>& excluded) noexcept {
    std::uint8_t mask = 0;
    for (std::size_t i = 0; i < 5; ++i) {
        if (can_equip[i] && !excluded[i]) mask = static_cast<std::uint8_t>(mask | (1U << i));
    }
    return mask;
}

std::uint8_t BuildRodItemMask(const std::array<bool,5>& is_rod,
                              const std::array<bool,5>& can_equip,
                              const std::array<bool,5>& excluded) noexcept {
    std::uint8_t mask = 0;
    for (std::size_t i = 0; i < 5; ++i) {
        if (is_rod[i] && can_equip[i] && !excluded[i]) mask = static_cast<std::uint8_t>(mask | (1U << i));
    }
    return mask;
}

void AppendTargetMask(TargetAggregate& aggregate, const std::uint8_t usable_items) noexcept {
    ++aggregate.count;
    aggregate.usable_item_mask = static_cast<std::uint8_t>(aggregate.usable_item_mask | usable_items);
}

TrickCursorPrepareState PrepareTrickCursor(const std::int32_t cursor_x,
                                           const std::int32_t cursor_y,
                                           const bool has_trick,
                                           const std::int32_t focus_x,
                                           const std::int32_t focus_y) noexcept {
    return TrickCursorPrepareState{has_trick, cursor_x, cursor_y,
                                   has_trick ? focus_x : -1,
                                   has_trick ? focus_y : -1};
}

TurnEndCommit CommitTurnEnd(const std::uint32_t flags,
                            const std::int32_t cursor_x,
                            const std::int32_t cursor_y) noexcept {
    return TurnEndCommit{flags & ~0x3U,
                         static_cast<std::int8_t>(cursor_x),
                         static_cast<std::int8_t>(cursor_y),
                         0, 0, false, false, false, false};
}

} // namespace fates::map::native
