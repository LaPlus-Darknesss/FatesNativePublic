#pragma once
#include "fates/runtime/native_game_state.hpp"
#include <optional>
#include <span>
namespace fates::runtime::native {
struct NativeRuntime;
struct UnitPairEligibilityFacts {
    std::uint64_t unit_flags{},person_flags{},job_flags{};
    bool linked{};
    std::uint8_t force{};
};
// Original CanDoubleOn predicate. It does not check distance, occupancy, actor
// presentation, self-pairing or whether a player command is currently allowed.
bool CanPairUnitsExact(const UnitPairEligibilityFacts&,const UnitPairEligibilityFacts&,
                       std::uint64_t prohibited_mask) noexcept;
std::optional<bool> CanPairCurrentUnits(const NativeRuntime&,std::uint16_t,std::uint16_t) noexcept;
enum class UnitPairStatus : std::uint8_t {
    Ok,InvalidUnit,SameUnit,ExistingPair,MalformedPair,InvalidCoordinates,RevisionExhausted
};
struct UnitPairRequest { std::uint16_t lead{},partner{}; };
// These own DoubleOn/DoubleOff data effects. Callers own CanDoubleOn admission,
// tactical image removal/addition, rescue-position selection and actor effects.
// Linking invalidates relationship-derived bonuses; neither operation resets
// the two unit-owned guard gauge bytes or changes RNG/Force membership.
UnitPairStatus LinkUnitPair(NativeGameState&,std::uint16_t lead,std::uint16_t partner) noexcept;
UnitPairStatus UnlinkUnitPair(NativeGameState&,std::uint16_t lead) noexcept;
UnitPairStatus LinkUnitPairs(NativeGameState&,std::span<const UnitPairRequest>);
}
