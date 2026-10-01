#include "fates/graphics/i_texture.hpp"

#include "fates/detail/texture_runtime.hpp"
#include "fates/graphics/gfx_memory.hpp"

#include <array>
#include <cstddef>
#include <memory>

namespace {
std::unique_ptr<ITexture> gNullTexture;
std::unique_ptr<ITexture> gDummyTexture;

float BytesPerPixel(TexFormat::Type format) {
    switch (static_cast<std::uint8_t>(format)) {
    case 0:
        return 4.0f;
    case 1:
        return 3.0f;
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        return 2.0f;
    case 10:
    case 11:
    case 12:
        return 0.5f;
    default:
        return 1.0f;
    }
}
} // namespace

ITexture::ITexture() {
    fates::decomp_detail::InitializeTextureMatrix(matrix_);
}

ITexture::~ITexture() {
    Cleanup();
}

void ITexture::Initialize() {
    gNullTexture = std::make_unique<ITexture>();
    gDummyTexture = std::make_unique<ITexture>();

    std::array<std::byte, 0x1000> scratch{};
    fates::decomp_detail::FillNullTexturePattern(scratch.data(), scratch.size());
    gNullTexture->Setup(
        TexTarget::Type::Device,
        32,
        32,
        static_cast<TexFormat::Type>(0),
        scratch.data(),
        1);

    fates::decomp_detail::FillDummyTexturePattern(scratch.data(), scratch.size());
    gDummyTexture->Setup(
        TexTarget::Type::Device,
        32,
        32,
        static_cast<TexFormat::Type>(0),
        scratch.data(),
        1);
}

void ITexture::Setup(
    TexTarget::Type target,
    int width,
    int height,
    TexFormat::Type format,
    const void* source,
    int mipCount) {
    Cleanup();

    mipCount_ = static_cast<std::uint8_t>(mipCount);
    width_ = static_cast<std::uint16_t>(width);
    height_ = static_cast<std::uint16_t>(height);
    target_ = target;
    format_ = format;
    SetMatrix(width, height);
    if (mipCount_ > 1) {
        filterMode_ = 1;
    }

    const unsigned int imageSize = GetImageSize(width, height, format, mipCount);
    if (target == TexTarget::Type::External) {
        backingAddress_ = const_cast<void*>(source);
    } else {
        switch (target) {
        case TexTarget::Type::Device:
            backingAddress_ = GfxMemory::TryAllocDevice(imageSize, 0x80);
            break;
        case TexTarget::Type::VramAB:
            backingAddress_ = GfxMemory::TryAllocVramAB(imageSize, 0x80);
            break;
        case TexTarget::Type::VramBA:
            backingAddress_ = GfxMemory::TryAllocVramBA(imageSize, 0x80);
            break;
        case TexTarget::Type::External:
            break;
        }
    }

    physicalAddress_ = fates::decomp_detail::GetTexturePhysicalAddress(backingAddress_);
    if (source != nullptr && target != TexTarget::Type::External && backingAddress_ != nullptr) {
        fates::decomp_detail::UploadTextureData(
            static_cast<std::uint8_t>(target),
            backingAddress_,
            source,
            imageSize);
    }
}

void ITexture::Setup(const nw::h3d::res::TextureContent* textureContent) {
    if (textureContent == nullptr) {
        return;
    }

    Cleanup();
    width_ = static_cast<std::uint16_t>(
        fates::decomp_detail::GetTextureContentWidth(*textureContent));
    height_ = static_cast<std::uint16_t>(
        fates::decomp_detail::GetTextureContentHeight(*textureContent));
    target_ = TexTarget::Type::External;
    format_ = static_cast<TexFormat::Type>(
        fates::decomp_detail::GetTextureContentFormat(*textureContent));
    mipCount_ = fates::decomp_detail::GetTextureContentMipCount(*textureContent);
    SetMatrix(width_, height_);
    if (mipCount_ > 1) {
        filterMode_ = 1;
    }
    backingAddress_ = nullptr;
    physicalAddress_ = fates::decomp_detail::GetTextureContentAddress(*textureContent);
}

void ITexture::Cleanup() {
    if (physicalAddress_ != 0 && target_ != TexTarget::Type::External) {
        GfxMemory::FreeAddress(backingAddress_);
    }
    backingAddress_ = nullptr;
    physicalAddress_ = 0;
}

ITexture* ITexture::GetDummyTex() {
    return gDummyTexture.get();
}

ITexture* ITexture::GetNullTex() {
    return gNullTexture.get();
}

unsigned int ITexture::GetImageSize(
    int width,
    int height,
    TexFormat::Type format,
    int mipCount) {
    const float pixels = static_cast<float>(width * height);
    unsigned int total = static_cast<unsigned int>(BytesPerPixel(format) * pixels);

    // Preserve the retail mip accumulation order. The compiler's sequence is
    // equivalent to repeatedly adding quarter-sized levels but retains integer
    // truncation at each step.
    if (mipCount > 1) {
        unsigned int level = total;
        if ((mipCount & 1) == 0) {
            level = total >> 2;
            total += level;
        }
        unsigned int delayed = 0;
        for (int pairs = (mipCount - 1) >> 1; pairs != 0; --pairs) {
            total += level >> 2;
            level >>= 4;
            delayed += level;
        }
        total += delayed;
    }
    return total;
}

unsigned int ITexture::GetImageSize(const ITexture* texture) {
    if (texture == nullptr || texture->physicalAddress_ == 0) {
        return 0;
    }
    return GetImageSize(
        texture->width_,
        texture->height_,
        texture->format_,
        texture->mipCount_);
}

void ITexture::SetMatrixInvOne() {
    fates::decomp_detail::BuildTextureMatrixInvOne(matrix_);
}

void ITexture::SetMatrix(int width, int height) {
    fates::decomp_detail::BuildTextureMatrix(matrix_, width, height);
}

bool ITexture::HasColorFormat() const {
    const std::uint8_t value = static_cast<std::uint8_t>(format_);
    return value != 8 && value != 11;
}

std::uint8_t ITexture::GetPicaWarpMode() const {
    return fates::decomp_detail::MapPicaWarpMode(wrapMode_);
}

std::uint8_t ITexture::GetPicaMagFilter() const {
    return fates::decomp_detail::MapPicaMagFilter(filterMode_);
}

std::uint8_t ITexture::GetPicaMinFilter() const {
    return fates::decomp_detail::MapPicaMinFilter(filterMode_);
}
