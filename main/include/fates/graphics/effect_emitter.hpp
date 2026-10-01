#pragma once
#include "fates/detail/effect_runtime.hpp"
#include "fates/graphics/effect_package.hpp"
class RenderState;
class EffectEmitter {
public:
    EffectEmitter();
    ~EffectEmitter();
    bool Load(const EffectFile& file,const char* name);
    bool Load(const EffectFile& file,int index);
    void Free();
    void Draw(RenderState& state,float frameRemainder);
    bool IsAlive() const;
    void Fade();
    void SetMatrix(const nn::math::MTX34& world);
    void SetColor(const Color8& color);
    void SetRatio(float ratio);
    void ForceCalc(int frames);
private:
    EffectPackage file_{};
    const EffectObject* resource_{};
    fates::decomp_detail::EffectEmitterHandle emitter_{};
};
