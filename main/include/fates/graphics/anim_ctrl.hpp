#pragma once
#include <array>
#include <memory>
#include <vector>
#include "fates/graphics/anim_obj.hpp"
class Model;
class AnimCtrl {
public:
    AnimCtrl();
    virtual ~AnimCtrl();
    virtual void OnPlayAnim(AnimObj* anim);
    virtual void OnStopAnim(AnimObj* anim);
    void DeleteAnim(AnimObj* anim);
    void RandomAnim();
    bool TryPlayAll(const ResFile& resources,const char* identifier);
    AnimObj* PlayMatAnim(const ResFile& resources,int index);
    AnimObj* PlayVisAnim(const ResFile& resources,int index);
    AnimObj* PlaySkelAnim(const ResFile& resources,int index,float blendTime=0.0f);
    void SetAnimFrame(float frame);
    void SetStepFrame(float step);
    void SetFrameToEnd();
    void SetFrameToFinish();
    bool PlayAll(const ResFile& resources,const char* identifier);
    void SetLoop(bool loop);
    void CalcAnim(Model* model,float deltaFrame);
    void FreeAnim();
    AnimObj* AllocAnim(AnimConst::Type type);
    void ResetAnim();
    void StopAll();
    void SweepAnim();
    AnimObj* GetAnimObj(AnimConst::Type type) const;
    bool IsFinished() const;
    float GetAnimFrame() const;
    float GetRemainFrame() const;
    bool IsLoop() const;
private:
    using Group=std::vector<AnimObj*>;
    std::vector<std::unique_ptr<AnimObj>> owned_{};
    std::array<Group,4> groups_{};
    fates::decomp_detail::AnimationBlendState blendState_{};
    Model* model_{};
    std::uint8_t blendMode_{};
    std::uint8_t activeGroup_{};
};
