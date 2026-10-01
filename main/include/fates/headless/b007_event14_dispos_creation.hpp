#pragma once

#include "fates/headless/b007_dispos_unit_pool.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

namespace fates::headless {

enum class B007Event14CreateStatus : std::uint8_t {
    Rejected,
    CreatedAtPreferredCoordinate,
    NeedsRetailPlacementSearch,
};

struct B007Event14CreatedUnitProjection {
    std::string pid; // exact retail Shift-JIS bytes
    std::uint8_t force{};
    std::uint8_t level{};
    std::int8_t search_origin_x{};
    std::int8_t search_origin_y{};
    std::int8_t preferred_x{};
    std::int8_t preferred_y{};
    std::uint32_t spawn_flags{};
    bool fresh_unit_slot_required{};
    bool force_transfer_required{};
    bool active_map_unit{};
    bool explicit_job_override{};
};

using B007Event14PreferredTileAvailable =
    std::function<bool(std::int32_t x, std::int32_t y)>;

// Bounded portable projection of the fresh-Unit path reached by
// ev::Dispos("Event14", 0). This closes only the exact Event14 record and the
// preferred-coordinate success branch. If the preferred tile cannot be used,
// retail's Data::Calculate alternate-placement search remains an explicit
// boundary and is never approximated here.
B007Event14CreateStatus ProjectB007Event14Mode0Creation(
    const Fe14DisposFileProjection& dispos,
    const B007Event14PreferredTileAvailable& preferred_available,
    B007Event14CreatedUnitProjection& out);

std::string_view B007PidEvent14MacbethRaw();

} // namespace fates::headless
