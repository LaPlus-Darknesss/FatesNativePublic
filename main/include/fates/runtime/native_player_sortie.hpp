#pragma once
#include "fates/runtime/native_runtime.hpp"
#include "fates/headless/b007_dispos_unit_pool.hpp"
#include <span>
#include <vector>

namespace fates::runtime::native {
enum class PlayerSortieStatus : std::uint8_t {
    Ok, InvalidDifficulty, MissingGroup, InvalidReserveOrder, MissingDefinition,
    UnsupportedRecord, MissingFixedUnit, UnsupportedIdentity, InvalidDestination,
    OccupiedDestination, ForceCapacity, PhaseAlreadyBound, UnsupportedContext
};
struct ExistingPlayerSortieContext {
    std::uint8_t difficulty{};
    std::uint8_t game_mode{}; // Raw GameUserData+0x2C; mode 4 is not this path.
    std::uint32_t calculate_flags{}; // Only the ordinary two-pass invocation (0).
};
struct PlayerSortieAssignment {
    std::uint16_t source_record{}, unit_slot{}, authored_person_id{}, actual_person_id{};
    std::int16_t x{},y{};
    bool fixed_identity{}, player_identity_match{}, unique_person_match{};
};
struct PlayerSortieResult {
    PlayerSortieStatus status{PlayerSortieStatus::MissingGroup};
    std::uint16_t failed_record{0xFFFFu};
    std::vector<PlayerSortieAssignment> assignments;
    std::vector<std::uint16_t> unfilled_deployment_records;
    std::vector<std::uint16_t> unused_reserve_slots;
    bool committed{};
};
// Existing Force-3 reserve units only. The explicit order is the selected force
// list, not PID order inferred from deployment-slot labels. Fixed identity pass
// precedes the ordinary slot pass, as Dispos::Calculate/GetUnit do.
// No campaign creation, stat/level/equipment reset, reinstate, generic pair,
// obstacle relocation, coordinate animation, or extra event-group loading.
PlayerSortieResult PlanExistingPlayerSortie(
    const NativeRuntime&, const fates::headless::Fe14DisposGroupProjection&,
    ExistingPlayerSortieContext, std::span<const std::uint16_t> reserve_order);
PlayerSortieResult DeployExistingPlayerSortie(
    NativeRuntime&, const fates::headless::Fe14DisposGroupProjection&,
    ExistingPlayerSortieContext, std::span<const std::uint16_t> reserve_order);
} // namespace fates::runtime::native
