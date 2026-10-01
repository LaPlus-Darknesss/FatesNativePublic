#pragma once
#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace fates::graphics::portable {
// Serialized file offsets stay offsets. No original CPU/GPU address is installed
// into a host pointer. Names retain their original CP932 bytes.
struct BchPointer {
    std::uint8_t target_type{};
    std::uint32_t stored{};
    std::optional<std::size_t> offset;
};
struct BchCommandWrite {
    std::uint32_t register_index{},value{};
    std::uint8_t byte_mask{};
    std::size_t source_offset{};
    std::optional<BchPointer> relocation;
};
class BchFile {
public:
    // Atomic parser. FE14 0x22/0x23 header layout is the initial admitted domain.
    // Transport reuses the native FE14/LZ11 decoder; raw BCH and type0 wrappers
    // are supported too. A failed decode preserves output and reports its cause.
    static bool Decode(std::span<const std::uint8_t>,std::shared_ptr<const BchFile>& output,std::string& error);
    std::span<const std::uint8_t> Bytes() const noexcept {return bytes_;}
    const std::array<std::uint32_t,6>& SectionOffsets() const noexcept {return offsets_;}
    std::optional<BchPointer> Resolve(std::size_t field) const;
    bool ReadString(std::size_t field,std::optional<std::string>& output,std::string& error) const;
    bool Category(unsigned index,std::vector<std::size_t>& output,std::string& error) const;
    bool Commands(std::size_t offset,std::size_t word_count,std::vector<BchCommandWrite>& output,std::string& error) const;
private:
    std::uint32_t Word(std::size_t) const;
    bool Fits(std::size_t,std::size_t) const noexcept;
    std::vector<std::uint8_t> bytes_;
    std::array<std::uint32_t,6> offsets_{},lengths_{};
    std::map<std::size_t,std::uint8_t> relocations_;
};
struct BchModelDescriptor {
    std::size_t offset{};
    std::optional<std::string> name;
    std::array<float,12> matrix{};
};
bool ReadBchModels(const BchFile&,std::vector<BchModelDescriptor>&,std::string& error);
struct BchTextureDescriptor {
    std::optional<std::string> name;
    std::uint32_t width{},height{},format{};
    std::size_t data_offset{};
};
// Texture storage descriptors from the resource's unit0 upload command. Format0
// remains a raw code; its RGBA8 versus shadow/gas interpretation needs its consumer.
bool ReadBchTextures(const BchFile&,std::vector<BchTextureDescriptor>&,std::string& error);
}
