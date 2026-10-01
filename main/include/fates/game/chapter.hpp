#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

class Chapter {
public:
    // The retail indexed getter proves the 0x1C record stride. Paragon and
    // the unpromoted Chapter member-accessor evidence expose many candidate
    // fields, but Pass 7 keeps the record opaque until those accessors are
    // promoted and can carry field semantics as first-party authority.
    std::array<std::byte, 0x1C> raw{};

    static void Initialize(const void* data, int count);
    static Chapter* Get(const char* identifier);
    static Chapter* Get(std::uint8_t chapterId);
    static int GetNum();
    static void Finalize();
};

static_assert(sizeof(Chapter) == 0x1C);
