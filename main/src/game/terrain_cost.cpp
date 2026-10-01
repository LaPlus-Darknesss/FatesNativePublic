#include "fates/game/terrain_cost.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace {

const std::byte* gMovementCostTable = nullptr;

std::uint32_t ReadU32(const void* address) {
    std::uint32_t value{};
    std::memcpy(&value, address, sizeof(value));
    return value;
}

} // namespace

void TerrainCost::Initialize(const void* data) {
    gMovementCostTable = static_cast<const std::byte*>(data);
}

const MovementCosts* TerrainCost::Get(int movementCostIndex) {
    // Retail derives the row width from the first word and rounds it to a
    // 4-byte boundary before indexing. FE14's schema corroborates a 16-byte
    // row for the shipped table, but the dynamic formula is preserved.
    const std::uint32_t width = ReadU32(gMovementCostTable);
    const std::size_t stride = (static_cast<std::size_t>(width) + 3u) & ~3u;
    return reinterpret_cast<const MovementCosts*>(
        gMovementCostTable + 4 +
        static_cast<std::ptrdiff_t>(movementCostIndex) *
            static_cast<std::ptrdiff_t>(stride));
}

void TerrainCost::Finalize() {
    if (gMovementCostTable != nullptr) {
        gMovementCostTable = nullptr;
    }
}
