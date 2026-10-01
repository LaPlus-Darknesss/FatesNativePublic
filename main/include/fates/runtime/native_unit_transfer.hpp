#pragma once
#include "fates/runtime/native_unit_transfer_state.hpp"
#include "fates/runtime/native_unit_capabilities.hpp"
#include "fates/runtime/native_unit_map_end.hpp"
#include <array>
#include <span>
namespace fates::runtime::native {
struct NativeRuntime;
enum class UnitTransferEffect : std::uint8_t {
    ResetMapEnd,DeathCleanup,Heal,DestroyMapActor,DeleteChapterPoints,
    ClearFamilyChapterPoints,ClearChapterCounter,Remove,ClearUnit,
    JoinFirst,JoinLast,RefreshMapActorIcon
};
struct UnitTransferFacts {
    std::uint8_t source{},destination{};
    bool last{},clear_enhance{true};
    std::uint32_t public_flags{};
    bool map_actor{},chapter_points{},family{};
};
struct UnitTransferPlan {
    bool valid{};
    std::uint8_t destination{};
    bool clear_enhance{};
    std::array<UnitTransferEffect,10> effects{};
    std::uint8_t count{};
};
// Complete bounded Force0..9 transfer policy, including late clone redirection.
// Effect execution remains the responsibility of the concrete native owners.
UnitTransferPlan PlanUnitTransferExact(const UnitTransferFacts&) noexcept;
struct ForceTransferUnit {std::uint16_t slot{};std::uint32_t public_flags{};};
struct ForceTransferRequest {std::uint16_t slot{};std::uint8_t destination{};bool last{};};
// Caller supplies the verified original head-to-tail order. Distinct source and
// destination are required by the concrete batch owner below.
std::vector<ForceTransferRequest> BuildForceTransferRequestsExact(std::span<const ForceTransferUnit>,std::uint8_t destination,bool last);
enum class UnitTransferStatus : std::uint8_t {
    Ok,InvalidUnit,InvalidForce,AlreadyBound,UnboundTransfer,StaleTransfer,
    InvalidSnapshot,MissingForceOrder,UnresolvedMapActor,FreePoolRequired,
    UnresolvedMapEnd,UnresolvedCapability,RevisionExhausted,SameForceBatch,
    UnresolvedPool,UnresolvedPair
};
struct UnitTransferResult {
    UnitTransferStatus status{UnitTransferStatus::Ok};
    std::uint16_t failed_slot{0xffffu};
    std::uint16_t units_moved{};
    UnitMapEndStatus map_end_status{UnitMapEndStatus::Ok};
    UnitCapabilityStatus capability_status{UnitCapabilityStatus::Ok};
};
UnitTransferStatus RestoreUnitTransferSnapshot(NativeRuntime&,std::uint16_t,const UnitTransferSnapshot&);
UnitTransferStatus InvalidateUnitTransferSnapshot(NativeRuntime&,std::uint16_t) noexcept;
// All effects are staged before publishing game state. Known map-actor absence
// and native forces0..8 are supported. Destination9 requires the shared bound
// free pool and no live pair links; actor destruction remains external.
UnitTransferResult TransferUnit(NativeRuntime&,std::uint16_t,std::uint8_t destination,bool last=true,bool clear_enhance=true);
// Mirrors the original head/next+JoinLast or tail/previous+JoinFirst traversal.
// Nonempty same-force batches would revisit relinked nodes in the original loop.
UnitTransferResult TransferForceUnits(NativeRuntime&,std::uint8_t source,std::uint8_t destination,bool last=true);
}
