#include "fates/graphics/tex_command.hpp"

#include "fates/detail/tex_command_runtime.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace {

constexpr std::array<std::uint16_t, 3> kTextureSizeRegisters{
    0x0082, 0x0092, 0x009A,
};

constexpr std::array<std::uint16_t, 3> kTextureAddressRegisters{
    0x0085, 0x0095, 0x009D,
};

std::uint32_t FindValue(
    int textureUnit,
    const nw::h3d::fnd::DataArray<unsigned int>& commands,
    const std::array<std::uint16_t, 3>& registers) {
    std::uint32_t value = 0;
    const auto index = static_cast<std::size_t>(textureUnit);
    if (!fates::decomp_detail::TryFindDrawCommandValue(
            commands,
            registers[index],
            value)) {
        return 0;
    }
    return value;
}

} // namespace

namespace TexCommand {

unsigned int GetAddress(
    int textureUnit,
    const nw::h3d::fnd::DataArray<unsigned int>& commands) {
    // PICA texture addresses are stored in eight-byte units.
    return FindValue(textureUnit, commands, kTextureAddressRegisters) << 3;
}

unsigned int GetWidth(
    int textureUnit,
    const nw::h3d::fnd::DataArray<unsigned int>& commands) {
    return FindValue(textureUnit, commands, kTextureSizeRegisters) >> 16;
}

unsigned int GetHeight(
    int textureUnit,
    const nw::h3d::fnd::DataArray<unsigned int>& commands) {
    return FindValue(textureUnit, commands, kTextureSizeRegisters) & 0xFFFFu;
}

} // namespace TexCommand
