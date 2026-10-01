#include "fates/game/weapon_data.hpp"

#include <cstddef>
#include <cstdint>

namespace {

WeaponBonus* gWeaponBonuses = nullptr;
const WeaponInteractionTable* gWeaponInteractions = nullptr;

} // namespace

void WeaponBonus::Initialize(const void* data) {
    gWeaponBonuses = const_cast<WeaponBonus*>(
        static_cast<const WeaponBonus*>(data));
}

WeaponBonus* WeaponBonus::Get(ItemKind::Type type) {
    const auto index = static_cast<std::int32_t>(type);
    return reinterpret_cast<WeaponBonus*>(
        reinterpret_cast<std::byte*>(gWeaponBonuses) +
        static_cast<std::ptrdiff_t>(index) * 0x0C);
}

void WeaponBonus::Finalize() {
    if (gWeaponBonuses != nullptr) {
        gWeaponBonuses = nullptr;
    }
}

void WeaponInteract::Initialize(const void* data) {
    gWeaponInteractions =
        static_cast<const WeaponInteractionTable*>(data);
}

const WeaponInteractionTable* WeaponInteract::Get() {
    return gWeaponInteractions;
}

void WeaponInteract::Finalize() {
    if (gWeaponInteractions != nullptr) {
        gWeaponInteractions = nullptr;
    }
}
