#include "fates/runtime/native_message_archive.hpp"
#include "fates/runtime/native_archive.hpp"
#include <algorithm>

namespace fates::runtime::native {
namespace {
bool Fits(std::size_t at,std::size_t count,std::size_t size) noexcept {
    return at<=size && count<=size-at;
}
std::uint32_t Word(std::span<const std::uint8_t> b,std::size_t at) noexcept {
    return std::uint32_t(b[at])|(std::uint32_t(b[at+1])<<8)|
        (std::uint32_t(b[at+2])<<16)|(std::uint32_t(b[at+3])<<24);
}
}
MessageArchiveStatus NativeMessageArchive::Read(
    std::shared_ptr<const io::native::NativeFileImage> image,
    std::shared_ptr<const NativeMessageArchive>& out) {
    using S=MessageArchiveStatus;
    if(!image)return S::MissingImage;
    const auto b=image->bytes();
    if(b.size()<0x20)return S::InvalidHeader;
    if(b.size()>16u*1024u*1024u)return S::LimitExceeded;
    if(Word(b,0)!=b.size())return S::InvalidHeader;
    if(b[0x1f] || std::equal(b.begin()+0x18,b.begin()+0x1f,"HSDArc"))return S::ConstructedImage;
    if(Word(b,8))return S::RelocationsRequired;
    ArchiveLabelTable labels;
    switch(ReadArchiveLabelTable(b,labels)) {
    case ArchiveLabelTableStatus::Ready:break;
    case ArchiveLabelTableStatus::InvalidHeader:return S::InvalidHeader;
    case ArchiveLabelTableStatus::ConstructedImage:return S::ConstructedImage;
    case ArchiveLabelTableStatus::InvalidTable:return S::InvalidTable;
    case ArchiveLabelTableStatus::InvalidLabel:return S::InvalidLabel;
    case ArchiveLabelTableStatus::LimitExceeded:return S::LimitExceeded;
    }
    const auto data_bytes=labels.data_bytes;
    auto next=std::shared_ptr<NativeMessageArchive>(new NativeMessageArchive);
    next->data_.reserve(data_bytes/2);
    std::vector<std::size_t> terminators;
    for(std::size_t at=0;at<data_bytes;at+=2) {
        const auto value=char16_t(unsigned(b[0x20+at])|(unsigned(b[0x21+at])<<8));
        if(!value)terminators.push_back(at/2);
        next->data_.push_back(value);
    }
    next->entries_.reserve(labels.entries.size());
    for(auto& label:labels.entries) {
        const auto value=label.payload_offset;
        if(!Fits(value,2,data_bytes))return S::InvalidLabel;
        const auto end=std::lower_bound(terminators.begin(),terminators.end(),value/2);
        if(end==terminators.end())return S::UnterminatedMessage;
        next->entries_.push_back({
            std::move(label.identifier),
            value,*end-value/2});
    }
    next->image_=std::move(image);
    out=std::move(next);
    return S::Ready;
}
std::optional<std::u16string_view> NativeMessageArchive::Text(std::size_t index) const noexcept {
    if(index>=entries_.size())return std::nullopt;
    const auto& entry=entries_[index];
    return std::u16string_view(data_.data()+entry.payload_offset/2,entry.code_units);
}
}
