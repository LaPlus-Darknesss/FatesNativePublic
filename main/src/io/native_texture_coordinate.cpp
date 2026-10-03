#include "fates/io/native_texture_coordinate.hpp"
#include <algorithm>

namespace fates::io::native {
using S=UniqueArchiveStatus;
UniqueArchiveStatus NativeTextureCoordinateScene::Create(std::shared_ptr<NativeUniqueArchiveObjects> archive,
    FileObjectHandle object,std::optional<std::string_view> name,std::shared_ptr<NativeTextureCoordinateScene>& out) {
    if(!archive)return S::InvalidObject;
    const auto found=archive->Find(object,name);
    if(found.status!=S::Ready && found.status!=S::Missing)return found.status;
    auto next=std::shared_ptr<NativeTextureCoordinateScene>(new NativeTextureCoordinateScene);
    next->archive_=std::move(archive);next->object_=std::move(object);next->header_=found.value;
    out=std::move(next);return S::Ready;
}
UniqueArchiveStatus NativeTextureCoordinateScene::DescribeRequests(TextureCoordinatePlan& out) const {
    // The original queries the File table before testing a null scene header.
    // Its result is unused; a known-missing File label is not an error, whereas
    // an unowned/destroyed private index is never substituted with absence.
    auto files=archive_->Find(object_,std::string_view("File"));
    if(files.status!=S::Ready && files.status!=S::Missing)return files.status;
    return DescribeRetainedHeaderRequests(out);
}
UniqueArchiveStatus NativeTextureCoordinateScene::DescribeRetainedHeaderRequests(TextureCoordinatePlan& out) const {
    TextureCoordinatePlan next;
    if(!header_){out=std::move(next);return S::Ready;}
    auto count=archive_->ReadU16(header_,4);if(count.status!=S::Ready)return count.status;
    // Original TextureLoad doesn't read stride/table when there are zero rows.
    next.position_count=count.value;
    if(!count.value){out=std::move(next);return S::Ready;}
    auto stride=archive_->ReadU16(header_,6);if(stride.status!=S::Ready)return stride.status;
    next.stride=stride.value;
    auto table=archive_->Pointer(header_,8);if(table.status!=S::Ready)return table.status;
    if(!table.value)return S::InvalidPointer;
    for(std::uint32_t i=0;i<count.value;++i) {
        // Both multiplicands are u16-domain; the original MLA cannot overflow u32.
        auto row=archive_->At(table.value,std::size_t(i)*stride.value,8);
        if(row.status!=S::Ready)return row.status;
        auto texture=archive_->Pointer(row.value,4);if(texture.status!=S::Ready)return texture.status;
        if(!texture.value)continue;
        auto file=archive_->Pointer(texture.value,4);if(file.status!=S::Ready)return file.status;
        if(!file.value)continue;
        auto path=archive_->Pointer(file.value,0);if(path.status!=S::Ready)return path.status;
        if(!path.value)continue;
        if(std::any_of(next.requests.begin(),next.requests.end(),[&](const auto& p){
            return p.path_identity.SamePointer(path.value);
        }))continue;
        // Retail has 16 slots but no safe overflow branch. Refuse rather than
        // clobber a neighboring host object; this is an explicit safety boundary.
        if(next.requests.size()==16)return S::CapacityExceeded;
        auto text=archive_->ReadString(path.value);if(text.status!=S::Ready)return text.status;
        if(!text.value)return S::InvalidPointer;
        auto name=text.value->substr(0,79);
        constexpr std::string_view suffix=".bch.lz";
        name.append(suffix.substr(0,79-name.size()));
        next.requests.push_back({path.value,*text.value,std::move(name),2});
    }
    out=std::move(next);return S::Ready;
}
std::optional<std::string_view> NativeTextureCoordinateScene::TalkWindowPath(std::uint32_t route) noexcept {
    static constexpr std::array<std::string_view,3> paths{
        "ui/TalkWindowW.bch.lz","ui/TalkWindowB.bch.lz","ui/TalkWindow.bch.lz"};
    return route<paths.size()?std::optional<std::string_view>(paths[route]):std::nullopt;
}
}
