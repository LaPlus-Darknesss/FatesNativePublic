#pragma once

#include "fates/detail/arm32_address.hpp"

#include <cstdint>

struct ArenaTable {
    fates::decomp_detail::Arm32Address entries{};
    std::uint32_t count{};
};

static_assert(sizeof(ArenaTable) == 8);

class Arena {
public:
    static void Initialize(const void* data);
    static const ArenaTable* GetLow();
    static const ArenaTable* GetHigh();
};
