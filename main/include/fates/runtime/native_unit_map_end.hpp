#pragma once
#include <array>
#include <cstdint>

namespace fates::runtime::native {
struct UnitState;
struct NativeRuntime;

// The 23-byte Enhance payload is split by behavior. UnitState::weakness remains
// the sole persistent owner of the final eight lanes; snapshots are value inputs.
struct UnitEnhanceSnapshot {
    std::array<std::uint8_t,7> flags{};
    std::array<std::uint8_t,8> values{}, weakness{};
    bool operator==(const UnitEnhanceSnapshot&) const=default;
};
struct NativeUnitEnhanceState {
    bool bound{};
    std::array<std::uint8_t,7> flags{};
    std::array<std::uint8_t,8> values{};
};
enum class UnitEnhanceCleanup : std::uint8_t { ResetCondition, Dead, Clear };
void ResetEnhanceConditionsExact(UnitEnhanceSnapshot&) noexcept;
void ApplyEnhanceDeathExact(UnitEnhanceSnapshot&) noexcept;
void ClearEnhanceExact(UnitEnhanceSnapshot&) noexcept;

// Unknown gameplay meanings stay unnamed. These are additional map-end fields,
// not the chapter-end scratch owner. Original offsets document evidence only.
struct UnitMapEndFields {
    std::uint32_t secondary_flags{}; // Unit+C
    std::array<std::uint8_t,2> previous_position{{255,255}}; // Unit+F6,F7
    std::array<std::uint8_t,3> counters{}; // Unit+12D,132,133
    bool operator==(const UnitMapEndFields&) const=default;
};
struct NativeUnitMapEndState {
    bool bound{};
    UnitMapEndFields fields{};
};
struct UnitMapEndSnapshot {
    std::uint32_t flags{};
    std::uint64_t private_flags{};
    std::array<std::uint8_t,2> position{{255,255}};
    UnitMapEndFields fields{};
    UnitEnhanceSnapshot enhance{};
    // Oracle-facing resolved reference only. Persistent native exclusions are
    // already owned by UnitState::attack_restrictions, never duplicated here.
    std::uint32_t excluded_person_reference{};
    std::uint8_t excluded_forces{};
    std::uint8_t guard_progress{}; // existing UnitState::pair owner in native state
    bool operator==(const UnitMapEndSnapshot&) const=default;
};
struct UnitMapEndContext {
    std::uint8_t force{};
    bool person_resident{};
    std::uint64_t person_private_flags{},job_private_flags{};
};
// Cached PrivateSkill IDs in original GameData: SPID_敗北条件=5,
// SPID_敗北条件離脱=47. Only the Unit-owned first mask can be removed.
void ResetUnitEndOfMapExact(UnitMapEndSnapshot&,const UnitMapEndContext&,
    bool clear_position,bool clear_enhance) noexcept;

enum class UnitMapEndStatus : std::uint8_t {
    Ok,InvalidUnit,AlreadyBound,UnboundMapEnd,UnboundEnhance,
    MissingDefinition,UnboundPersonArchives,RevisionExhausted,InvalidOperation
};
UnitEnhanceSnapshot CurrentUnitEnhanceSnapshot(const UnitState&) noexcept;
// Explicit carried-state providers; neither API is a save parser or constructor.
UnitMapEndStatus RestoreUnitMapEndFields(UnitState&,const UnitMapEndFields&) noexcept;
UnitMapEndStatus RestoreUnitEnhance(UnitState&,const UnitEnhanceSnapshot&) noexcept;
UnitMapEndStatus CleanupUnitEnhance(UnitState&,UnitEnhanceCleanup) noexcept;
// Commits the original reset fields atomically after resolving the dependencies
// used by that branch. Cached combat values are invalidated, not recomputed.
// Transfer, GetMHPImpl, free-pool allocation and presentation remain separate.
UnitMapEndStatus ResetUnitEndOfMap(NativeRuntime&,std::uint16_t unit_slot,
    bool clear_position,bool clear_enhance) noexcept;
}
