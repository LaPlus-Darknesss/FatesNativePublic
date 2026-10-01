#pragma once

#include <cstdint>

namespace nw::h3d::fnd {
template <typename T>
class DataArray;
}

namespace fates::decomp_detail {

// Adapts Nintendo H3D's DataArray/DrawCommandIterator representation to a
// simple register-value query. TexCommand owns the Fates/PICA register mapping
// and value decoding.
bool TryFindDrawCommandValue(
    const nw::h3d::fnd::DataArray<unsigned int>& commands,
    std::uint16_t registerId,
    std::uint32_t& value);

} // namespace fates::decomp_detail
