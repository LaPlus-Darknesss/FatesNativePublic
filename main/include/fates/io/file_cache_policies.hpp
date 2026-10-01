#pragma once
#include <bit>
#include <cstdint>
namespace fates::io {
inline constexpr std::uint32_t FileAsyncMask=0x60000000u;
constexpr bool FileSweepEligible(std::uint8_t level,std::uint16_t references,
    std::uint32_t flags,std::uint32_t sweep_level) noexcept {
    return level==sweep_level && references==0 && !(flags&FileAsyncMask);
}
constexpr std::int32_t FileSweepSubtract(std::int32_t budget,std::uint32_t size) noexcept {
    return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(budget)-size);
}
constexpr std::uint16_t FileReleaseReference(std::uint16_t references) noexcept {
    return static_cast<std::uint16_t>(references-1u);
}
constexpr bool FileKeepAfterRelease(bool data,std::uint8_t level,std::uint32_t flags) noexcept {
    return data && level && !(flags&0x18000000u);
}
}
