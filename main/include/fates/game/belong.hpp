#pragma once

#include <array>
#include <cstddef>

class Belong {
public:
    // Retail Belong records use a 0x10-byte stride. Field-level naming beyond
    // identifier/name/help remains intentionally deferred.
    std::array<std::byte, 0x10> raw{};

    static void Initialize(const void* data, int count);
    static Belong* Get(const char* identifier);
    static Belong* Get(int affiliationId);
    static void Finalize();
};

static_assert(sizeof(Belong) == 0x10);

// Official localized contributor-facing terminology.
using Affiliation = Belong;
