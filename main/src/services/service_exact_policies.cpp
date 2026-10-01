#include "fates/services/core_services.hpp"

namespace fates::services {

std::uint8_t DecodeGameMode(std::uint32_t flags) {
    if ((flags & 0x8u) != 0) return 0u;
    if ((flags & 0x4u) != 0) return 1u;
    return 2u;
}

std::uint32_t EncodeGameMode(std::uint32_t flags, std::uint8_t mode) {
    if (mode == 0u) return flags | 0x0cu;
    if (mode == 1u) return (flags & ~0x8u) | 0x4u;
    return flags & ~0x0cu;
}

bool GameSkipIsWait(std::uint8_t state, std::uint32_t flags) {
    return state != 0u && (state != 3u || (flags & 0x5u) != 0u);
}

std::uint32_t GameSkipRequestEscape(std::uint8_t state, std::uint32_t flags) {
    return state == 0u ? flags : (flags | 0x4u);
}

bool MapLoadedByte(std::uint8_t loaded) { return loaded != 0u; }
bool FieldObjectVisible(std::uint8_t flags) { return (flags & 0x4u) == 0u; }
bool FieldObjectAccessPlaying(std::int8_t access_state) { return access_state != static_cast<std::int8_t>(-1); }

bool FadePairIsBlackOut(bool first_exists, std::uint8_t first_alpha, bool second_exists, std::uint8_t second_alpha) {
    return first_exists && second_exists && first_alpha == 0xffu && second_alpha == 0xffu;
}

bool FadeSlotIsActiveFadeIn(bool exists, bool started, bool ending) {
    return exists && started && !ending;
}

bool DeliveryMessageExists(std::int16_t first_word) { return first_word != 0; }

} // namespace fates::services
