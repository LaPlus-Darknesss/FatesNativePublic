#pragma once
#include "fates/graphics/anim_obj.hpp"
class MatAnim final : public AnimObj {
public:
    MatAnim():AnimObj(AnimConst::Type::Material){}
    ~MatAnim() override;
    bool Play(const ResFile& resources,int index) override;
    void Stop() override; // exact retail entry is a four-byte no-op
};
