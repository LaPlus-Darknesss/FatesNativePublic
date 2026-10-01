#pragma once
#include <cstdint>
#include "fates/graphics/scene_node.hpp"
#include "fates/graphics/model.hpp"
#include "fates/graphics/effect_emitter.hpp"
#include "fates/graphics/anim_ctrl.hpp"
#include "fates/graphics/effect_package.hpp"
#include "fates/io/res_file.hpp"

class RenderState;
class EffectNode : public SceneNode {
public:
    EffectNode();
    ~EffectNode() override;
    bool Setup(const char* modelName,const char* emitterName);
    bool Setup();
    void ReadAsync(const char* path);
    void OnUpdate(RenderState& state);
    void OnDrawOpa(RenderState& state);
    void OnDrawXlu(RenderState& state);
    void FadeEmitter();
    void SetAnimFrame(float frame);
    void SetStepFrame(float frame);
    void SetModelAlpha(unsigned char alpha);
    void SetModelColor(const Color8& color);
    void SetEmitterAlpha(unsigned char alpha);
    void SetEmitterColor(const Color8& color);
    void SetEmitterRatio(float ratio);
    void UpdateTransform();
    void SetIdling(int frames);
    bool IsFinished() const;
    bool IsEmitterAlive() const;
    bool IsAsyncLoading() const;
    float GetAnimFrame() const;
    float GetRemainFrame() const;
    Model& GetModel(){ return model_; }
private:
    enum Dirty : std::uint16_t { TransformDirty=0x1, FrameDirty=0x2, StepDirty=0x4, IdlingDirty=0x8, ModelColorDirty=0x10, EmitterColorDirty=0x20, RatioDirty=0x40 };
    Model model_{};
    EffectEmitter emitter_{};
    ResFile modelResources_{};
    EffectPackage emitterResources_{};
    EffectPackage sourcePackage_{};
    AnimCtrl anim_{};
    Color8 modelColor_{255,255,255,255};
    Color8 emitterColor_{255,255,255,255};
    float animFrame_{};
    float stepFrame_{1.0f};
    float emitterRatio_{1.0f};
    float frameRemainder_{};
    float totalFrame_{};
    std::uint16_t idlingFrames_{};
    std::uint16_t dirty_{};
};
