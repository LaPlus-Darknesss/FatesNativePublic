#include "fates/runtime/native_talk_control_arguments.hpp"
#include <limits>
namespace fates::runtime::native {
TalkDecimalResult TalkDecimalize(const NativeTalkTokens::Reader& source,std::size_t begin) {
    TalkDecimalResult result;result.next=begin;std::size_t at=begin,reads{};bool negative=false;
    const auto read=[&](std::size_t index)->std::optional<char16_t>{
        if(++reads>65536){result.status=TalkArgumentStatus::ReadLimit;return {};}
        return source?source(index):std::nullopt;
    };
    const auto advance=[&](){if(at==std::numeric_limits<std::size_t>::max()){result.status=TalkArgumentStatus::Overflow;return false;}++at;return true;};
    auto word=read(at);if(!word)return result;
    if(*word==u'-'){negative=true;if(!advance())return result;word=read(at);if(!word)return result;}
    while(*word>=u'0' && *word<=u'9') {
        result.value=result.value*10u+static_cast<std::uint32_t>(*word-u'0');
        if(!advance())return result;
        word=read(at);if(!word)return result;
    }
    if(negative)result.value=0u-result.value;
    result.next=at;result.consumed=at-begin;result.status=TalkArgumentStatus::Ready;return result;
}
TalkColorArgumentResult TalkGetColor8(const NativeTalkTokens::Reader& source,std::size_t begin,TalkArgumentColor initial) {
    TalkColorArgumentResult result;result.next=begin;result.color=initial;
    for(std::size_t channel=0;channel<4;++channel) {
        const auto value=TalkDecimalize(source,result.next);
        if(value.status!=TalkArgumentStatus::Ready){result.status=value.status;return result;}
        if(value.consumed){result.color.bytes[channel]=static_cast<std::uint8_t>(value.value);result.color.known.set(channel);}
        if(value.next==std::numeric_limits<std::size_t>::max()){result.status=TalkArgumentStatus::Overflow;return result;}
        result.next=value.next+1;
    }
    result.status=TalkArgumentStatus::Ready;return result;
}
}
