#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace nw::h3d::res {
struct TextureContent;
}

namespace fates::decomp_detail {

// CTR/PICA-facing services intentionally remain outside the human game-source
// model. Pass 18 promotes GfxMemory allocation ownership and TexCommand
// decoding, while physical-address translation, DMA, and H3D content access
// remain platform-facing services.
void InitializeTextureMatrix(std::array<float, 12>& matrix);
void BuildTextureMatrix(std::array<float, 12>& matrix, int width, int height);
void BuildTextureMatrixInvOne(std::array<float, 12>& matrix);

std::uintptr_t GetTexturePhysicalAddress(const void* address);
void UploadTextureData(
    std::uint8_t target,
    void* destination,
    const void* source,
    unsigned int size);

void FillNullTexturePattern(void* destination, std::size_t size);
void FillDummyTexturePattern(void* destination, std::size_t size);

int GetTextureContentWidth(const nw::h3d::res::TextureContent& content);
int GetTextureContentHeight(const nw::h3d::res::TextureContent& content);
std::uintptr_t GetTextureContentAddress(const nw::h3d::res::TextureContent& content);
std::uint8_t GetTextureContentFormat(const nw::h3d::res::TextureContent& content);
std::uint8_t GetTextureContentMipCount(const nw::h3d::res::TextureContent& content);

std::uint8_t MapPicaWarpMode(std::uint8_t retailWrapMode);
std::uint8_t MapPicaMagFilter(std::uint8_t retailFilterMode);
std::uint8_t MapPicaMinFilter(std::uint8_t retailFilterMode);

} // namespace fates::decomp_detail
