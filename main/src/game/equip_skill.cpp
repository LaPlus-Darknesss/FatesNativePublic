#include "fates/game/equip_skill.hpp"

#include "fates/detail/core_game_data_runtime.hpp"
#include "fates/engine/ident_hash.hpp"

#include <cstdint>

namespace {

EquipSkill* gEquipSkills = nullptr;
int gCommonEquipSkillCount = 0;
int gEquipSkillCount = 0;

std::int32_t RetailSortKey(const EquipSkill& skill) {
    // Exact 0x0017588C behavior: signed low halfword OR unsigned high
    // halfword shifted into bits 16..31.
    return static_cast<std::int32_t>(
        static_cast<std::int16_t>(skill.id)) |
        static_cast<std::int32_t>(
            static_cast<std::uint32_t>(
                static_cast<std::uint16_t>(skill.sortOrder)) << 16);
}

} // namespace

void EquipSkill::Initialize(
    const void* data,
    int commonCount,
    int totalCount) {
    gEquipSkills = const_cast<EquipSkill*>(
        static_cast<const EquipSkill*>(data));
    gCommonEquipSkillCount = commonCount;
    gEquipSkillCount = totalCount;
}

bool EquipSkill::SortCompare(short lhs, short rhs, void*) {
    if (rhs <= 0) {
        return false;
    }
    if (lhs < 1) {
        return true;
    }
    return RetailSortKey(*Get(rhs)) < RetailSortKey(*Get(lhs));
}

int EquipSkill::GetCommonNum() {
    return gCommonEquipSkillCount;
}

EquipSkill* EquipSkill::Get(const char* identifier) {
    return static_cast<EquipSkill*>(
        fates::decomp_detail::gEquipSkillIdentHash->GetSurely(identifier));
}

EquipSkill* EquipSkill::Get(short skillId) {
    return reinterpret_cast<EquipSkill*>(
        reinterpret_cast<std::byte*>(gEquipSkills) +
        static_cast<std::ptrdiff_t>(skillId) * 0x20);
}

int EquipSkill::GetNum() {
    return gEquipSkillCount;
}

void EquipSkill::Finalize() {
    if (gEquipSkills != nullptr) {
        gCommonEquipSkillCount = 0;
        gEquipSkillCount = 0;
        gEquipSkills = nullptr;
    }
}
