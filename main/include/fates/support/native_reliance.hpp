#pragma once
#include "fates/runtime/native_runtime.hpp"
#include "fates/support/native_family_support.hpp"
#include "fates/support/native_local_support.hpp"
namespace fates::support::native {
// PROVEN: descending comparisons even for unordered tables. >=99 bounds
// GetMaxLevel; it does not clamp the separate GetLevel operation.
int RelianceLevelExact(const std::array<std::uint8_t,4>&, int points) noexcept;
int RelianceMaxLevelExact(const std::array<std::uint8_t,4>&) noexcept;
struct OrdinaryRelianceScore { bool found{}; int level{}, points{}, points_to_next{}; };
OrdinaryRelianceScore ResolveOrdinaryRelianceExact(bool eligible,
    const fates::runtime::native::RelianceDefinition*, std::uint8_t points) noexcept;
struct CarriedSupportRelation { OrdinaryRelianceScore score{}; bool is_reliance{}; };
CarriedSupportRelation ResolveCarriedRelianceExact(bool eligible,std::uint16_t subject,std::uint16_t other,
    const SupportFamilyState*,const SupportFamilyState*,const fates::runtime::native::RelianceDefinition*,std::uint8_t points) noexcept;
// Raw carried facts. Unknown is distinct from a known null retail pointer.
// Present Family/Edit requires the corresponding complete semantic payload.
enum class SupportStoragePresence : std::uint8_t { Unknown, Absent, Present };
struct CarriedSupportUnitState {
    std::uint16_t slot{}, person_id{}, job_id{};
    SupportStoragePresence family{}, edit{}, points_storage{};
    std::vector<std::uint8_t> points{};
    std::optional<SupportFamilyState> family_state;
    std::optional<SupportEditState> edit_state;
    bool constructor_key_known{};
    std::uint32_t constructor_key{};
};
struct CarriedSupportSnapshot {
    std::uint32_t situation_flags{}, user_flags{};
    std::array<std::uint8_t,3> control{{1,2,2}};
    // Complete occupied census: inverse records also search reserve forces.
    std::vector<CarriedSupportUnitState> units{};
};
using OrdinarySupportUnitState=CarriedSupportUnitState;
using OrdinarySupportSnapshot=CarriedSupportSnapshot;
enum class SupportProviderStatus : std::uint8_t {
    Ok, InvalidContext, IncompleteUnitCensus, MissingDefinition,
    MissingFamilyState, UnsupportedFamily, MissingEditState, UnsupportedEdit,
    UnsupportedDownloadIdentity, MissingPointStorage, PointIndexOutOfRange,
    MissingBonusTable, MissingPersonalityDefinition, RevisionExhausted
};
SupportProviderStatus RestoreCarriedSupportSnapshot(
    fates::runtime::native::NativeRuntime&, const CarriedSupportSnapshot&);
// Ordinary-only compatibility entry: refuses present Family/Edit.
// Atomically derive and bind relations and totals. Does not deserialize saves,
// fabricate point storage, draw constructor RNG, or implement support growth.
SupportProviderStatus RestoreOrdinarySupportSnapshot(
    fates::runtime::native::NativeRuntime&, const OrdinarySupportSnapshot&);
bool OrdinarySupportSourceMatches(const fates::runtime::native::NativeRuntime&);
}
