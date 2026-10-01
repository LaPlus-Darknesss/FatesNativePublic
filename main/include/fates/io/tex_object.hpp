#pragma once

#include "fates/io/res_object.hpp"
#include "fates/graphics/i_texture.hpp"

#include <memory>

class TexObject : public ResObject {
public:
    TexObject() = default;
    ~TexObject() override = default;

    void Setup() override;
    void Cleanup() override;

    const ITexture* FindTexture(const char* identifier) const;
    const ITexture* GetTexture(int index) const;
    int GetTextureCount() const { return textureCount_; }

private:
    std::unique_ptr<ITexture[]> textures_{};
    int textureCount_{};
};
