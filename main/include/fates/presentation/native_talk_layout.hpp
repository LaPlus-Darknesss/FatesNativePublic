#pragma once
#include "fates/io/native_coordinate_resources.hpp"

namespace fates::presentation::native {
enum class TalkLayoutItem:std::uint8_t {Face,String,NextIcon,NamePlate,StringTrim};
// The five original TalkUtil label selectors. Current lookup remains owned by
// TextureCoordinate; this neither caches records nor copies coordinate structs.
class NativeTalkLayout final {
public:
    explicit NativeTalkLayout(std::shared_ptr<io::native::NativeCoordinateResources>);
    static std::optional<std::string_view> Label(TalkLayoutItem,std::uint32_t location);
    bool UsesScheduler(const runtime::native::NativeProcessScheduler&) const noexcept;
    bool UsesCoordinates(const io::native::NativeCoordinateResources&) const noexcept;
    static std::string FaceWindowLabel(std::uint32_t location,bool shout);
    io::native::UniqueArchiveRecordResult FaceWindow(std::uint32_t location,bool shout) const;
    static std::string_view StandWindowLabel(bool shout) noexcept;
    io::native::UniqueArchiveRecordResult StandWindow(bool shout) const;
    std::optional<std::int32_t> MinimumFaceWindowWidth() const;
    static std::int32_t FaceMoveFromOffsetX(std::uint32_t side) noexcept;
    io::native::UniqueArchiveRecordResult Find(TalkLayoutItem,std::uint32_t location) const;
private:
    std::shared_ptr<io::native::NativeCoordinateResources> coordinates_;
};
}
