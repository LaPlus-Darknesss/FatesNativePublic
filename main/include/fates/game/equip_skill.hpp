#pragma once

#include "fates/detail/arm32_address.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

class EquipSkill {
public:
    // Retail lookup/comparison/accessor evidence constrains the 0x20-byte
    // record and the named members below. Bytes whose gameplay meaning is
    // currently supported only by third-party schemas deliberately remain
    // opaque until a retail accessor or another first-party source proves
    // them.
    fates::decomp_detail::Arm32Address unknown00{};       // +0x00
    fates::decomp_detail::Arm32Address nameMessage{};     // +0x04, GetName
    fates::decomp_detail::Arm32Address helpMessage{};     // +0x08, GetHelp
    fates::decomp_detail::Arm32Address unknown0C{};       // +0x0C
    std::uint16_t id{};                                   // +0x10, SortCompare
    std::int16_t sortOrder{};                             // +0x12, SortCompare
    std::uint16_t icon{};                                 // +0x14, GetIcon
    std::array<std::byte, 3> unknown16{};                 // +0x16
    std::uint8_t flags19{};                               // +0x19, IsUnknown/name/help/icon gate
    std::array<std::byte, 6> unknown1A{};                 // +0x1A

    static void Initialize(const void* data, int commonCount, int totalCount);
    static bool SortCompare(short lhs, short rhs, void* context);
    static int GetCommonNum();
    static EquipSkill* Get(const char* identifier);
    static EquipSkill* Get(short skillId);
    static int GetNum();
    static void Finalize();
};

static_assert(sizeof(EquipSkill) == 0x20);
static_assert(offsetof(EquipSkill, nameMessage) == 0x04);
static_assert(offsetof(EquipSkill, helpMessage) == 0x08);
static_assert(offsetof(EquipSkill, id) == 0x10);
static_assert(offsetof(EquipSkill, sortOrder) == 0x12);
static_assert(offsetof(EquipSkill, icon) == 0x14);
static_assert(offsetof(EquipSkill, flags19) == 0x19);

// Official localized terminology is simply "Skill"; retain the retail
// EquipSkill class name in source/provenance and offer a readable alias.
using SkillData = EquipSkill;
