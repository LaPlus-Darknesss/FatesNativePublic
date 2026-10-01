#pragma once
#include "fates/runtime/native_game_state.hpp"
#include <span>
namespace fates::runtime::native {
struct NativeRuntime;
enum class ForceSkillLookupStatus:std::uint8_t {Ok,MissingOrder,MissingPerson,MissingJob,UnknownPersonArchives};
struct ForceSkillLookupResult {
    ForceSkillLookupStatus status{ForceSkillLookupStatus::Ok};
    std::optional<std::uint16_t> slot;
    std::size_t examined{};
};
// Force::GetUnitFromSkill after original PrivateSkill name -> mask resolution.
// Retains Force order, ORs Unit/Person/Job private flags and then IsUnique.
ForceSkillLookupResult FindForceUnitFromPrivateSkill(const NativeRuntime&,std::uint8_t force,std::uint64_t mask);
// Original Force::IsAllied relation, shared by map and local-aura consumers.
bool ForcesAlliedExact(std::uint8_t a,std::uint8_t b) noexcept;
enum class ForceOrderStatus { Ok,InvalidForce,InvalidUnit,InvalidOrder,MissingOrder,AlreadyMember,NotMember,Capacity };
// Semantic Force::JoinFirst/JoinLast/Remove list effects. Remove does NOT alter
// the Unit's force identity, matching the original unlink routine. Inputs must
// be a bound, structurally valid list; empty lists have count zero.
ForceOrderStatus JoinForceOrderExact(NativeForceOrderState&,std::uint16_t slot,std::uint16_t person,bool last) noexcept;
ForceOrderStatus RemoveForceOrderExact(NativeForceOrderState&,std::uint16_t slot) noexcept;
bool ForceOrderMatches(const NativeGameState&,std::uint8_t force) noexcept;
const NativeForceOrderState* GetVerifiedForceOrder(const NativeGameState&,std::uint8_t force) noexcept;
// A genuinely empty native Force has an unambiguous order. A nonempty unbound
// Force remains unresolved. Never recover a list by sorting UnitPool slots.
ForceOrderStatus InitializeEmptyForceOrder(NativeGameState&,std::uint8_t force) noexcept;
// For a newly constructed occupied Unit with force_type 9, after its fields are
// initialized. This does not reconstruct the original free-pool allocator/order.
ForceOrderStatus JoinFreshUnitForceOrder(NativeGameState&,std::uint16_t slot,std::uint8_t force,bool last=true) noexcept;
// Only Force unlink/relink + identity effects, NOT Unit::Transfer's reset, death,
// clear, icon or map side effects. Higher-level lifecycle owners must supply them.
// Same-force movement still removes/reinserts, as the original TransferImpl does.
ForceOrderStatus MoveUnitForceMembership(NativeGameState&,std::uint16_t slot,std::uint8_t force,bool last) noexcept;
// Compatibility entry points use the same general list owner.
bool PlayerForceOrderMatches(const NativeGameState&) noexcept;
// Called before the existing sortie commit's already-validated Force-3 -> 0
// writes. Unknown prior destination order stays unbound. Known reserve order is
// updated too; neither Force order is guessed from the supplied reserve sequence.
void AppendPlayerSortieForceOrder(NativeGameState&,std::span<const std::uint16_t>) noexcept;
}
