#pragma once
#include "fates/graphics/anim_obj.hpp"
class CamAnim final : public AnimObj {
public:
    CamAnim();
    ~CamAnim() override;
    bool Play(const ResFile& resources,int index) override;
    void Stop() override;
    bool Calc();
    const fates::decomp_detail::CameraAnimationState& GetCameraState() const { return cameraState_; }
private:
    fates::decomp_detail::CameraAnimationState cameraState_{};
};
