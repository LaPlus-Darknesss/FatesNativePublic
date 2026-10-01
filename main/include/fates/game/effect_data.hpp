#pragma once

#include <array>
#include <cstddef>

class EffectData {
public:
    // Retail StructList::Load uses a 0x4C stride. Only the three floats used
    // by HasBBox are promoted semantically here; other fields stay opaque.
    std::array<std::byte, 0x3C> unknown00{};
    float bboxValue0{}; // +0x3C
    float bboxValue1{}; // +0x40
    float bboxValue2{}; // +0x44
    std::array<std::byte, 0x04> unknown48{};

    static void Initialize();
    static const char* GetFilePath(const char* archiveName);
    static void Finalize();

    bool HasBBox() const;
};

static_assert(sizeof(EffectData) == 0x4C);
static_assert(offsetof(EffectData, bboxValue0) == 0x3C);
