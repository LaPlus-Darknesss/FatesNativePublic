#pragma once

#include <array>
#include <cstdint>
#include <span>

namespace fates::map::native {

inline constexpr std::uint32_t kRetailDefaultEventDeployFlags = 0x2002u;
inline constexpr std::uint32_t kRetailActorEventMoveBit = 0x80u;
inline constexpr std::uint8_t kRetailRouteTerminator = 0x80u;
inline constexpr std::uint8_t kRetailForcedEventRouteStep = 0x20u;

constexpr bool UnitMovePositionBypassesMapBounds(std::uint32_t scriptFlags) {
    return (scriptFlags & 0x2u) != 0;
}
constexpr bool UnitMovePositionBypassesOccupiedTargetRelocation(std::uint32_t scriptFlags) {
    return (scriptFlags & 0x2u) != 0;
}
constexpr std::uint32_t ResolveEventDeployFlags(std::uint32_t scriptFlags) {
    return (scriptFlags & 0x2000u) != 0 ? 0x2000u : kRetailDefaultEventDeployFlags;
}
constexpr std::uint32_t ResolveEventDeployRetryFlags(std::uint32_t deployFlags) {
    return deployFlags & ~0x2u;
}
constexpr std::uint32_t ResolveActorEventMoveFlags(std::uint32_t scriptFlags) {
    return scriptFlags | kRetailActorEventMoveBit;
}
constexpr bool ShouldForceNonEmptyEventRoute(std::uint8_t firstRouteByte,
                                             std::uint32_t scriptFlags) {
    return (firstRouteByte & kRetailRouteTerminator) != 0 && (scriptFlags & 0x6u) != 0;
}
constexpr std::array<std::uint8_t, 2> ForcedNonEmptyEventRoute() {
    return {kRetailForcedEventRouteStep, kRetailRouteTerminator};
}
constexpr int EventSeekRngDrawCount(int backtrackSteps) {
    return backtrackSteps > 0 ? backtrackSteps : 0;
}

int ResolveRouteGoalX(int startX, std::span<const std::uint8_t> route);
int ResolveRouteGoalY(int startY, std::span<const std::uint8_t> route);

} // namespace fates::map::native
