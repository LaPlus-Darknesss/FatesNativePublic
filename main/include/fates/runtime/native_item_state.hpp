#pragma once
#include <array>
#include <cstdint>
namespace fates::runtime::native {
// Semantic inventory value. No executable address or definition pointer.
struct UnitItemState {
    std::uint16_t item_id{},state{};
    bool operator==(const UnitItemState&) const=default;
};
struct UnitInventoryState {
    bool bound{};
    std::uint16_t owner_person{};
    std::array<UnitItemState,5> items{};
    bool operator==(const UnitInventoryState&) const=default;
};
}
