#pragma once

namespace nn::ulcd::CTR {
class StereoCamera;
}

namespace fates::decomp_detail {

// Provisional readable name for the retail pointer slot reached through the
// literal in ICamera::Initialize. Exact retail address is in the sidecar.
extern nn::ulcd::CTR::StereoCamera* gStereoCamera;

} // namespace fates::decomp_detail
