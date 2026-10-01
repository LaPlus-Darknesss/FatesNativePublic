#pragma once
#include "fates/runtime/native_runtime.hpp"
#include <cstdint>
#include <span>

namespace fates::ai::native {

enum class NativeDisposDifficulty : std::uint8_t { Normal=0, Hard=1, Lunatic=2 };
enum class NativeDisposCombatInitStatus : std::uint8_t {
    Ok, MissingDefinition, UnsupportedLevel, InvalidItem, NoEquipableAuthoredWeapon,
    InvalidDifficulty, UnsupportedItemVariant, UnsupportedEquipRestriction,
};
struct NativeDisposCombatInitResult {
    NativeDisposCombatInitStatus status{NativeDisposCombatInitStatus::Ok};
    std::uint16_t initialized_units{};
    std::uint16_t equipped_units{};
    std::uint16_t failed_unit_slot{0xFFFFu};
    std::uint16_t failed_item_id{};
    std::uint16_t excluded_difficulty_items{};
    std::uint16_t stored_exp_raises{};
};

// Shared portable slice of retail Unit::CreateFromDispos/CreateImpl combat
// initialization. Callers supply the already-imported native Unit slots; chapter
// topology and AI semantics stay outside this routine.
NativeDisposCombatInitResult InitializeDisposCombatState(
    fates::runtime::native::NativeRuntime& runtime,
    NativeDisposDifficulty difficulty,
    std::span<const std::uint16_t> unit_slots);

} // namespace fates::ai::native
