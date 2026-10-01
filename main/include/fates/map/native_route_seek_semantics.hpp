#pragma once

#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace fates::map::native {

struct EventRouteSeekResult {
    std::vector<std::uint8_t> route;
    std::vector<std::uint8_t> backtrackTieCounts;
    std::uint32_t systemRngDrawCount{};
};

// Portable semantic movement image. -1 is unreachable; non-negative values are
// accumulated retail movement cost. This intentionally does not expose Deploy
// queue/object layout.
std::vector<std::int16_t> BuildPositiveCostMovementImage(
    int width,
    int height,
    int startX,
    int startY,
    int maxCost,
    std::span<const std::int8_t> enterCosts);

// Exact SetForEvent/Route::Seek choice behavior for alternate=false. Retail
// still consumes one system-RNG draw per backtrack step, then forces tie index
// zero. The consumer is called once per step with the retail tie/modulus count.
EventRouteSeekResult SeekEventRouteExact(
    int width,
    int height,
    std::span<const std::int16_t> moveImage,
    int goalX,
    int goalY,
    const std::function<void(std::uint32_t)>& consumeSystemRngDraw);

} // namespace fates::map::native
