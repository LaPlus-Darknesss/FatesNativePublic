#pragma once

#include <array>
#include <cstddef>

class Personality {
public:
    // Retail Get(int) proves a 0x40 stride. The FE14 schema supplies useful
    // hypotheses for boon/bane subfields, but those names remain out of the
    // durable C++ layout until first-party accessors constrain them.
    std::array<std::byte, 0x40> raw{};

    static void Initialize(const void* data, int count);
    static Personality* Get(int personalityId);
    static void Finalize();
};

static_assert(sizeof(Personality) == 0x40);
