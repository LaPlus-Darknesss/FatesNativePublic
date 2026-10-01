#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace fates::headless {

enum class B007Event14PlacementStatus : std::uint8_t {
    InvalidInput,
    PreferredCoordinate,
    AlternateStrict,
    AlternateRelaxed,
    RejectedPreferredOutsideActiveRect,
    RejectedPreferredInvalidTerrain,
    RejectedNoAlternate,
};

struct B007Event14PlacementGrid {
    int width{};
    int height{};
    int active_min_x{};
    int active_min_y{};
    int active_max_x{}; // exclusive
    int active_max_y{}; // exclusive
    std::span<const std::int8_t> enter_costs;       // -1 = impassable
    std::span<const std::uint8_t> destination_ok;   // retail Terrain validity gate
    std::span<const std::uint8_t> occupied;         // final-destination occupancy
    std::span<const std::uint8_t> strict_blocked;   // non-allied transit blockers for Deploy flag 2
};

struct B007Event14PlacementResult {
    B007Event14PlacementStatus status{B007Event14PlacementStatus::InvalidInput};
    int final_x{-1};
    int final_y{-1};
    int actor_start_x{-1};
    int actor_start_y{-1};
    int preferred_path_cost{-1};
    int origin_path_cost{-1};
    std::uint32_t packed_score{};
    bool used_relaxed_retry{};
    bool fresh_unit_should_clear_on_failure{};
    std::uint32_t rng_draws{}; // exact: placement search consumes zero RNG draws
};

// Exact portable projection of the Event14 branch inside
// map::Dispos::Data::Calculate for B007. The original record supplies
// coord1=(20,14), coord2=(15,12), spawn flags 0x700. The host supplies only
// portable terrain/occupancy facts; retail Deploy/Unit/Actor layouts stay out
// of the ABI.
B007Event14PlacementResult ResolveB007Event14PlacementExact(
    const B007Event14PlacementGrid& grid);

// ev::DisposWait is a shared Command.cmb helper that loops while
// map::Dispos::IsWaitProcess reports an active ProcDispos. Gameplay placement
// is committed before this barrier. A headless host may complete presentation
// immediately by reporting false.
bool B007DisposWaitIsBlocked(bool proc_dispos_active);

} // namespace fates::headless
