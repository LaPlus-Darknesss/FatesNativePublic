#pragma once
#include <array>
#include <cstdint>
#include <vector>
namespace fates::runtime::native {
// Persisted constructor/campaign facts missing from the bounded ROMFS constructor.
// No fallback to pool order, fresh RNG, or an invented no-support relationship.
struct ResolvedSupportUnitInput {
    std::uint16_t slot{}, person_id{}, job_id{};
    bool tie_key_known{};
    std::uint32_t constructor_tie_key{};
    bool bonus_totals_known{};
    // Resolved Unit::GetDualSupport totals for levels 0..4, Hit/Crit/Avoid/Dodge.
    // Family/edit-derived contributions must be supplied by their real owner.
    std::array<std::array<std::int16_t,4>,5> bonus_totals{};
};
struct ResolvedSupportRelationInput {
    std::uint16_t subject{}, source{}, subject_person{}, source_person{};
    bool score_relation_found{}, is_reliance{};
    std::int32_t level{}, points_to_next{};
};
struct SupportSourceUnitGuard {
    std::uint16_t slot{}, person_id{}, job_id{};
    std::uint8_t force_type{};
    std::uint32_t uniqueness_flags{};
    std::uint64_t lineage_revision{};
    std::uint64_t transfer_revision{};
    std::uint64_t slot_generation{};
};
struct ResolvedSupportContext {
    bool ordinary_source_bound{};
    std::uint64_t definition_revision{};
    std::vector<SupportSourceUnitGuard> source_units{};
    bool bound{};
    std::uint32_t situation_flags{}, user_flags{};
    std::array<std::uint8_t,3> control{{1,2,2}};
    std::uint64_t phase_revision{};
    std::uint8_t chapter_index{};
    std::vector<ResolvedSupportUnitInput> units{};
    std::vector<ResolvedSupportRelationInput> relations{};
};
}
