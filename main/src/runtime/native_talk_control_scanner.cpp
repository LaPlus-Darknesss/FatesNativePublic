#include "fates/runtime/native_talk_control_scanner.hpp"
#include <limits>

namespace fates::runtime::native {
namespace {
using S=TalkCodeStatus;
struct Code {char16_t name;std::uint32_t vtable;std::array<std::uint32_t,3> methods;};
// Original constructor1EFEE0:123slots share TcdiNone, these20 are replaced.
constexpr Code Default{0,0x695454,{0x4f323c,0x21934c,0x4f3234}};
constexpr std::array<Code,20> Codes{{
 {u't',0x68de94,{0x1c0420,0x1c040c,0x1c0418}}, {u'C',0x695424,{0x4f3174,0x4f3160,0x4f316c}},
 {u'F',0x69543c,{0x4f31b4,0x4f3188,0x4f3194}}, {u'B',0x68f9c4,{0x1d53dc,0x21934c,0x1d53a4}},
 {u'W',0x68c2c0,{0x19049c,0x19028c,0x190460}}, {u'k',0x68d44c,{0x1a6574,0x21934c,0x1a656c}},
 {u'p',0x68de7c,{0x1c03bc,0x1c03a4,0x1c03b4}}, {u'w',0x695484,{0x4f3338,0x21934c,0x4f32f0}},
 {u'c',0x69287c,{0x216d7c,0x21934c,0x216d68}}, {u'i',0x68f9f4,{0x1d5924,0x21934c,0x1d5910}},
 {u'S',0x6960ac,{0x504928,0x504298,0x5045e0}}, {u'P',0x69546c,{0x4f3264,0x21934c,0x4f3244}},
 {u'V',0x68d464,{0x1a6638,0x21934c,0x1a6618}}, {u'E',0x68f9dc,{0x1d5770,0x21934c,0x1d56e8}},
 {u'L',0x6949f0,{0x4e09b0,0x4e099c,0x4e09a8}}, {u'e',0x68de64,{0x1c039c,0x21934c,0x1c0394}},
 {u'm',0x68fa0c,{0x1d5944,0x21934c,0x1d593c}}, {u'b',0x6949d8,{0x4e0988,0x4e0964,0x4e0968}},
 {u'l',0x68c2a8,{0x190200,0x21934c,0x1901e0}}, {u'T',0x6960d8,{0x504a58,0x21934c,0x504a38}}
}};
struct Scan {
    const NativeTalkControlScanner::Reader& source;NativeTalkTokens& tokens;
    S status{S::Ready};std::size_t reads{};
    std::optional<char16_t> Read(std::size_t at) {
        if(status!=S::Ready)return {};
        if(reads==65536){status=S::ReadLimit;return {};}
        ++reads;auto result=source?source(at):std::nullopt;
        if(!result)status=S::InvalidSource;
        return result;
    }
    bool Add(std::size_t& at,std::size_t amount) {
        if(amount>std::numeric_limits<std::size_t>::max()-at){status=S::Overflow;return false;}
        at+=amount;return true;
    }
    bool Token(std::size_t& at) {
        const auto result=tokens.Skip([this](std::size_t offset){return Read(offset);},at);
        if(result.status!=TalkTextStatus::Ready){if(status==S::Ready)status=S::InvalidSource;return false;}
        return Add(at,result.words);
    }
    bool Number(std::size_t& at) {
        // Original scanners do not validate the separator: consume one word after
        // optional '-' and digits, even when that word is NUL or punctuation.
        auto word=Read(at);if(!word)return false;
        if(*word==u'-'){if(!Add(at,1))return false;word=Read(at);if(!word)return false;}
        while(*word>=u'0' && *word<=u'9') {
            if(!Add(at,1))return false;
            word=Read(at);if(!word)return false;
        }
        return Add(at,1);
    }
};
}
std::optional<TalkCodeRoute> NativeTalkControlScanner::Route(char16_t command,TalkCodeOperation operation) {
    const auto method=static_cast<std::uint8_t>(operation);
    if(command>=123 || method>=3)return {};
    const auto* code=&Default;
    for(const auto& item:Codes)if(item.name==command){code=&item;break;}
    return TalkCodeRoute{command,code->vtable,code->methods[method],code!=&Default};
}
TalkCodeResult NativeTalkControlScanner::Skip(const Reader& source,std::size_t start) {
    Scan scan{source,tokens_};auto at=start;
    if(!scan.Add(at,1))return {scan.status,{}, {},scan.reads};
    const auto command=scan.Read(at);if(!command)return {scan.status,{}, {},scan.reads};
    const auto route=Route(*command,TalkCodeOperation::Skip);
    if(!route)return {S::InvalidCode,{}, {},scan.reads};
    if(!scan.Add(at,1))return {scan.status,*route,{},scan.reads};
    auto result=[&](bool success=true)->TalkCodeResult {return {success?S::Ready:scan.status,*route,success?std::optional{at}:std::nullopt,scan.reads};};
    if(!route->known_handler)return {S::Ready,*route,{},scan.reads};
    if(*command==u'C'||*command==u'k'||*command==u'p')return result();
    if(*command==u't'||*command==u'L'||*command==u'e'||*command==u'm')return result(scan.Add(at,1));
    if(*command==u'w')return result(scan.Number(at));
    if(*command==u'c'||*command==u'i') {
        // GetColor8's temporary output is private to Skip. No font/icon side
        // effect is performed here; preserve its four numeric cursor advances.
        for(unsigned i=0;i<4;++i)if(!scan.Number(at))return result(false);
        return result();
    }
    if(*command==u'b'||*command==u'l') {
        // These original handlers intentionally scan through NUL until '|'.
        for(;;){const auto word=scan.Read(at);if(!word)return result(false);if(!scan.Add(at,1))return result(false);if(*word==u'|')return result();}
    }
    if(*command==u'P'||*command==u'V'||*command==u'T')return result(scan.Token(at));
    if(*command==u'F')return result(scan.Add(at,1)&&scan.Token(at));
    if(*command==u'B'||*command==u'W') {
        const auto sub=scan.Read(at);if(!sub)return result(false);
        if(!scan.Add(at,1))return result(false);
        if(*sub==u'm'||*sub==u's'||(*command==u'B'&&*sub==u'c'))if(!scan.Token(at))return result(false);
        if(*command==u'W'&&*sub==u'm')return result(scan.Add(at,1));
        return result();
    }
    if(*command==u'E') {
        bool comma=false;auto look=at;
        for(;;) {const auto word=scan.Read(look);if(!word)return result(false);
            if(*word==0||*word==u'|')break;
            if(*word==u','){comma=true;break;}
            if(!scan.Add(look,1))return result(false);
        }
        if(!scan.Token(at))return result(false);
        return result(!comma||scan.Token(at));
    }
    if(*command==u'S') {
        const auto family=scan.Read(at);auto sub_at=at;if(!scan.Add(sub_at,1))return result(false);
        const auto sub=scan.Read(sub_at);if(!family||!sub)return result(false);
        if(!scan.Add(at,2))return result(false);
        if(*family==u'b'||*family==u'r') {
            if(*sub==u'p')return result(scan.Token(at)&&scan.Number(at));
            if(*sub==u'e'||*sub==u's')return result(scan.Number(at));
            if(*sub==u'v')return result(scan.Number(at)&&scan.Number(at));
        } else if(*family==u's'||*family==u'v') {
            if(*sub==u'p'||*sub==u'j'||*sub==u'e')return result(scan.Token(at));
        } else if(*family==u'l') {
            if(*sub==u'p'||*sub==u's')return result(scan.Token(at)&&scan.Number(at));
            if(*sub==u'v')return result(scan.Token(at)&&scan.Number(at)&&scan.Number(at));
        }
        return result();
    }
    return {S::InvalidCode,*route,{},scan.reads};
}
TalkCodeResult NativeTalkControlScanner::Skip(std::span<const char16_t> input,std::size_t start) {
    return Skip([&](std::size_t at)->std::optional<char16_t>{return at<input.size()?std::optional{input[at]}:std::nullopt;},start);
}
TalkCodeResult NativeTalkControlScanner::Skip(const NativeMessageLookup& owner,const MessageLookupResult& input,std::size_t start) {
    return Skip([&](std::size_t at)->std::optional<char16_t>{const auto word=owner.ReadSourceWord(input,at);return word.status==MessageLookupStatus::Ready?std::optional{word.value}:std::nullopt;},start);
}
TalkCodeResult NativeTalkControlScanner::Skip(const NativeTalkText& owner,std::size_t start) {
    return Skip([&](std::size_t at)->std::optional<char16_t>{const auto& buffer=owner.buffer();return at<buffer.words.size()&&buffer.known[at]?std::optional{buffer.words[at]}:std::nullopt;},start);
}
}
