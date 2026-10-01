#include "fates/event/native_phase_event_filter.hpp"

namespace fates::event::native {
bool PhaseEventMatchesExact(PhaseEventFilter filter, std::uint16_t turn, std::uint8_t active_force) noexcept {
    if (filter.force >= 0 && filter.force != active_force) return false;
    if (filter.first_turn > 0 && filter.first_turn > turn) return false;
    return filter.last_turn <= 0 || turn <= filter.last_turn;
}
}
