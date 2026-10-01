#include "fates/game/experience.hpp"

#include <cstddef>

namespace {

const ExperienceTableRoot32* gExperienceTable = nullptr;

template <typename T>
T* TableAt(
    fates::decomp_detail::Arm32Address address,
    int index,
    std::size_t stride) {
    auto* const base =
        fates::decomp_detail::MutableTargetPointer<std::byte>(address);
    return reinterpret_cast<T*>(
        base + static_cast<std::ptrdiff_t>(index) *
            static_cast<std::ptrdiff_t>(stride));
}

} // namespace

void ExpTable::Initialize(const void* data) {
    gExperienceTable =
        static_cast<const ExperienceTableRoot32*>(data);
}

RodDeclineExperienceEntry* ExpTable::GetRodDecline(int index) {
    return TableAt<RodDeclineExperienceEntry>(
        gExperienceTable->rodDecline,
        index,
        0x28);
}

RodHighDeclineExperienceEntry* ExpTable::GetRodHighDecline(int index) {
    return TableAt<RodHighDeclineExperienceEntry>(
        gExperienceTable->rodHighDecline,
        index,
        0x04);
}

SupportDeclineExperienceEntry* ExpTable::GetSupportDecline(int index) {
    return TableAt<SupportDeclineExperienceEntry>(
        gExperienceTable->supportDecline,
        index,
        0x28);
}

BattleThresholdExperienceEntry* ExpTable::GetBattleThreshold(int index) {
    return TableAt<BattleThresholdExperienceEntry>(
        gExperienceTable->battleThreshold,
        index,
        0x18);
}

void ExpTable::Finalize() {
    if (gExperienceTable != nullptr) {
        gExperienceTable = nullptr;
    }
}

DanceExperienceEntry* ExpTable::GetDance(int index) {
    return TableAt<DanceExperienceEntry>(
        gExperienceTable->dance,
        index,
        0x28);
}

BattleExperienceEntry* ExpTable::GetBattle(int index) {
    return TableAt<BattleExperienceEntry>(
        gExperienceTable->battle,
        index,
        0x48);
}
