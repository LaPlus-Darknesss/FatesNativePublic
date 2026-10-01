#pragma once

#include "fates/detail/arm32_address.hpp"

#include <array>
#include <cstddef>

struct BattleExperienceEntry {
    std::array<std::byte, 0x48> raw{};
};
struct BattleThresholdExperienceEntry {
    std::array<std::byte, 0x18> raw{};
};
struct RodDeclineExperienceEntry {
    std::array<std::byte, 0x28> raw{};
};
struct RodHighDeclineExperienceEntry {
    std::array<std::byte, 0x04> raw{};
};
struct DanceExperienceEntry {
    std::array<std::byte, 0x28> raw{};
};
struct SupportDeclineExperienceEntry {
    std::array<std::byte, 0x28> raw{};
};

struct ExperienceTableRoot32 {
    fates::decomp_detail::Arm32Address battle{};           // +0x00
    fates::decomp_detail::Arm32Address battleThreshold{};  // +0x04
    fates::decomp_detail::Arm32Address rodDecline{};       // +0x08
    fates::decomp_detail::Arm32Address rodHighDecline{};   // +0x0C
    fates::decomp_detail::Arm32Address dance{};            // +0x10
    fates::decomp_detail::Arm32Address supportDecline{};   // +0x14
};

static_assert(sizeof(ExperienceTableRoot32) == 0x18);
static_assert(sizeof(BattleExperienceEntry) == 0x48);
static_assert(sizeof(BattleThresholdExperienceEntry) == 0x18);
static_assert(sizeof(RodDeclineExperienceEntry) == 0x28);
static_assert(sizeof(RodHighDeclineExperienceEntry) == 0x04);
static_assert(sizeof(DanceExperienceEntry) == 0x28);
static_assert(sizeof(SupportDeclineExperienceEntry) == 0x28);

class ExpTable {
public:
    static void Initialize(const void* data);
    static RodDeclineExperienceEntry* GetRodDecline(int index);
    static RodHighDeclineExperienceEntry* GetRodHighDecline(int index);
    static SupportDeclineExperienceEntry* GetSupportDecline(int index);
    static BattleThresholdExperienceEntry* GetBattleThreshold(int index);
    static void Finalize();
    static DanceExperienceEntry* GetDance(int index);
    static BattleExperienceEntry* GetBattle(int index);
};
