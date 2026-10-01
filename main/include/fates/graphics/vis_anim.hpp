#pragma once
#include "fates/graphics/anim_obj.hpp"
class VisAnim final : public AnimObj {
public:
    VisAnim():AnimObj(AnimConst::Type::Visibility){}
    ~VisAnim() override;
    bool Play(const ResFile& resources,int index) override;
    void Stop() override;
};
