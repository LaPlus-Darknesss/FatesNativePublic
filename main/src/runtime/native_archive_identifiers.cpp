#include "fates/runtime/native_archive_identifiers.hpp"

namespace fates::runtime::native {
ArchiveIdentifierStatus NativeArchiveIdentifiers::Construct(
    std::shared_ptr<const io::native::NativeFileImage> image,
    std::shared_ptr<const ArchiveRegistration>& out,std::shared_ptr<const ArchiveImageLifetime> lifetime) {
    using S=ArchiveIdentifierStatus;
    if(retired_)return S::Retired;
    if(!image)return S::InvalidImage;
    if(lifetime && !lifetime->live())return S::StaleValue;
    if(const auto found=active_.find(image.get());found!=active_.end()) {
        if(found->second->lifetime_!=lifetime)return S::ForeignRegistration;
        out=found->second;return S::Ready;
    }
    std::shared_ptr<ArchiveRegistration> next;
    if(const auto found=known_.find(image.get());found!=known_.end())next=found->second.lock();
    if(!next) {
        next=std::make_shared<ArchiveRegistration>();
        if(ReadArchiveLabelTable(image->bytes(),next->labels_)!=ArchiveLabelTableStatus::Ready)return S::InvalidArchive;
        next->domain_=domain_;next->image_=std::move(image);next->lifetime_=std::move(lifetime);
    }
    else if(next->lifetime_!=lifetime)return S::ForeignRegistration;
    if(next->labels_.entries.size()>32768-index_.Size())return S::LimitExceeded;
    auto index=index_;auto active=active_;auto known=known_;
    std::erase_if(known,[](const auto& entry){return entry.second.expired();});
    for(const auto& entry:next->labels_.entries)
        index.Append(entry.identifier.c_str(),{next,entry.payload_offset});
    active.emplace(next->image_.get(),next);
    known.insert_or_assign(next->image_.get(),next);
    next->registered_=true;index_=std::move(index);active_=std::move(active);known_=std::move(known);out=std::move(next);
    return S::Ready;
}
ArchiveIdentifierStatus NativeArchiveIdentifiers::Destruct(const std::shared_ptr<const ArchiveRegistration>& token) {
    using S=ArchiveIdentifierStatus;
    if(retired_)return S::Retired;
    if(!token || token->domain_!=domain_)return S::ForeignRegistration;
    if(token->lifetime_ && !token->lifetime_->live())return S::StaleValue;
    if(!token->registered_)return S::Ready;
    const auto found=active_.find(token->image_.get());
    if(found==active_.end() || found->second.get()!=token.get())return S::ForeignRegistration;
    auto index=index_;
    for(auto entry=token->labels_.entries.rbegin();entry!=token->labels_.entries.rend();++entry)
        index.EraseFirst(entry->identifier.c_str());
    index_=std::move(index);found->second->registered_=false;active_.erase(found);return S::Ready;
}
ArchiveIdentifierStatus NativeArchiveIdentifiers::DestructImage(
    const std::shared_ptr<const io::native::NativeFileImage>& image,
    const std::shared_ptr<const ArchiveImageLifetime>& lifetime) {
    using S=ArchiveIdentifierStatus;
    if(retired_)return S::Retired;
    if(!image)return S::InvalidImage;
    if(lifetime && !lifetime->live())return S::StaleValue;
    const auto found=active_.find(image.get());
    if(found==active_.end())return S::Ready;
    if(found->second->lifetime_!=lifetime)return S::ForeignRegistration;
    return Destruct(found->second);
}
ArchiveIdentifierResult NativeArchiveIdentifiers::Find(std::string_view name) const {
    using S=ArchiveIdentifierStatus;
    if(retired_)return {S::Retired,{}};
    if(name.size()>65535 || name.find('\0')!=std::string_view::npos)return {S::InvalidIdentifier,{}};
    const std::string key(name);const auto* entry=index_.Find(key.c_str());
    if(!entry)return {S::Missing,{}};
    // Reverse, first-hash deletion can leave a value from the destructed image
    // when archives have colliding labels. Preserve membership for diagnostics,
    // but never treat that retired registration as a live typed object or absence.
    const auto& token=entry->value.registration;
    return {token->registered_ && (!token->lifetime_ || token->lifetime_->live())?S::Ready:S::StaleValue,entry->value};
}
ArchiveIdentifierWordResult NativeArchiveIdentifiers::ReadWord(std::string_view name) const {
    using S=ArchiveIdentifierStatus;
    const auto found=Find(name);
    if(found.status!=S::Ready)return {found.status};
    const auto& token=*found.value.registration;
    const auto offset=found.value.payload_offset;
    const auto size=token.labels_.data_bytes;
    if(offset>size || size-offset<4)return {S::InvalidArchive};
    const auto bytes=token.image_->bytes();
    const auto word=[&](std::size_t at) {
        return std::uint32_t(bytes[at])|(std::uint32_t(bytes[at+1])<<8)|
            (std::uint32_t(bytes[at+2])<<16)|(std::uint32_t(bytes[at+3])<<24);
    };
    for(std::size_t i=0;i<token.labels_.relocation_count;++i)
        if((word(32+size+i*4)&~3u)==offset)return {S::InvalidArchive};
    return {S::Ready,word(32+offset)};
}
ArchiveIdentifierStringResult NativeArchiveIdentifiers::ReadStringField(
    const ArchiveIdentifierValue& value,std::size_t field) const {
    using S=ArchiveIdentifierStatus;
    if(retired_)return {S::Retired};
    const auto& token=value.registration;
    if(!token||token->domain_!=domain_)return {S::ForeignRegistration};
    if(!token->registered_||(token->lifetime_&&!token->lifetime_->live()))return {S::StaleValue};
    const auto size=token->labels_.data_bytes,offset=value.payload_offset;
    if(offset>size||field>size-offset||size-offset-field<4)return {S::InvalidArchive};
    const auto at=offset+field;const auto bytes=token->image_->bytes();
    const auto word=[&](std::size_t p) {
        return std::uint32_t(bytes[p])|(std::uint32_t(bytes[p+1])<<8)|
            (std::uint32_t(bytes[p+2])<<16)|(std::uint32_t(bytes[p+3])<<24);
    };
    std::size_t relocations{};
    for(std::size_t i=0;i<token->labels_.relocation_count;++i)
        if((word(32+size+i*4)&~3u)==at)++relocations;
    const auto relative=word(32+at);
    if(!relocations&&!relative)return {S::Ready,std::nullopt};
    // An unrelocated nonzero word is not an image-relative pointer. Duplicate
    // relocation would add the original base twice and has no portable target.
    if(relocations!=1||relative>=bytes.size()-32)return {S::InvalidArchive};
    const std::size_t begin=32+relative;auto end=begin;
    while(end<bytes.size()&&bytes[end])++end;
    if(end==bytes.size())return {S::InvalidArchive};
    for(std::size_t i=0;i<token->labels_.relocation_count;++i) {
        const auto relocated=32+std::size_t(word(32+size+i*4)&~3u);
        if(begin<relocated+4&&relocated<=end)return {S::InvalidArchive};
    }
    return {S::Ready,std::string(reinterpret_cast<const char*>(bytes.data()+begin),end-begin)};
}
ArchiveIdentifierWordResult NativeArchiveIdentifiers::ReadHalfword(
    const ArchiveIdentifierValue& value,std::size_t displacement) const {
    using S=ArchiveIdentifierStatus;
    if(retired_)return {S::Retired};
    const auto& token=value.registration;
    if(!token||token->domain_!=domain_)return {S::ForeignRegistration};
    if(!token->registered_||(token->lifetime_&&!token->lifetime_->live()))return {S::StaleValue};
    const auto size=token->labels_.data_bytes,offset=value.payload_offset;
    if(offset>size||displacement>size-offset||size-offset-displacement<2)return {S::InvalidArchive};
    const auto at=offset+displacement;const auto bytes=token->image_->bytes();
    for(std::size_t i=0;i<token->labels_.relocation_count;++i) {
        const auto p=32+size+i*4;
        const auto relocated=(std::uint32_t(bytes[p])|(std::uint32_t(bytes[p+1])<<8)|
            (std::uint32_t(bytes[p+2])<<16)|(std::uint32_t(bytes[p+3])<<24))&~3u;
        if(at<std::size_t(relocated)+4&&relocated<at+2)return {S::InvalidArchive};
    }
    return {S::Ready,std::uint32_t(bytes[32+at])|(std::uint32_t(bytes[33+at])<<8)};
}
void NativeArchiveIdentifiers::Retire() noexcept {
    if(retired_)return;
    retired_=true;
    for(auto& entry:active_)entry.second->registered_=false;
    active_.clear();known_.clear();index_.Clear();
}
}
