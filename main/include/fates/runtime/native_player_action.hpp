#pragma once
#include "fates/runtime/native_runtime.hpp"
#include <cstdint>
#include <vector>

namespace fates::runtime::native {

struct ReachableCell {
    std::int16_t x{};
    std::int16_t y{};
    std::int16_t cost{};
    bool occupiable{};
};

struct AttackTarget {
    std::uint16_t unit_slot{};
    std::int16_t x{};
    std::int16_t y{};
    std::uint8_t distance{};
};

enum class PlayerActionStatus : std::uint8_t {
    Ok,
    InvalidUnit,
    MissingDefinition,
    MissingTerrain,
    InvalidDestination,
    OccupiedDestination,
    InvalidItem,
};

struct PlayerActionResult {
    PlayerActionStatus status{PlayerActionStatus::InvalidUnit};
    std::uint16_t unit_slot{};
    std::int16_t start_x{};
    std::int16_t start_y{};
    std::int16_t end_x{};
    std::int16_t end_y{};
    std::vector<ReachableCell> reachable;
    std::vector<AttackTarget> attack_targets;
};

// Unit-generic tactical movement used by both player and AI transactions.
// Shared movement-cost engine. The explicit-source/budget form is used by the
// retail AI planning/reverse-goal fields; normal player/AI movement calls the
// same implementation with the Unit's current position and Job movement.
std::vector<ReachableCell> EnumerateUnitMovementFromWithBudget(
    const NativeRuntime& runtime, std::uint16_t unit_slot,
    std::int16_t source_x, std::int16_t source_y, int movement_budget);
// Explicit planning options used by the bounded retail AI move fields. Default
// overload above retains its existing behavior for old callers.
struct MovementFieldOptions {
    bool block_hostiles{true};
    bool active_rectangle_only{};
    bool destroyed_terrain_projection{}; // Deploy flag 0x400, Tile.change_id_2
    bool base_cost_fallback{};            // Deploy flag 0x1000000, TerrainCost row 0
};
std::vector<ReachableCell> EnumerateUnitMovementField(
    const NativeRuntime&, std::uint16_t, std::int16_t, std::int16_t, int,
    MovementFieldOptions);
std::vector<ReachableCell> EnumerateUnitMovement(const NativeRuntime& runtime,
                                                  std::uint16_t unit_slot);
PlayerActionStatus CommitUnitMove(NativeRuntime& runtime,
                                  std::uint16_t unit_slot,
                                  std::int16_t x,
                                  std::int16_t y);

// Compatibility/player-facing names retained for existing callers.
std::vector<ReachableCell> EnumeratePlayerMovement(const NativeRuntime& runtime,
                                                    std::uint16_t unit_slot);
PlayerActionStatus CommitPlayerMove(NativeRuntime& runtime,
                                    std::uint16_t unit_slot,
                                    std::int16_t x,
                                    std::int16_t y);
std::vector<AttackTarget> EnumerateAttackTargets(const NativeRuntime& runtime,
                                                 std::uint16_t unit_slot,
                                                 std::uint16_t item_id);
PlayerActionResult ExecuteMoveThenEnumerateAttack(NativeRuntime& runtime,
                                                  std::uint16_t unit_slot,
                                                  std::int16_t x,
                                                  std::int16_t y,
                                                  std::uint16_t item_id);

} // namespace fates::runtime::native
