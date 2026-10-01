#pragma once

#include <array>
#include <cstdint>

class WeaponLevel {
public:
    // Retail GetLevel reads rank thresholds from the first seven bytes.
    // The eighth byte remains opaque until another first-party reader proves it.
    std::array<std::uint8_t, 8> thresholds{};

    static void Initialize(const void* data);
    static const WeaponLevel* Get();
    static void Finalize();

    int GetLevel(int weaponExperience) const;
};

static_assert(sizeof(WeaponLevel) == 8);

using WeaponRankThresholds = WeaponLevel;
