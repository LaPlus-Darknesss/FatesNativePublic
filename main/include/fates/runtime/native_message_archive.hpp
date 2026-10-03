#pragma once
#include "fates/io/native_file_store.hpp"
#include <optional>

namespace fates::runtime::native {
enum class MessageArchiveStatus : std::uint8_t {
    Ready, MissingImage, InvalidHeader, ConstructedImage, RelocationsRequired,
    InvalidTable, InvalidLabel, UnterminatedMessage, LimitExceeded
};
struct MessageArchiveEntry {
    std::string identifier; // Original bytes, including CP932; never UTF-8 normalized.
    std::size_t payload_offset{}, code_units{};
};
// Immutable text-archive data behind GlobalFile's already decoded file image.
// ArchiveConstruct's ordered labels and masked data offsets are preserved,
// including duplicate names and shared value offsets. Registry binding, language
// selection and Mess's mutable expansion buffer are separate owners.
// Relocations need a pointer-bearing archive owner and are explicitly refused;
// this reader does not reinterpret unresolved pointer words as UTF-16 text.
class NativeMessageArchive final {
public:
    static MessageArchiveStatus Read(std::shared_ptr<const io::native::NativeFileImage>,
        std::shared_ptr<const NativeMessageArchive>&);
    NativeMessageArchive(const NativeMessageArchive&)=delete;
    NativeMessageArchive& operator=(const NativeMessageArchive&)=delete;
    std::span<const MessageArchiveEntry> entries() const noexcept {return entries_;}
    std::optional<std::u16string_view> Text(std::size_t index) const noexcept;
    const std::shared_ptr<const io::native::NativeFileImage>& image() const noexcept {return image_;}
private:
    NativeMessageArchive()=default;
    std::shared_ptr<const io::native::NativeFileImage> image_;
    std::vector<char16_t> data_;
    std::vector<MessageArchiveEntry> entries_;
};
}
