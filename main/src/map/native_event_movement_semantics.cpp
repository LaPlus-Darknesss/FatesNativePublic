#include "fates/map/native_event_movement_semantics.hpp"

namespace fates::map::native {

int ResolveRouteGoalX(int startX, std::span<const std::uint8_t> route) {
    int x = startX;
    for (const auto step : route) {
        if ((step & kRetailRouteTerminator) != 0) break;
        if ((step & 0x1u) != 0) --x;
        else if ((step & 0x2u) != 0) ++x;
    }
    return x;
}

int ResolveRouteGoalY(int startY, std::span<const std::uint8_t> route) {
    int y = startY;
    for (const auto step : route) {
        if ((step & kRetailRouteTerminator) != 0) break;
        if ((step & 0x8u) != 0) --y;
        else if ((step & 0x4u) != 0) ++y;
    }
    return y;
}

} // namespace fates::map::native
