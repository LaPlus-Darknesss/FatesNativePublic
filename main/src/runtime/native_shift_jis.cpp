#include "fates/runtime/native_shift_jis.hpp"
#include <algorithm>
#include <array>
#include <limits>
namespace fates::runtime::native {
namespace {
constexpr std::size_t Base=0x100000,RangeAddress=0x611fb4,ToUnicodeAddress=0x6be5bc,ToSjisAddress=0x6c3fbc;
constexpr std::array<std::array<std::uint32_t,2>,5> Ranges{{{{0,1152}},{{0x2000,1664}},{{0x3000,1024}},{{0x4e00,20904}},{{0xf928,1726}}}};
constexpr std::size_t UnicodeCount=60*192,SjisBytes=26470*2;
std::uint32_t U32(std::span<const std::uint8_t> bytes,std::size_t offset) {
    return std::uint32_t(bytes[offset])|(std::uint32_t(bytes[offset+1])<<8)|(std::uint32_t(bytes[offset+2])<<16)|(std::uint32_t(bytes[offset+3])<<24);
}
std::uint32_t Limit(std::optional<std::int32_t> value) {
    return value && *value>=0?static_cast<std::uint32_t>(*value):static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max());
}
ShiftJisResult Stop(ShiftJisResult value,ShiftJisStatus status){value.status=status;return value;}
}
ShiftJisStatus NativeShiftJis::FromOriginalCode(std::span<const std::uint8_t> code,std::shared_ptr<const NativeShiftJis>& out) {
    if(code.size()<ToSjisAddress-Base+SjisBytes)return ShiftJisStatus::InvalidTableImage;
    for(std::size_t i=0;i<Ranges.size();++i)for(std::size_t word=0;word<2;++word)
        if(U32(code,RangeAddress-Base+8*i+4*word)!=Ranges[i][word])return ShiftJisStatus::InvalidTableImage;
    if(U32(code,0x164208-Base)!=RangeAddress || U32(code,0x16420c-Base)!=ToSjisAddress || U32(code,0x13392c-Base)!=ToUnicodeAddress)
        return ShiftJisStatus::InvalidTableImage;
    auto tables=std::make_shared<NativeShiftJis>();
    tables->to_sjis_.assign(code.begin()+static_cast<std::ptrdiff_t>(ToSjisAddress-Base),code.begin()+static_cast<std::ptrdiff_t>(ToSjisAddress-Base+SjisBytes));
    tables->to_unicode_.reserve(UnicodeCount);
    for(std::size_t i=0;i<UnicodeCount;++i) {
        const auto offset=ToUnicodeAddress-Base+2*i;
        tables->to_unicode_.push_back(static_cast<std::uint16_t>(std::uint16_t(code[offset])|static_cast<std::uint16_t>(std::uint16_t(code[offset+1])<<8)));
    }
    // Existing admitted ASCII path is the actual original table, including DEL.
    for(std::size_t i=1;i<=127;++i)if(tables->to_sjis_[2*i]!=static_cast<std::uint8_t>(i) || tables->to_sjis_[2*i+1]!=0)return ShiftJisStatus::InvalidTableImage;
    out=std::move(tables);return ShiftJisStatus::Ready;
}
std::shared_ptr<const NativeShiftJis> NativeShiftJis::AsciiSubset(){static const auto value=std::make_shared<const NativeShiftJis>();return value;}
ShiftJisResult NativeShiftJis::ToSjis(const WordReader& source,std::optional<std::span<std::uint8_t>> destination,
    std::optional<std::int32_t> source_limit,std::optional<std::int32_t> destination_limit,std::size_t budget) const {
    ShiftJisResult result;
    if(!source){result.code=2;return result;}
    const auto input=Limit(source_limit),output=destination?Limit(destination_limit):Limit({});
    while(result.produced<output && result.consumed<input) {
        if(!budget--)return Stop(result,ShiftJisStatus::ReadLimit);
        const auto word=source(result.consumed);if(!word)return Stop(result,ShiftJisStatus::Unavailable);
        if(!*word)break;
        std::array<std::uint8_t,2> bytes{};std::uint32_t count{};
        const auto value=static_cast<std::uint32_t>(*word);
        if(to_sjis_.empty()) {
            if(value>127u)return Stop(result,ShiftJisStatus::TablesUnavailable);
            bytes[0]=static_cast<std::uint8_t>(value);count=1;
        } else if(value-0xe000u<0x18ffu) {
            const auto offset=value-0xe000u;const auto lead=offset/188u;const auto trail=offset-lead*188u;
            bytes={static_cast<std::uint8_t>(lead+0xf0u),static_cast<std::uint8_t>(trail+(trail>=63u?65u:64u))};count=2;
        } else {
            std::size_t base=0;bool mapped=false;
            for(const auto& range:Ranges) {
                if(value<range[0])break;
                const auto index=value-range[0];
                if(index<range[1]) {
                    bytes={to_sjis_[2*(base+index)],to_sjis_[2*(base+index)+1]};
                    mapped=bytes[0]!=0;count=bytes[1]?2u:1u;break;
                }
                base+=range[1];
            }
            if(!mapped){result.code=3;break;}
        }
        // Convert the character before checking remaining output space, exactly
        // as STD does. It may report invalid source despite only one byte free.
        if(count>output-result.produced)break;
        if(destination) {
            if(result.produced>destination->size() || count>destination->size()-result.produced)return Stop(result,ShiftJisStatus::DestinationUnavailable);
            for(std::uint32_t i=0;i<count;++i)(*destination)[result.produced+i]=bytes[i];
        }
        ++result.consumed;result.produced+=count;
    }
    result.counts_written=true;return result;
}
ShiftJisResult NativeShiftJis::ToUtf16(const ByteReader& source,std::optional<std::span<char16_t>> destination,
    std::optional<std::int32_t> source_limit,std::optional<std::int32_t> destination_limit,std::size_t budget) const {
    ShiftJisResult result;if(!source){result.code=2;return result;}
    const auto input=Limit(source_limit),output=destination?Limit(destination_limit):Limit({});
    const auto read=[&](std::size_t at)->std::optional<std::uint8_t>{if(!budget){result.status=ShiftJisStatus::ReadLimit;return {};}--budget;return source(at);};
    while(result.produced<output && result.consumed<input) {
        const auto first=read(result.consumed);if(!first)return Stop(result,result.status==ShiftJisStatus::Ready?ShiftJisStatus::Unavailable:result.status);
        if(!*first)break;
        std::uint32_t count=1,value{};
        if(*first<=0x7eu)value=*first;
        else if(*first>=0xa1u && *first<=0xdfu)value=static_cast<std::uint16_t>(static_cast<std::uint32_t>(*first)-0x140u);
        else {
            const auto lead=static_cast<std::uint32_t>(*first);
            if((lead^0x20u)-0xa1u>=0x3cu){result.code=3;break;}
            // Original reads trail BEFORE source-length rejection. A missing
            // accessible byte is unknown state, not a synthetic NUL/invalid code.
            const auto second=read(std::size_t(result.consumed)+1);
            if(!second)return Stop(result,result.status==ShiftJisStatus::Ready?ShiftJisStatus::Unavailable:result.status);
            if(*second==0x7fu || ((std::uint32_t(*second)-0x40u)&0xffu)>0xbcu || input-result.consumed<2u){result.code=3;break;}
            count=2;
            // The original validates a signed load, then performs a second live
            // byte read for its table index. Do not collapse these observations.
            const auto lookup=read(std::size_t(result.consumed)+1);
            if(!lookup)return Stop(result,result.status==ShiftJisStatus::Ready?ShiftJisStatus::Unavailable:result.status);
            if(to_unicode_.empty())return Stop(result,ShiftJisStatus::TablesUnavailable);
            const auto row=lead-(lead>=0xe0u?0x40u:0u)-0x81u;
            const auto index=static_cast<std::int64_t>(row)*192+*lookup-64;
            if(index<0 || static_cast<std::uint64_t>(index)>=to_unicode_.size())return Stop(result,ShiftJisStatus::Unavailable);
            value=to_unicode_[static_cast<std::size_t>(index)];
            if(!value){result.code=3;break;}
        }
        if(count>input-result.consumed)break;
        if(destination) {
            if(result.produced>=destination->size())return Stop(result,ShiftJisStatus::DestinationUnavailable);
            (*destination)[result.produced]=static_cast<char16_t>(value);
        }
        result.consumed+=count;++result.produced;
    }
    result.counts_written=true;return result;
}
ShiftJisResult NativeShiftJis::TerminatedSjis(const WordReader& source,std::span<std::uint8_t> destination,std::size_t budget) const {
    if(destination.empty() || destination.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))return Stop({},ShiftJisStatus::DestinationUnavailable);
    const auto result=ToSjis(source,destination,{},static_cast<std::int32_t>(destination.size()-1),budget);
    if(result.status==ShiftJisStatus::Ready)destination[result.counts_written?result.produced:destination.size()-1]=0;
    return result;
}
ShiftJisResult NativeShiftJis::TerminatedUtf16(const ByteReader& source,std::span<char16_t> destination,std::size_t budget) const {
    if(destination.empty() || destination.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))return Stop({},ShiftJisStatus::DestinationUnavailable);
    const auto result=ToUtf16(source,destination,{},static_cast<std::int32_t>(destination.size()-1),budget);
    if(result.status==ShiftJisStatus::Ready)destination[result.counts_written?result.produced:destination.size()-1]=0;
    return result;
}
}
