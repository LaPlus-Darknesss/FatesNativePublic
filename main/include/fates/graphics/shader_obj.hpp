#pragma once

#include "fates/detail/shader_runtime.hpp"
#include "fates/io/file_base.hpp"

class ShaderObj {
public:
    ShaderObj();
    ~ShaderObj();

    void SetUniform(int slot, const char* name);
    void SetAttr(int slot, const char* name);
    void SetLineSize(float width, float viewportWidth, float projectionScale) const;
    void SetColor(const Color8& color) const;
    void SetViewProj(const nn::math::MTX44& matrix) const;
    void SetTexMtx(const nn::math::MTX34& matrix) const;
    void SetWorldMtx(const nn::math::MTX34& matrix) const;
    void Transfer() const;

private:
    FileBase resourceFile_{};
    fates::decomp_detail::ShaderRuntimeState runtimeState_{};
};
