#pragma once
#include <array>
#include <cstdint>
namespace fates::map::native {
struct DangerImage {
    std::array<std::uint8_t,128> selected_attack{},all_attack{},selected_rod{},all_rod{};
    std::uint32_t flags{};
    bool operator==(const DangerImage&) const=default;
};
}
