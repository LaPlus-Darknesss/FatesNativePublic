#pragma once

#include <cstdint>

class RenderState;
class SceneSystem;

namespace GfxConst { enum Type : int; }

namespace fates::decomp_detail {
struct RenderStateRuntime {
    SceneSystem* scene{};
    int currentType{};
    bool persistentDraw{};
    void* commandAlterCallback{};
};

void InitializeRenderState(RenderStateRuntime& state, SceneSystem* scene);
void FinalizeRenderState(RenderStateRuntime& state);
void ChangeRenderState(RenderStateRuntime& state, GfxConst::Type type);
void EndRenderCommand(RenderStateRuntime& state);
void BeginRenderDraw(RenderStateRuntime& state, bool persistent);
void EndRenderDraw(RenderStateRuntime& state, bool persistent);
void ReplayRenderStateReplacement(RenderStateRuntime& state);

} // namespace fates::decomp_detail
