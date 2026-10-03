#pragma once
#include "fates/runtime/native_game_state.hpp"
#include "fates/runtime/native_archive_identifiers.hpp"
#include <span>
namespace fates::runtime::native {
class DefinitionStore;
enum class UnitPersonLookupStatus : std::uint8_t {
    Ok,UnknownPersonArchives,InvalidPersonReference,MissingUnitPerson,
    StaleUnitPerson,InvalidUnit
};
struct UnitPersonLookupResult {
    UnitPersonLookupStatus status{UnitPersonLookupStatus::Ok};
    std::optional<std::uint16_t> slot,unavailable_slot;
    std::uint16_t slots_examined{},uniqueness_checks{},download_checks{};
};
// Explicit carried record admission. Numeric equality is a consistency check,
// never the source of the intended identity. Rejection leaves the Unit untouched.
UnitPersonLookupStatus BindUnitPersonRecord(const DefinitionStore&,UnitState&,const PersonRecordReference&);
// PROVEN 004F5D24: ascending250-slot scan, Force9 exclusion, IsUnique before
// Person identity comparison. No Force order, map/HP or phase dependency.
UnitPersonLookupResult FindUnitFromPerson(const DefinitionStore&,const NativeGameState&,const PersonRecordReference&);
enum class PlayerUnitLookupStatus : std::uint8_t {
    Ready,MissingPlayerFlag,RetiredRegistry,StalePlayerFlag,InvalidPlayerFlag,
    InvalidUnit,MissingPerson,StalePerson,MissingJob,UnknownPersonArchives
};
struct PlayerUnitLookupResult {
    PlayerUnitLookupStatus status{PlayerUnitLookupStatus::Ready};
    std::optional<std::uint16_t> slot,unavailable_slot;
    std::uint16_t slots_examined{},uniqueness_checks{},download_checks{},force_checks{},flag_checks{};
};
// UnitPool::GetPlayer's shared lazy flag cache and ascending250-slot scan.
// This is distinct from Force-list lookup and Command.cmb's female-first PID
// helper. Construct once for a fresh runtime scope; retain it across calls.
// The registry must outlive the selector. A successfully cached scalar mask
// survives the source archive's release, just as the original static does.
class NativePlayerUnitSelector final {
public:
    explicit NativePlayerUnitSelector(const NativeArchiveIdentifiers& identifiers):identifiers_(identifiers) {}
    PlayerUnitLookupResult Find(const DefinitionStore&,const NativeGameState&);
    std::optional<std::uint64_t> cached_mask() const noexcept {return mask_;}
private:
    const NativeArchiveIdentifiers& identifiers_;
    std::optional<std::uint64_t> mask_;
};
enum class UnitClearEffect : std::uint8_t {
    RemoveEditPrivateFlag,DeleteEdit,DeleteFamily,DestroyActor,DeleteOrdinaryPoints,
    ClearOrdinaryCount,DeleteChapterPoints,ClearChapterCount,ResetFields,
    ClearIdentifier,ClearEnhance,ResetSkillPool,ClearAi,ClearCloth,ClearRecord
};
struct UnitClearPresence {bool edit{},family{},actor{},ordinary_points{},chapter_points{};};
struct UnitClearPlan {std::array<UnitClearEffect,15> effects{};std::uint8_t count{};};
UnitClearPlan PlanUnitClearExact(UnitClearPresence) noexcept;
enum class UnitPoolStatus : std::uint8_t {
    Ok,AlreadyBound,UnboundPool,InvalidOrder,InvalidUnit,MissingForceOrder,
    UnresolvedActor,UnresolvedPair,RevisionExhausted,NotEmpty,Exhausted
};
// Explicit carried order; never infer a nonempty free list from slot numbers.
// clear_key is the original current constructor-key global consumed by Clear.
UnitPoolStatus RestoreFreeUnitPoolOrder(NativeGameState&,std::span<const std::uint16_t>,std::uint32_t clear_key);
UnitPoolStatus InitializeFreshUnitPool(NativeGameState&,std::uint32_t clear_key);
bool FreeUnitPoolMatches(const NativeGameState&) noexcept;
std::optional<std::uint16_t> FindFreeUnitPoolSlot(const NativeGameState&) noexcept;
// Called by the existing fresh Force join after construction, before publication.
// When the pool is bound the constructed slot must be the original free head.
UnitPoolStatus CompleteFreshUnitPoolSlot(NativeGameState&,std::uint16_t) noexcept;
// Atomic list removal, native owned-state clear, and prepend/append to Force9.
// Does not implement actor destruction, pair teardown, or whole UnitPool::Reset.
UnitPoolStatus RecycleUnitPoolSlot(NativeGameState&,std::uint16_t,bool last);
}
