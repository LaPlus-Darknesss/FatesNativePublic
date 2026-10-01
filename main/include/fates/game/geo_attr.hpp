#pragma once

#include "fates/detail/arm32_address.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

class GeoAttr {
public:
    // Retail initialization hashes the string pointer at +0x00 with
    // sign::GetSum16 and stores the result at +0x10.
    fates::decomp_detail::Arm32Address name{};
    std::array<std::byte, 0x0C> unknown04{};
    std::uint32_t nameSum16{};

    static void Initialize();
    static GeoAttr* GetAttr(std::uint16_t nameSum);
    static void Finalize();
};

static_assert(sizeof(GeoAttr) == 0x14);
static_assert(offsetof(GeoAttr, nameSum16) == 0x10);
