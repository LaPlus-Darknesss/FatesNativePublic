#pragma once
#include <cstdint>
#include <string_view>

namespace fates::runtime::native {
// Shared status for the carried event-state projection. Existing flag API
// spellings remain aliases, preserving their enumerator values and overrides.
enum class EventStateStatus : std::uint8_t {
    Ok, InvalidSnapshot, InvalidName, Retired, ForeignPlan, StalePlan,
    ConsumedPlan, RevisionExhausted, UnknownOperation, MissingBank,
    NullRuntime, StalePlayerState, UnknownCommand, ProviderUnavailable
};
inline bool IsValidEventName(std::string_view name) noexcept {
    return name.size()<=65535 && name.find('\0')==std::string_view::npos;
}
}
