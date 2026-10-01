#pragma once
#include "fates/graphics/anim_obj.hpp"
class LightAnim final : public AnimObj {
public:
    LightAnim();
    ~LightAnim() override;
    bool Play(const ResFile& resources,int index) override;
    void Stop() override;
    bool Calc();
    void Transform(const nn::math::MTX34& transform);
    const fates::decomp_detail::LightAnimationState& GetLightState() const { return lightState_; }
private:
    fates::decomp_detail::LightAnimationState lightState_{};
};
