#pragma once

#include <cstddef>
#include <cstdint>

struct MovementCosts {
    // Paragon FE14 MoveCosts schema names the shipped 0x10-byte row. Fields
    // still marked unknown by that schema intentionally stay unknown here.
    std::int8_t wall_no_fly{};
    std::int8_t unknown{};
    std::int8_t emptiness_fly_1{};
    std::int8_t tornado{};
    std::int8_t plain{};
    std::int8_t woods{};
    std::int8_t pillar{};
    std::int8_t desert{};
    std::int8_t mountain{};
    std::int8_t water{};
    std::int8_t forts{};
    std::int8_t tempest{};
    std::int8_t unknown_2{};
    std::int8_t icy_sea{};
    std::int8_t dark_waters{};
    std::int8_t unknown_3{};
};
static_assert(sizeof(MovementCosts) == 0x10);

class TerrainCost {
public:
    static void Initialize(const void* data);
    static const MovementCosts* Get(int movementCostIndex);
    static void Finalize();
};
