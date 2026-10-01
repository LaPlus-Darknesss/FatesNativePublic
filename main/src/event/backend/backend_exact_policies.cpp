#include "fates/event/backend/backend_spine.hpp"
#include <cstring>

namespace fates::event::backend {

bool GameSkipStateIsBlackOut(std::uint8_t state) { return state == 3u; }
bool GameSkipStateIsSkipping(std::uint8_t state) { return state != 0u; }

std::uint32_t PackContentTimestamp(ContentTimestampFields f) {
    return (static_cast<std::uint32_t>(f.year) & 0x0fffu)
        | ((static_cast<std::uint32_t>(f.month) & 0x0fu) << 12)
        | ((static_cast<std::uint32_t>(f.day) & 0x1fu) << 16)
        | ((static_cast<std::uint32_t>(f.hour) & 0x1fu) << 21)
        | ((static_cast<std::uint32_t>(f.minute) & 0x3fu) << 26);
}

ContentTimestampFields UnpackContentTimestamp(std::uint32_t packed) {
    return ContentTimestampFields{
        static_cast<std::uint16_t>(packed & 0x0fffu),
        static_cast<std::uint8_t>((packed >> 12) & 0x0fu),
        static_cast<std::uint8_t>((packed >> 16) & 0x1fu),
        static_cast<std::uint8_t>((packed >> 21) & 0x1fu),
        static_cast<std::uint8_t>((packed >> 26) & 0x3fu),
    };
}

bool SetFieldObjectEscapeBits(std::uint16_t& flags, bool escape) {
    constexpr std::uint16_t kEscape = 0x0010u;
    constexpr std::uint16_t kDirty = 0x0002u;
    const bool was = (flags & kEscape) != 0;
    if (was == escape) return false;
    flags = escape ? static_cast<std::uint16_t>(flags | kEscape)
                   : static_cast<std::uint16_t>(flags & ~kEscape);
    flags = static_cast<std::uint16_t>(flags | kDirty);
    return true;
}

} // namespace fates::event::backend
