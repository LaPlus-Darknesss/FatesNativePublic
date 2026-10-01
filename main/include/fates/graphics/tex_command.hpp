#pragma once

#include <cstdint>

namespace nw::h3d::fnd {
template <typename T>
class DataArray;
}

namespace TexCommand {

// Retail supports three texture units. Address values are stored in PICA
// command words in 8-byte units; width/height share each unit's size register.
unsigned int GetAddress(
    int textureUnit,
    const nw::h3d::fnd::DataArray<unsigned int>& commands);
unsigned int GetWidth(
    int textureUnit,
    const nw::h3d::fnd::DataArray<unsigned int>& commands);
unsigned int GetHeight(
    int textureUnit,
    const nw::h3d::fnd::DataArray<unsigned int>& commands);

} // namespace TexCommand
