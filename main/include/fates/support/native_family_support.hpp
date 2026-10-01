#pragma once
#include "fates/runtime/native_definition_store.hpp"
#include <optional>
namespace fates::support::native {
// Carried semantic facts, not copies of pointer-bearing retail layouts. Missing
// references are known null; the containing snapshot separately tracks unknown.
struct SupportEditState { std::uint8_t boon{}, bane{}; bool operator==(const SupportEditState&) const=default; };
struct SupportParentState {
    std::optional<std::uint16_t> person, father, mother;
    SupportEditState edit{};
    std::uint8_t points{};
    bool operator==(const SupportParentState&) const=default;
};
struct SupportFamilyState {
    std::array<SupportParentState,2> parents{};
    std::uint8_t sibling_points{};
    // Original Family+11,+25,+29 are chapter counters, distinct from the
    // persistent support points at+10,+24,+28. Existing support queries ignore them.
    bool chapter_points_known{};
    std::array<std::uint8_t,3> chapter_points{};
    bool operator==(const SupportFamilyState&) const=default;
};
enum class FamilyRelationshipKind : std::uint8_t { None, ParentChild, Siblings };
struct FamilySupportRelationship {
    FamilyRelationshipKind kind{};
    std::uint8_t child{}, points{}; // child: 1=subject, 2=other; zero for siblings
};
int FamilySupportLevelExact(int points) noexcept;
FamilySupportRelationship ResolveFamilySupportExact(bool eligible,
    std::uint16_t subject, std::uint16_t other,
    const SupportFamilyState* subject_family, const SupportFamilyState* other_family) noexcept;
bool IsUncleAuntSupportExact(std::uint16_t subject, std::uint16_t other,
    const SupportFamilyState*, const SupportFamilyState*) noexcept;
// Signed Personality+0x3c selects the four attack-stance capabilities; -1 has
// no transfer. Rank zero contributes no edit transfer. Integer division truncates.
int PersonEditSupportExact(const std::array<std::int8_t,20>& table,
    int capability, int level, int boon_capability, int bane_capability) noexcept;
int FamilyAttackStanceTotalExact(const std::array<std::int8_t,20>& first,
    const std::array<std::int8_t,20>& second, int capability, int level,
    const std::array<int,2>& first_edit, const std::array<int,2>& second_edit) noexcept;
enum class CarriedBonusStatus : std::uint8_t { Ok, MissingPerson, MissingTable, MissingPersonality };
CarriedBonusStatus ResolveCarriedSupportBonuses(const fates::runtime::native::DefinitionStore&,
    std::uint16_t person, const SupportFamilyState*, const SupportEditState*,
    std::array<std::array<std::int16_t,4>,5>& result);
}
