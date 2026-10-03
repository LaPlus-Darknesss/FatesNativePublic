#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace fates::graphics::portable {
struct TextureImage {std::uint32_t width{},height{};std::vector<std::uint8_t> rgba;};
// Base mip only, canonical top-row-first RGBA8. Initial native field formats:
// RGBA8, RGB8, RGB565, RGBA4, HILO8, A8, ETC1 and ETC1A4, plus
// the original font-sheet L8, L4, LA8, LA4 and A4 formats. Unsupported formats
// remain explicit. Input may include subsequent mip levels, which are untouched.
bool DecodeTextureBase(std::span<const std::uint8_t>,unsigned format,
    std::uint32_t width,std::uint32_t height,TextureImage&,std::string& error);
}
