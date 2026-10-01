#pragma once
#include "fates/graphics/anim_obj.hpp"
class SkelAnim final : public AnimObj {
public:
    SkelAnim():AnimObj(AnimConst::Type::Skeletal){}
    ~SkelAnim() override;
    bool Play(const ResFile& resources,int index) override;
    void Stop() override;
    void Calc(Model& model,fates::decomp_detail::AnimationBlendState* blend);
};
