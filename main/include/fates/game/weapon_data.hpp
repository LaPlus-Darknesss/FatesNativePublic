#pragma once

#include "fates/game/item_kinds.hpp"

#include <array>
#include <cstddef>

class WeaponBonus {
public:
    // Retail lookup arithmetic proves the 0x0C stride. Rank-specific byte
    // labels are retained in schema provenance until first-party readers
    // constrain their exact source semantics.
    std::array<std::byte, 0x0C> raw{};

    static void Initialize(const void* data);
    static WeaponBonus* Get(ItemKind::Type type);
    static void Finalize();
};

static_assert(sizeof(WeaponBonus) == 0x0C);

struct WeaponInteractionTable {
    // Paragon identifies the retail table as AllWeaponInteractions (0x40),
    // while retail WeaponInteract::Get exposes the single table pointer.
    // Individual byte meanings remain evidence-only.
    std::array<std::byte, 0x40> raw{};
};

static_assert(sizeof(WeaponInteractionTable) == 0x40);

class WeaponInteract {
public:
    static void Initialize(const void* data);
    static const WeaponInteractionTable* Get();
    static void Finalize();
};
