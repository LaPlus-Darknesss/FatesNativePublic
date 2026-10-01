#include "fates/io/tex_object.hpp"

#include "fates/detail/resource_runtime.hpp"
#include "fates/graphics/i_texture.hpp"

void TexObject::Setup() {
    ResObject::Setup();
    if (!IsResourceSetup()) {
        return;
    }

    textureCount_ = fates::decomp_detail::GetResourceTextureCount(ResourceState());
    if (textureCount_ <= 0) {
        textureCount_ = 0;
        return;
    }

    textures_ = std::make_unique<ITexture[]>(static_cast<std::size_t>(textureCount_));
    for (int index = 0; index < textureCount_; ++index) {
        textures_[index].Setup(
            fates::decomp_detail::GetResourceTextureContent(ResourceState(), index));
    }
}

void TexObject::Cleanup() {
    textures_.reset();
    textureCount_ = 0;
    ResObject::Cleanup();
}

const ITexture* TexObject::FindTexture(const char* identifier) const {
    if (!IsResourceSetup() || identifier == nullptr) {
        return nullptr;
    }
    const int index = fates::decomp_detail::FindResourceTextureIndex(
        ResourceState(), identifier);
    return GetTexture(index);
}

const ITexture* TexObject::GetTexture(int index) const {
    if (!IsResourceSetup() || index < 0 || index >= textureCount_ || !textures_) {
        return nullptr;
    }
    return &textures_[index];
}
