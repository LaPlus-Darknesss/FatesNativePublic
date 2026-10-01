#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

class ItemKind {
public:
    enum class Type : std::int32_t {};
    struct Data; // Retail Unit::CaclulateFirstWeaponExp consumes this table-entry type.

    // Retail Get(int) proves a 0x0C stride. Field names remain opaque until
    // the member-accessor/message layer is promoted.
    std::array<std::byte, 0x0C> raw{};

    static void Initialize(const void* data);
    static ItemKind* Get(int kind);
    static void Finalize();
};

static_assert(sizeof(ItemKind) == 0x0C);

// GameData.yml names the corresponding table WeaponProficiencyTable.
using WeaponProficiency = ItemKind;

class ItemSubKind {
public:
    enum class Type : std::int32_t {};
    // Retail Get(int) proves 0x10. GetHelp (currently evidence-only) also
    // constrains the message field at +0x08, but the remainder stays opaque
    // until that accessor family is promoted.
    std::array<std::byte, 0x10> raw{};

    static void Initialize(const void* data);
    static ItemSubKind* Get(int subKind);
    static void Finalize();
};

static_assert(sizeof(ItemSubKind) == 0x10);

// GameData.yml names the corresponding table WeaponProficiencyDataTable.
using WeaponProficiencyData = ItemSubKind;

class ItemRefine {
public:
    // Retail ItemRefine::Get proves a four-byte Forge-entry stride. The
    // individual byte meanings are schema hypotheses until more first-party
    // readers are promoted.
    std::array<std::byte, 0x04> raw{};

    static void Initialize(const void* data);
    static ItemRefine* Get(int levelTable, int upgradeIndex);
    static void Finalize();
};

static_assert(sizeof(ItemRefine) == 0x04);

// The localized gameplay feature is the Forge. Retain ItemRefine as the exact
// retail symbol family and provide the readable source alias.
using ForgeData = ItemRefine;
