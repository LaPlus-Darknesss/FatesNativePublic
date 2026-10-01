#pragma once
#include <cstdint>
#include "fates/detail/animation_runtime.hpp"
#include "fates/io/res_file.hpp"

namespace AnimConst {
enum class Type : std::uint8_t { Skeletal=0, Material=1, Visibility=2, Camera=3, Light=4 };
}
class Model;
class AnimCtrl;

class AnimObj {
public:
    explicit AnimObj(AnimConst::Type type);
    virtual ~AnimObj();
    virtual bool Play(const ResFile& resources,int index)=0;
    virtual void Stop();

    bool Alloc(const nw::h3d::res::AnimContent* content,const ResFile& resources);
    bool Free();
    void UpdateFrame(float deltaFrame);
    void SetFrame(float frame);
    float GetFrame() const { return frame_; }
    float GetStepFrame() const { return stepFrame_; }
    void SetStepFrame(float step) { stepFrame_=step; }
    float GetEndFrame() const;
    bool IsFinished() const;
    bool IsLoop() const;
    bool IsRewound() const;
    AnimConst::Type GetType() const { return type_; }

protected:
    friend class AnimCtrl;
    enum Flag : std::uint16_t { Fresh=0x1, Wrapped=0x2, Rewound=0x4, Finished=0x8 };
    const nw::h3d::res::AnimContent* Content() const { return content_; }
    fates::decomp_detail::AnimationBindingState& Binding() { return binding_; }
    const fates::decomp_detail::AnimationBindingState& Binding() const { return binding_; }
    void ConfigureBlend(float start,float target,float frames);
    void AdvanceBlend(float deltaFrame);
    bool BlendActive() const { return blendFramesRemaining_>0.0f; }

    AnimConst::Type type_;
    std::uint8_t group_{};
    std::uint16_t flags_{};
    const nw::h3d::res::AnimContent* content_{};
    float frame_{};
    float previousFrame_{-1.0f};
    float stepFrame_{1.0f};
    float blendWeight_{1.0f};
    float blendStart_{1.0f};
    float blendTarget_{1.0f};
    float blendFramesRemaining_{};
    float blendFramesTotal_{};
    fates::decomp_detail::AnimationBindingState binding_{};
    ResFile resources_{};
};
