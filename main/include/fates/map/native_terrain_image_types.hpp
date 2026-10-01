#pragma once
#include <array>
#include <cstdint>
namespace fates::map::native {
struct TerrainImageGeometry {
    std::uint8_t width{32},height{32},min_x{1},min_y{1},max_x{31},max_y{31};
    bool operator==(const TerrainImageGeometry&) const=default;
};
struct TerrainImagePlanes {
    std::array<std::uint8_t,1024> terrain{},refresh_copy{},cost_indices{};
    bool operator==(const TerrainImagePlanes&) const=default;
};
struct TerrainImageGrid {
    TerrainImageGeometry geometry;
    TerrainImagePlanes planes;
    bool operator==(const TerrainImageGrid&) const=default;
};
// Explicit snapshot of CastleWorld+5, not its constructor or an archive fallback.
struct CastleTerrainPlane {std::array<std::uint8_t,1024> cells{};};
}
