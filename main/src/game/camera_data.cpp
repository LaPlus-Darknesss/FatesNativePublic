#include "fates/game/camera_data.hpp"

#include "fates/detail/metadata_runtime.hpp"

void CameraData::Initialize() {
    fates::decomp_detail::LoadStructList(
        fates::decomp_detail::gCameraDataList,
        "GameData/CameraData.bin.lz",
        nullptr,
        0,
        0x44);
}

void CameraData::Finalize() {
    fates::decomp_detail::FreeStructList(
        fates::decomp_detail::gCameraDataList,
        0);
}
