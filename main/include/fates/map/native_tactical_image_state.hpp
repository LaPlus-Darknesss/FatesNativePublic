#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include <utility>
#include "fates/map/native_terrain_image_types.hpp"
#include "fates/map/native_danger_image_types.hpp"

namespace fates::map::native {
// Host-owned semantic image, not the layout of map::Image. The original byte
// plane uses a 32-cell stride and 1-based UnitPool keys; zero means empty.
struct UnitImageGrid {
    std::uint8_t width{32},height{32};
    std::array<std::uint8_t,1024> cells{};
    bool operator==(const UnitImageGrid&) const=default;
};

struct UnitImageAccess;
class NativeTacticalImageState {
    friend struct UnitImageAccess;
    friend struct TerrainImageAccess;
    friend struct DangerImageAccess;
    bool danger_initialized_{},danger_bound_{};
    DangerImage danger_{};
    // Collision-free semantic dependency record, never a raw object-memory hash.
    // It excludes the derived Danger planes and does not capture runtime pointers.
    std::vector<std::uint64_t> danger_inputs_;
    struct Source {
        bool occupied{};
        std::uint64_t generation{};
        std::uint16_t person{};
        std::uint8_t force{};
        std::int16_t x{},y{};
        std::uint32_t image_flags{};
        bool operator==(const Source&) const=default;
    };
    struct Order {
        bool bound{};
        std::uint16_t count{};
        std::array<std::uint16_t,250> slots{},persons{};
        bool operator==(const Order&) const=default;
    };
    bool bound_{};
    bool map_active_{};
    std::uint8_t chapter_{};
    // One shared full-width/full-height pair governs both owners. Terrain refresh
    // changes these dimensions without rebuilding cells or rebinding Unit inputs.
    UnitImageGrid grid_{};
    std::array<std::uint8_t,4> active_{{1,1,31,31}};
    TerrainImagePlanes terrain_{};
    bool terrain_initialized_{},terrain_bound_{},terrain_geometry_changed_{};
    bool terrain_map_active_{};
    std::uint8_t terrain_chapter_{};
    std::array<std::uint32_t,6> terrain_source_metadata_{};
    std::array<std::uint8_t,1024> terrain_source_grid_{};
    std::vector<std::pair<std::uint8_t,std::uint32_t>> terrain_source_tiles_;

    std::array<Source,250> sources_{};
    std::array<Order,3> orders_{};
};
}
