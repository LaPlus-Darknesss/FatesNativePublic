#include "fates/headless/b007_event14_dispos_creation.hpp"

namespace fates::headless {
namespace {
constexpr unsigned char kEvent14Pid[] = {
    0x50,0x49,0x44,0x5f,0x42,0x30,0x30,0x37,0x5f,0x45,0x44,
    0x83,0x43,0x83,0x78,0x83,0x93,0x83,0x67,0x97,0x70,
    0x83,0x7d,0x83,0x4e,0x83,0x78,0x83,0x58
};

std::string_view Raw(const unsigned char* p, std::size_t n) {
    return {reinterpret_cast<const char*>(p), n};
}
} // namespace

std::string_view B007PidEvent14MacbethRaw() {
    return Raw(kEvent14Pid, sizeof(kEvent14Pid));
}

B007Event14CreateStatus ProjectB007Event14Mode0Creation(
    const Fe14DisposFileProjection& dispos,
    const B007Event14PreferredTileAvailable& preferred_available,
    B007Event14CreatedUnitProjection& out) {
    const auto* group = dispos.FindGroup("Event14");
    if (!group || group->spawns.size() != 1 || !preferred_available) {
        return B007Event14CreateStatus::Rejected;
    }

    const auto& s = group->spawns.front();
    if (s.pid != B007PidEvent14MacbethRaw() || !s.job.empty() || s.team != 2 ||
        s.level != 0 || s.coord1_x != 20 || s.coord1_y != 14 ||
        s.coord2_x != 15 || s.coord2_y != 12 || s.spawn_flags != 0x700u) {
        return B007Event14CreateStatus::Rejected;
    }

    out.pid = s.pid;
    out.force = s.team;
    out.level = s.level;
    out.search_origin_x = s.coord1_x;
    out.search_origin_y = s.coord1_y;
    out.preferred_x = s.coord2_x;
    out.preferred_y = s.coord2_y;
    out.spawn_flags = s.spawn_flags;
    out.fresh_unit_slot_required = true;
    out.force_transfer_required = true;
    out.active_map_unit = true;
    out.explicit_job_override = false;

    if (!preferred_available(out.preferred_x, out.preferred_y)) {
        return B007Event14CreateStatus::NeedsRetailPlacementSearch;
    }
    return B007Event14CreateStatus::CreatedAtPreferredCoordinate;
}

} // namespace fates::headless
