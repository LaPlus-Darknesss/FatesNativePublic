#include "fates/runtime/native_message_lookup.hpp"
#include <algorithm>
#include <limits>

namespace fates::runtime::native {
namespace {
using S=MessageLookupStatus;
constexpr std::string_view kDefaultName="MPID_\x83\x66\x83\x74\x83\x48\x83\x8b\x83\x67\x96\xbc";
std::size_t ArgumentIndex(std::int32_t index) {
    return static_cast<std::uint32_t>(index)<4?static_cast<std::size_t>(index):0;
}
bool ValidText(std::optional<std::u16string_view> text) {
    return text && text->size()<=1024u*1024u && text->find(char16_t{})==std::u16string_view::npos;
}
void CopyText(std::span<char16_t> destination,std::u16string_view source) {
    // sut::wcsncpy copies through capacity and then replaces the final word
    // with NUL. It does not pad the remaining destination.
    const auto count=std::min(source.size(),destination.size());
    std::copy_n(source.begin(),count,destination.begin());
    destination[std::min(count,destination.size()-1)]=0;
}
MessageLookupResult Resolve(const NativeArchiveIdentifiers& identifiers,std::string_view name) {
    using S=MessageLookupStatus;
    const auto found=identifiers.Find(name);
    switch(found.status) {
    case ArchiveIdentifierStatus::Ready:break;
    case ArchiveIdentifierStatus::Missing:return {S::Missing};
    case ArchiveIdentifierStatus::InvalidIdentifier:return {S::InvalidIdentifier};
    case ArchiveIdentifierStatus::Retired:return {S::RegistryRetired};
    case ArchiveIdentifierStatus::StaleValue:return {S::StaleValue};
    default:return {S::InvalidArchive};
    }
    std::shared_ptr<const NativeMessageArchive> archive;
    if(NativeMessageArchive::Read(found.value.registration->image(),archive)!=MessageArchiveStatus::Ready)
        return {S::InvalidArchive};
    // The shared hash returns a value identity, not a spelling. In particular,
    // looking up the requested spelling again would hide real hash collisions.
    for(std::size_t i=0;i<archive->entries().size();++i) {
        if(archive->entries()[i].payload_offset!=found.value.payload_offset)continue;
        return {S::Ready,MessageLookupOrigin::Archive,std::u16string(*archive->Text(i)),
            found.value.registration,found.value.payload_offset};
    }
    return {S::InvalidArchive};
}
}
NativeMessageLookup::NativeMessageLookup(const NativeArchiveIdentifiers& identifiers,std::size_t capacity)
    :identifiers_(identifiers) {
    // A host allocation admission bound, not a replacement for an unknown game
    // buffer. Zero capacity would make the original sut helper write before it.
    if(capacity && capacity<=1024u*1024u)buffer_.resize(capacity);
}
MessageLookupResult NativeMessageLookup::GetNoReplace(std::optional<std::string_view> identifier) {
    using S=MessageLookupStatus;
    if(buffer_.empty())return {S::InvalidBuffer};
    if(!identifier)return {S::Missing};
    auto message=Resolve(identifiers_,*identifier);
    if(message.status!=S::Ready)return message;
    // Original sut::snprintf destination has 0x50 bytes including the terminator.
    auto common_name=std::string(*identifier)+"_COM";
    common_name.resize(std::min(common_name.size(),std::size_t{0x4f}));
    const auto common=Resolve(identifiers_,common_name);
    if(common.status!=S::Ready && common.status!=S::Missing)return {common.status};
    auto next_buffer=buffer_;
    if(common.status==S::Ready) {
        // Retail copies commonLength+1, before inspecting "$a", without
        // clamping to the active buffer. Refuse overflow atomically, rather
        // than silently changing this branch to a truncated common prefix.
        if(common.text.size()>=next_buffer.size())return {S::BufferTooSmall};
        std::copy(common.text.begin(),common.text.end(),next_buffer.begin());
        next_buffer[common.text.size()]=0;
    }
    if(message.text.starts_with(u"$a")) {
        message.text.erase(0,2);
        message.payload_offset+=4;
    } else if(common.status==S::Ready) {
        // sut::wcsncat's bound is TOTAL capacity. It may fill the final word,
        // then replaces that word with NUL. Preserve untouched buffer tail.
        const auto copied=std::min(message.text.size(),next_buffer.size()-common.text.size());
        std::copy_n(message.text.begin(),copied,next_buffer.begin()+common.text.size());
        const auto length=std::min(common.text.size()+copied,next_buffer.size()-1);
        next_buffer[length]=0;
        message={S::Ready,MessageLookupOrigin::Buffer,std::u16string(next_buffer.data(),length),
            {},0,buffer_identity_};
    }
    buffer_.swap(next_buffer);
    return message;
}
MessageSourceWord NativeMessageLookup::ReadSourceWord(const MessageLookupResult& source,std::size_t index) const {
    if(source.status!=S::Ready&&source.status!=S::Missing)return {source.status};
    if(source.origin==MessageLookupOrigin::Buffer) {
        if(source.buffer_identity!=buffer_identity_)return {S::InvalidBuffer};
        return index<buffer_.size()?MessageSourceWord{S::Ready,buffer_[index]}:MessageSourceWord{S::InvalidText};
    }
    if(source.origin==MessageLookupOrigin::Archive) {
        if(index>std::numeric_limits<std::size_t>::max()/2)return {S::InvalidText};
        const auto word=identifiers_.ReadHalfword({source.registration,source.payload_offset},index*2);
        switch(word.status) {
        case ArchiveIdentifierStatus::Ready:return {S::Ready,char16_t(word.value)};
        case ArchiveIdentifierStatus::Retired:return {S::RegistryRetired};
        case ArchiveIdentifierStatus::StaleValue:return {S::StaleValue};
        default:return {S::InvalidArchive};
        }
    }
    if(index>source.text.size())return {S::InvalidText};
    return {S::Ready,index==source.text.size()?char16_t{}:source.text[index]};
}
MessageLookupStatus NativeMessageLookup::SetArgument(std::int32_t index,
    std::optional<std::u16string_view> text) {
    if(expansion_depth_)return S::Busy;
    if(!ValidText(text))return S::InvalidText;
    const std::u16string stable(*text);
    CopyText(arguments_[ArgumentIndex(index)],stable);
    return S::Ready;
}
MessageLookupStatus NativeMessageLookup::SetArgument(std::int32_t index,std::int32_t value) {
    const auto number=std::to_string(value);
    const std::u16string wide(number.begin(),number.end());
    return SetArgument(index,std::u16string_view(wide));
}
MessageLookupStatus NativeMessageLookup::SetLinkName(std::int32_t index,
    std::optional<std::u16string_view> text) {
    if(expansion_depth_)return S::Busy;
    // Unlike argument setters, the original link setter has no clamp. Refuse
    // addresses outside its two owned slots instead of silently choosing zero.
    if(static_cast<std::uint32_t>(index)>=links_.size())return S::InvalidIndex;
    if(!ValidText(text))return S::InvalidText;
    const std::u16string stable(*text);
    CopyText(links_[static_cast<std::size_t>(index)],stable);
    return S::Ready;
}
std::span<const char16_t> NativeMessageLookup::argument_words(std::int32_t index) const noexcept {
    return arguments_[ArgumentIndex(index)];
}
std::optional<std::size_t> NativeMessageLookup::argument_buffer_size(std::int32_t index) const noexcept {
    if(static_cast<std::uint32_t>(index)>=arguments_.size())return {};
    return arguments_[static_cast<std::size_t>(index)].size();
}
std::span<const char16_t> NativeMessageLookup::link_words(std::int32_t index) const noexcept {
    if(static_cast<std::uint32_t>(index)>=links_.size())return {};
    return links_[static_cast<std::size_t>(index)];
}
MessageLookupResult NativeMessageLookup::BufferResult() const {
    return {S::Ready,MessageLookupOrigin::Buffer,std::u16string(buffer_.data()),{},0,buffer_identity_};
}
MessageLookupResult NativeMessageLookup::Get(std::optional<std::string_view> identifier,
    const PlayerNameResolver& player_name) {
    if(buffer_.empty())return {S::InvalidBuffer};
    if(expansion_depth_>=64)return {S::RecursionLimit};
    // Refusals are host admission failures, not an original game return. Keep
    // the shared buffer unchanged so a caller can supply the missing owner.
    const auto before=buffer_;
    ++expansion_depth_;
    MessageLookupResult result;
    try {result=GetInternal(identifier,player_name);}
    catch(...) {--expansion_depth_;buffer_=before;throw;}
    --expansion_depth_;
    if(result.status!=S::Ready && result.status!=S::Missing)buffer_=before;
    return result;
}
MessageLookupResult NativeMessageLookup::GetInternal(std::optional<std::string_view> identifier,
    const PlayerNameResolver& player_name) {
    if(!identifier)return {S::Missing};
    auto message=Resolve(identifiers_,*identifier);
    if(message.status!=S::Ready)return message;
    auto common_name=std::string(*identifier)+"_COM";
    common_name.resize(std::min(common_name.size(),std::size_t{0x4f}));
    const auto common=Resolve(identifiers_,common_name);
    if(common.status!=S::Ready && common.status!=S::Missing)return {common.status};
    std::size_t common_length{};
    if(common.status==S::Ready) {
        common_length=common.text.size();
        if(common_length>=buffer_.size())return {S::BufferTooSmall};
        CopyText(std::span<char16_t>(buffer_).first(common_length+1),common.text);
    }
    if(message.text.starts_with(u"$a")) {
        const auto status=Expand(std::u16string_view(message.text).substr(2),common_length,player_name);
        return status==S::Ready?BufferResult():MessageLookupResult{status};
    }
    if(common.status!=S::Ready)return message;
    const auto count=std::min(message.text.size(),buffer_.size()-common_length);
    std::copy_n(message.text.begin(),count,buffer_.begin()+common_length);
    buffer_[std::min(common_length+count,buffer_.size()-1)]=0;
    return BufferResult();
}
MessageLookupStatus NativeMessageLookup::Expand(std::u16string_view input,std::size_t output,
    const PlayerNameResolver& player_name) {
    // The original end is relative to THIS output pointer. A common prefix
    // does not reduce capacity. Actual writes are fenced against the owned
    // allocation; an original overrun is a refusal, never a shorter expansion.
    const auto end=output+buffer_.size()-1;
    const auto terminate=[&]() {
        if(output>=buffer_.size())return S::BufferTooSmall;
        buffer_[output]=0;return S::Ready;
    };
    std::size_t at{};
    while(at<input.size() && output<end) {
        std::u16string expansion;
        std::size_t consumed{};
        bool from_buffer{};
        if(input[at]==u'$' && at+1<input.size()) {
            if(input[at+1]==u'a') {
                if(at+2>=input.size())return S::InvalidText;
                const auto index=static_cast<std::int32_t>(input[at+2])-u'0';
                expansion=arguments_[ArgumentIndex(index)].data();consumed=3;
            } else if(input[at+1]==u'N' && at+2<input.size()) {
                if(input[at+2]==u'l') {
                    if(at+3>=input.size())return S::InvalidText;
                    const auto index=static_cast<std::uint32_t>(input[at+3])-u'0';
                    expansion=links_[index<links_.size()?index:0].data();consumed=4;
                } else if(input[at+2]==u'u') {
                    if(!player_name)return S::DependencyUnavailable;
                    auto name=player_name();
                    if(name.status==S::Missing)name=Get(kDefaultName,player_name);
                    if(name.status!=S::Ready && name.status!=S::Missing)return name.status;
                    from_buffer=name.origin==MessageLookupOrigin::Buffer;
                    if(from_buffer) {
                        if(name.buffer_identity!=buffer_identity_)return S::InvalidBuffer;
                        expansion=buffer_.data();
                    } else {
                        if(!ValidText(std::u16string_view(name.text)))return S::InvalidText;
                        expansion=std::move(name.text);
                    }
                    consumed=3;
                }
            }
        }
        if(!consumed) {
            if(output>=buffer_.size())return S::BufferTooSmall;
            buffer_[output++]=input[at++];continue;
        }
        const auto length=expansion.size();
        // Equality stops the whole expansion, leaving the current output
        // terminated. This is stricter than the single-code-unit copy branch.
        if(output+length>=end)return terminate();
        // Copy from the LIVE shared buffer when GetName/default Get returned it.
        // A forward overlapping copy can replace its own source terminator;
        // do not substitute a snapshot/memmove or conceal a later overrun.
        std::size_t copied{};
        while(copied<buffer_.size()) {
            const auto word=from_buffer?buffer_[copied]:
                (copied<expansion.size()?expansion[copied]:char16_t{});
            if(!word)break;
            if(output+copied>=buffer_.size())return S::BufferTooSmall;
            buffer_[output+copied]=word;++copied;
        }
        const auto terminator=output+std::min(copied,buffer_.size()-1);
        if(terminator>=buffer_.size())return S::BufferTooSmall;
        buffer_[terminator]=0;
        output+=length;at+=consumed;
    }
    return terminate();
}
}
