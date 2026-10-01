#pragma once

#include <cstddef>

namespace nn::ulcd::CTR {

class StereoCamera {
public:
    StereoCamera();
    void Initialize();

private:
    // ICamera::Initialize allocates exactly 0x64 bytes before invoking the
    // retail constructor. Keep the SDK object opaque until its fields matter.
    std::byte opaque_[0x64]{};
};

static_assert(sizeof(StereoCamera) == 0x64);

} // namespace nn::ulcd::CTR
