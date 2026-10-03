#include "fates/presentation/native_talk_layout.hpp"
namespace fates::presentation::native {
NativeTalkLayout::NativeTalkLayout(std::shared_ptr<io::native::NativeCoordinateResources> value):coordinates_(std::move(value)){}
bool NativeTalkLayout::UsesCoordinates(const io::native::NativeCoordinateResources& coordinates) const noexcept{return coordinates_.get()==&coordinates;}
std::optional<std::string_view> NativeTalkLayout::Label(TalkLayoutItem item,std::uint32_t location) {
    switch(item) {
    case TalkLayoutItem::Face:
        return location==0?"MiniTalkW_Status1_MiniTalkW1Face":"MiniTalkW_Status1_MiniTalkW2Face";
    case TalkLayoutItem::String:
        if(location==0)return "MiniTalkW_Status1_Mini1Message";
        if(location==6)return "MiniTalkW_Status1_Mini2Message";
        if(location==103)return "ShopBuyD_Status1_TalkWMessage";
        return "TalkW_Status1_TalkWMessage";
    case TalkLayoutItem::NextIcon:
        if(location==0)return "MiniTalkW_TalkW2_Mini1Cursor";
        if(location==6)return "MiniTalkW_TalkW2_Mini2Cursor";
        return "TalkW_TalkW2_Cursor";
    case TalkLayoutItem::NamePlate:
        if(location==0)return "MiniTalkW_TalkW2_Mini1NameFrame";
        if(location==6)return "MiniTalkW_TalkW2_Mini2NameFrame";
        if(location==7)return "TalkW_TalkW2_TalkWNameFrame2";
        if(location==103)return "ShopBuyD_TalkW2_TalkWNameFrame";
        return "TalkW_TalkW2_TalkWNameFrame";
    case TalkLayoutItem::StringTrim:
        if(location==0)return "MiniTalkW_Status1_Mini1MessageTrim";
        if(location==6)return "MiniTalkW_Status1_Mini2MessageTrim";
        if(location==103)return "ShopBuyD_Status1_VariableTalkWTrim";
        return "TalkW_Status1_TalkWMessageTrim";
    }
    return {};
}
std::int32_t NativeTalkLayout::FaceMoveFromOffsetX(std::uint32_t side) noexcept {return side==0?-16:(side==2?16:0);}
io::native::UniqueArchiveRecordResult NativeTalkLayout::Find(TalkLayoutItem item,std::uint32_t location) const {
    if(!coordinates_)return {io::native::UniqueArchiveStatus::InvalidObject,{}};
    const auto label=Label(item,location);
    if(!label)return {io::native::UniqueArchiveStatus::InvalidIdentifier,{}};
    return coordinates_->Find(*label);
}
}

namespace fates::presentation::native {
std::string NativeTalkLayout::FaceWindowLabel(std::uint32_t location,bool shout) {
    const auto index=location==0?1:(location==6?2:0);
    return "MiniTalkW_TalkW_Mini"+std::to_string(index)+(shout?"Shout":"");
}
io::native::UniqueArchiveRecordResult NativeTalkLayout::FaceWindow(std::uint32_t location,bool shout) const {
    if(!coordinates_)return {io::native::UniqueArchiveStatus::InvalidObject,{}};
    return coordinates_->Find(FaceWindowLabel(location,shout));
}
std::string_view NativeTalkLayout::StandWindowLabel(bool shout) noexcept {
    return shout?"TalkW_TalkW_TalkWShout":"TalkW_TalkW_TalkW";
}
io::native::UniqueArchiveRecordResult NativeTalkLayout::StandWindow(bool shout) const {
    if(!coordinates_)return {io::native::UniqueArchiveStatus::InvalidObject,{}};
    return coordinates_->Find(StandWindowLabel(shout));
}
std::optional<std::int32_t> NativeTalkLayout::MinimumFaceWindowWidth() const {
    using S=io::native::UniqueArchiveStatus;
    if(!coordinates_)return {};
    const auto position=FaceWindow(0,false);
    if(position.status==S::Missing || (position.status==S::Ready && !position.value))return 0;
    if(position.status!=S::Ready)return {};
    const auto signed16=[](std::uint32_t word){return word&0x8000u?static_cast<std::int32_t>(word)-65536:static_cast<std::int32_t>(word);};
    auto width=coordinates_->ReadU16(position.value,12);if(width.status!=S::Ready)return {};
    auto value=signed16(width.value);if(value>0)return value;
    const auto texture=coordinates_->Pointer(position.value,4);if(texture.status!=S::Ready)return {};
    if(!texture.value)return value;
    width=coordinates_->ReadU16(texture.value,12);if(width.status!=S::Ready)return {};
    value=signed16(width.value);if(value<0)value=-value;
    return signed16(static_cast<std::uint32_t>(value)&0xffffu); // original SXTH after abs(-32768).
}
}

namespace fates::presentation::native {bool NativeTalkLayout::UsesScheduler(const runtime::native::NativeProcessScheduler& scheduler) const noexcept{return coordinates_ && coordinates_->UsesScheduler(scheduler);}}
