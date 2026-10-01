#include "fates/graphics/shader_obj.hpp"

#include "fates/detail/resource_file_runtime.hpp"

ShaderObj::ShaderObj() {
    fates::decomp_detail::InitializeShaderRuntime(runtimeState_);
}

ShaderObj::~ShaderObj() {
    resourceFile_.Free();
    fates::decomp_detail::DestroyShaderRuntime(runtimeState_);
}

void ShaderObj::SetUniform(int slot, const char* name) {
    fates::decomp_detail::SetShaderUniformBinding(runtimeState_, slot, name);
}

void ShaderObj::SetAttr(int slot, const char* name) {
    fates::decomp_detail::SetShaderAttributeBinding(runtimeState_, slot, name);
}

void ShaderObj::SetLineSize(float width, float viewportWidth, float projectionScale) const {
    if (projectionScale <= 0.0f) return;
    fates::decomp_detail::EmitShaderLineSize(runtimeState_, width, viewportWidth, projectionScale);
}

void ShaderObj::SetColor(const Color8& color) const {
    fates::decomp_detail::EmitShaderColor(runtimeState_, color);
}

void ShaderObj::SetViewProj(const nn::math::MTX44& matrix) const {
    fates::decomp_detail::EmitShaderViewProjection(runtimeState_, matrix);
}

void ShaderObj::SetTexMtx(const nn::math::MTX34& matrix) const {
    fates::decomp_detail::EmitShaderTextureMatrix(runtimeState_, matrix);
}

void ShaderObj::SetWorldMtx(const nn::math::MTX34& matrix) const {
    fates::decomp_detail::EmitShaderWorldMatrix(runtimeState_, matrix);
}

void ShaderObj::Transfer() const {
    fates::decomp_detail::TransferShaderCommands(*this);
}
