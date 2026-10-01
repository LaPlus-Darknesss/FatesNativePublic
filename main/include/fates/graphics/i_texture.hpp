#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace nw::h3d::res {
struct TextureContent;
}

namespace TexTarget {
enum class Type : std::uint8_t {
    External = 0,
    Device = 1,
    VramAB = 2,
    VramBA = 3,
};
}

namespace TexFormat {
// Retail stores the texture format as one byte. Named format enumerators stay
// deliberately deferred until the format table itself is promoted.
enum class Type : std::uint8_t {};
}

class ITexture {
public:
    ITexture();
    ~ITexture();

    ITexture(const ITexture&) = delete;
    ITexture& operator=(const ITexture&) = delete;

    static void Initialize();

    void Setup(
        TexTarget::Type target,
        int width,
        int height,
        TexFormat::Type format,
        const void* source,
        int mipCount);
    void Setup(const nw::h3d::res::TextureContent* textureContent);
    void Cleanup();

    static ITexture* GetDummyTex();
    static ITexture* GetNullTex();
    static unsigned int GetImageSize(
        int width,
        int height,
        TexFormat::Type format,
        int mipCount);
    static unsigned int GetImageSize(const ITexture* texture);

    void SetMatrixInvOne();
    void SetMatrix(int width, int height);

    bool HasColorFormat() const;
    std::uint8_t GetPicaWarpMode() const;
    std::uint8_t GetPicaMagFilter() const;
    std::uint8_t GetPicaMinFilter() const;

    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    TexTarget::Type GetTarget() const { return target_; }
    TexFormat::Type GetFormat() const { return format_; }
    int GetMipCount() const { return mipCount_; }
    std::uintptr_t GetPhysicalAddress() const { return physicalAddress_; }
    const std::array<float, 12>& GetMatrix() const { return matrix_; }

private:
    void* backingAddress_{};
    std::uintptr_t physicalAddress_{};
    std::uint16_t width_{};
    std::uint16_t height_{};
    TexTarget::Type target_{TexTarget::Type::External};
    TexFormat::Type format_{};
    std::uint8_t filterMode_{};
    std::uint8_t wrapMode_{};
    std::uint8_t unknownMode_{};
    std::uint8_t mipCount_{};
    std::array<float, 12> matrix_{};
};
