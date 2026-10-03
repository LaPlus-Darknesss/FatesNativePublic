#pragma once
#include "fates/io/native_file_store.hpp"
#include <array>
#include <optional>

namespace fates::io::native {
// System language/region bytes, not serialized player-save fields. Availability
// is explicit. Region is only required by the executable's default language arm.
struct NativeLanguageState {
    std::optional<std::uint8_t> language;
    std::optional<std::uint8_t> region;
};
enum class LanguagePathStatus : std::uint8_t {
    Ready, UnknownLanguage, UnknownRegion, SourceUnavailable, InvalidPath
};
struct LanguagePathResult {
    LanguagePathStatus status{LanguagePathStatus::SourceUnavailable};
    std::string path;
    bool localized{};
};
// Lang::Folder and Mess's CreateFileName share game-owned bounded path buffers.
// Existence is source existence, never GlobalFile registry membership. The
// platform query must not reenter or mutate this owner. Unknown input/source
// refuses publication; known absence returns the original path without testing
// that original path. Mess's route/common selection is a separate caller.
class NativeLanguagePaths final {
public:
    explicit NativeLanguagePaths(std::function<FileSourceStatus(std::string_view)> exists)
        :exists_(std::move(exists)) {}
    LanguagePathResult Folder(std::string_view,const NativeLanguageState&);
    // Lang::FolderForTexture only attempts localization for region byte2.
    // Other known regions return the input without reading language, querying
    // source existence or touching the shared language buffer.
    LanguagePathResult FolderForTexture(std::string_view,const NativeLanguageState&);
    LanguagePathResult CreateMessageName(std::optional<std::string_view> route,
        std::string_view archive,const NativeLanguageState&);
    // Mess formats common overlays directly into its same80-byte buffer;
    // this path deliberately has no Lang::Folder or existence query.
    LanguagePathResult CreateCommonMessageName(std::string_view archive);
    std::span<const char> language_buffer() const noexcept {return language_buffer_;}
    std::span<const char> message_buffer() const noexcept {return message_buffer_;}
private:
    std::function<FileSourceStatus(std::string_view)> exists_;
    std::array<char,0x80> language_buffer_{};
    std::array<char,0x50> message_buffer_{};
};
}
