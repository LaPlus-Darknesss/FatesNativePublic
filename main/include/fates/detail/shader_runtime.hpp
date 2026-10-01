#pragma once

#include "fates/graphics/basic_types.hpp"

class ShaderObj;

namespace fates::decomp_detail {
struct ShaderRuntimeState {
    void* vendorShader{};
};

void InitializeShaderRuntime(ShaderRuntimeState& state);
void DestroyShaderRuntime(ShaderRuntimeState& state);
void SetShaderUniformBinding(ShaderRuntimeState& state, int slot, const char* name);
void SetShaderAttributeBinding(ShaderRuntimeState& state, int slot, const char* name);
void EmitShaderLineSize(const ShaderRuntimeState& state, float width, float viewportWidth, float projectionScale);
void EmitShaderColor(const ShaderRuntimeState& state, const Color8& color);
void EmitShaderViewProjection(const ShaderRuntimeState& state, const nn::math::MTX44& matrix);
void EmitShaderTextureMatrix(const ShaderRuntimeState& state, const nn::math::MTX34& matrix);
void EmitShaderWorldMatrix(const ShaderRuntimeState& state, const nn::math::MTX34& matrix);

} // namespace fates::decomp_detail
