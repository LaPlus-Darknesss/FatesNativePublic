#pragma once
#include "fates/runtime/native_talk_text.hpp"
namespace fates::runtime::native {
enum class TalkArgumentStatus:std::uint8_t {Ready,Unavailable,ReadLimit,Overflow};
struct TalkDecimalResult {
    TalkArgumentStatus status{TalkArgumentStatus::Unavailable};
    std::uint32_t value{}; // Original modulo-2^32 Decimalize, not signed overflow.
    std::size_t next{},consumed{};
};
struct TalkArgumentColor {
    std::array<std::uint8_t,4> bytes{};
    std::bitset<4> known;
};
struct TalkColorArgumentResult {
    TalkArgumentStatus status{TalkArgumentStatus::Unavailable};
    TalkArgumentColor color;
    std::size_t next{};
};
// Exact bounded Decimalize and GetColor8. Optional '-' and decimal digits only;
// GetColor8 consumes a separator even when it is NUL/punctuation. Empty fields
// retain their input byte/knownness. No whitespace, '+' or clamp-to-255 policy.
TalkDecimalResult TalkDecimalize(const NativeTalkTokens::Reader&,std::size_t);
TalkColorArgumentResult TalkGetColor8(const NativeTalkTokens::Reader&,std::size_t,TalkArgumentColor);
}
