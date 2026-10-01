#include "fates/game/weapon_level.hpp"

namespace {
const WeaponLevel* gWeaponLevels = nullptr;
}

void WeaponLevel::Initialize(const void* data) {
    gWeaponLevels = static_cast<const WeaponLevel*>(data);
}

const WeaponLevel* WeaponLevel::Get() {
    return gWeaponLevels;
}

void WeaponLevel::Finalize() {
    gWeaponLevels = nullptr;
}

int WeaponLevel::GetLevel(int weaponExperience) const {
    int level = 0;
    while (true) {
        if (thresholds[level] <= weaponExperience) {
            return level;
        }
        if (thresholds[level + 1] <= weaponExperience) {
            return level + 1;
        }
        level += 2;
        if (level > 5) {
            return 6;
        }
    }
}
