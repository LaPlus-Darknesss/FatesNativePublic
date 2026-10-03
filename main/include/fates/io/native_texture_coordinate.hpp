#pragma once
#include "fates/io/native_unique_archive_objects.hpp"
#include <array>

namespace fates::io::native {
struct TextureCoordinateRequest {
    UniqueArchiveRecord path_identity;
    std::string original_path;
    std::string archive_path; // Original 80-byte bounded path + .bch.lz, before Lang.
    std::uint32_t flags{2};
};
struct TextureCoordinatePlan {
    std::size_t position_count{},stride{};
    std::vector<TextureCoordinateRequest> requests;
};
// Retains the actual private-archive scene header and image-relative identities.
// This is the data/selection portion of Scene::TextureLoad, NOT a TexFile loader.
// DescribeRequests is an atomic, side-effect-free preflight. It neither advances
// a game scene's loaded-count nor changes the shared language buffer. A later
// concrete TexFile owner must perform per-slot localization/read/destruction in
// original order; no Ready result here can complete a Talk resource wait.
class NativeTextureCoordinateScene final {
public:
    static UniqueArchiveStatus Create(std::shared_ptr<NativeUniqueArchiveObjects>,FileObjectHandle,
        std::optional<std::string_view> name,std::shared_ptr<NativeTextureCoordinateScene>&);
    UniqueArchiveStatus DescribeRequests(TextureCoordinatePlan&) const;
    // Header-only immutable preflight for a live global owner which has already
    // performed TextureLoad's CURRENT StructFile "File" lookup. A known-null
    // retained header must not consult a retired construction-time file again.
    UniqueArchiveStatus DescribeRetainedHeaderRequests(TextureCoordinatePlan&) const;
    const UniqueArchiveRecord& header() const noexcept {return header_;}
    // TalkConst::GetWindowFilePath has precisely these three original table rows.
    // Out-of-table reads are an unowned state, not a fallback to the neutral row.
    static std::optional<std::string_view> TalkWindowPath(std::uint32_t route) noexcept;
private:
    std::shared_ptr<NativeUniqueArchiveObjects> archive_;
    FileObjectHandle object_;
    UniqueArchiveRecord header_;
};
}
