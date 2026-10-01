#pragma once
#include "fates/runtime/native_runtime.hpp"
#include "fates/support/native_family_support.hpp"
namespace fates::support::native {
using GuardTable=std::array<std::int8_t,40>;
struct GuardEdit {std::array<std::uint8_t,3> boon{},bane{};bool enabled{};};
int PersonGuardTotalExact(const GuardTable&,int capability,int level) noexcept;
int PersonGuardEditExact(const GuardTable&,int capability,int level,const GuardEdit&) noexcept;
int FamilyGuardTotalExact(const GuardTable& first,const GuardTable& second,int capability,int level,
    const GuardEdit& first_edit={},const GuardEdit& second_edit={}) noexcept;
CarriedBonusStatus ResolveCarriedGuardBonuses(const fates::runtime::native::DefinitionStore&,std::uint16_t person,
    const SupportFamilyState*,const SupportEditState*,std::array<std::array<std::int16_t,8>,5>&);
enum class PairBonusStatus : std::uint8_t {
    Ok,InvalidPair,MissingDefinition,MissingSupport,StaleSupport,MissingRelation,
    UnboundLineage,MissingTable,MissingPersonality,UnresolvedSkill
};
struct PairBonusProjection {
    PairBonusStatus status{PairBonusStatus::Ok};
    std::uint16_t source_slot{0xffff};
    int support_level{};
    std::array<std::int16_t,8> total{};
};
// Bonus-source is the reciprocal partner; the receiving unit owns Guardian.
// Derives on every query from current definitions, carried lineage and support.
// Does not publish a cache, mutate stats, perform placement or consume RNG.
PairBonusProjection ProjectCurrentPairBonuses(const fates::runtime::native::NativeRuntime&,std::uint16_t receiver);
// Original GetDoubleCapability's source/receiver calculation. Pointer selection
// and pair admission belong to its caller. No force, role, HP or defeat filter;
// current support/lineage identity and definition proofs are still required.
PairBonusProjection ProjectCurrentGuardBonusesFromSource(const fates::runtime::native::NativeRuntime&,
    std::uint16_t receiver,std::uint16_t source);
}
