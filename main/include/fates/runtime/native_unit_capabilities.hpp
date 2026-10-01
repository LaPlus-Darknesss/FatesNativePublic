#pragma once
#include "fates/runtime/native_unit_stat_state.hpp"
namespace fates::runtime::native {
struct UnitState;
struct NativeRuntime;
enum class UnitCapabilityStatus : std::uint8_t {
    Ok,InvalidUnit,InvalidCapability,MissingDefinition,MissingPersonality,
    UnboundLineage,StaleLineage,UnboundCapability,StaleCapability,UnboundEnhance,
    UnresolvedSkill,AlreadyBound,RevisionExhausted,UnboundInventory,StaleInventory,
    MissingItemDefinition,MalformedPair,UnresolvedPairBonus,UnresolvedHeldEnhancement
};
struct UnitCapabilityResult {
    UnitCapabilityStatus status{UnitCapabilityStatus::Ok};
    int value{};
};
struct UnitLineageRestorePlan {
    UnitCapabilityStatus status{UnitCapabilityStatus::Ok};
    NativeUnitLineageState state{};
    bool changed{},stat_inputs_changed{};
};
// Rebinding is an explicit carried-state replacement. Planning allows the
// support provider to include it in its existing all-or-nothing transaction.
UnitLineageRestorePlan PlanUnitLineageRestore(const DefinitionStore&,const UnitState&,const UnitLineageSnapshot&) noexcept;
UnitCapabilityStatus RestoreUnitLineageSnapshot(NativeRuntime&,std::uint16_t,const UnitLineageSnapshot&) noexcept;
UnitCapabilityStatus RestoreUnitCapabilitySnapshot(NativeRuntime&,std::uint16_t,const UnitCapabilitySnapshot&) noexcept;
UnitCapabilityStatus InvalidateUnitCapabilitySnapshot(NativeRuntime&,std::uint16_t) noexcept;

UnitCapabilityResult ProjectEditCapability(const DefinitionStore&,const fates::support::native::SupportEditState&,std::uint8_t capability,bool limit) noexcept;
UnitCapabilityResult ProjectFamilyCapabilityLimit(const DefinitionStore&,const fates::support::native::SupportFamilyState&,std::uint8_t capability) noexcept;
UnitCapabilityResult ProjectCurrentCapabilityLimit(const NativeRuntime&,const UnitState&,std::uint8_t capability) noexcept;
// Shared signed Person/Job/current/Edit base and public40000000 penalty, capped
// by the existing current limit owner. No enhancement or cached combat stat.
UnitCapabilityResult ProjectCurrentBaseCapability(const NativeRuntime&,const UnitState&,std::uint8_t capability) noexcept;
// Original GetMHPImpl ignores its second boolean; only effects and the explicit
// Enhance type are semantic inputs. Current difficulty/equipment and Enhance
// flags are read from their existing owners, never from a cached max_hp value.
UnitCapabilityResult ProjectCurrentMaximumHp(const NativeRuntime&,const UnitState&,bool effects=true,std::int32_t enhance_type=0) noexcept;
// Publishes only the derived HP ceiling. It neither heals current HP nor marks
// the other seven combat capabilities as recomputed.
UnitCapabilityStatus RefreshUnitMaximumHp(NativeRuntime&,std::uint16_t) noexcept;
}
