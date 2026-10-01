#include "fates/runtime/native_definition_store.hpp"
#include "fates/runtime/native_archive.hpp"
#include <algorithm>
#include <bit>
#include <optional>
#include <set>

namespace fates::runtime::native {
namespace {
struct Reader {
    const std::vector<std::uint8_t>& bytes;
    std::size_t data_end;
    std::size_t string_begin{};
    void Fits(std::size_t p, std::size_t n) const {
        if(p>data_end || n>data_end-p)throw 1;
    }
    std::uint32_t U32(std::size_t p) const {
        Fits(p,4);return std::uint32_t(bytes[p])|(std::uint32_t(bytes[p+1])<<8u)|
            (std::uint32_t(bytes[p+2])<<16u)|(std::uint32_t(bytes[p+3])<<24u);
    }
    std::size_t Pointer(std::size_t p) const {
        const auto rel=U32(p);if(!rel)throw 1;
        const auto at=std::size_t(rel)+0x20;Fits(at,1);return at;
    }
    std::string String(std::size_t p) const {
        if(!U32(p))return {};
        const auto at=std::size_t(U32(p))+0x20;
        if(at<string_begin || at>=bytes.size())throw 1;
        const auto begin=bytes.begin()+at,end=std::find(begin,bytes.end(),0);
        if(end==bytes.end())throw 1;
        return std::string(begin,end); // preserve original identifier bytes
    }
};
std::int16_t Numeric(std::string_view value) {
    if(value.empty())return -1;
    bool negative=value.front()=='-';if(negative)value.remove_prefix(1);
    if(value.empty())throw 1;
    std::uint16_t result=0;
    for(const char c:value) {
        if(c<'0'||c>'9')throw 1;
        result=static_cast<std::uint16_t>(std::uint32_t(result)*10u+unsigned(c-'0'));
    }
    if(negative)result=static_cast<std::uint16_t>(0u-result);
    return std::bit_cast<std::int16_t>(result);
}
}
bool DefinitionStore::LoadAiArchive(std::span<const std::uint8_t> raw) {
    std::vector<std::uint8_t> bytes;
    if(!DecompressFe14Archive(raw,bytes) || bytes.size()<0x30)return false;
    try {
        Reader header{bytes,bytes.size()};
        if(header.U32(0)!=bytes.size())return false;
        const auto end=std::size_t(header.U32(4))+0x20;
        if(end<0x30 || end>bytes.size())return false;
        const auto string_begin=end+std::size_t(header.U32(8))*4+std::size_t(header.U32(12))*8;
        if(string_begin>bytes.size())return false;
        Reader rd{bytes,end,string_begin};
        std::array<std::vector<AiDeclarationDefinition>,4> next;
        std::array<std::vector<std::vector<std::array<std::string,2>>>,4> strings;
        std::set<std::string> names;
        std::set<std::size_t> records;
        for(unsigned channel=0;channel<4;++channel) {
            const auto table=rd.Pointer(0x20+4*channel);
            for(std::size_t index=0;;++index) {
                if(!rd.U32(table+index*4))break;
                if(index>=256)throw 1;
                const auto at=rd.Pointer(table+index*4);rd.Fits(at,8);
                if((at&3u)||!records.insert(at).second)throw 1;
                AiDeclarationDefinition d{};d.name=rd.String(at);d.id=bytes[at+4];d.channel=bytes[at+5];
                if(d.name.empty()||d.channel!=channel||!names.insert(d.name).second)throw 1;
                std::vector<std::array<std::string,2>> values;
                for(std::size_t pos=at+8;;pos+=12) {
                    rd.Fits(pos,12);
                    AiCommandDefinition c{bytes[pos],bytes[pos+1],std::bit_cast<std::int8_t>(bytes[pos+2]),bytes[pos+3]};
                    values.push_back({rd.String(pos+4),rd.String(pos+8)});d.commands.push_back(c);
                    if(!c.kind)break;
                }
                next[channel].push_back(std::move(d));strings[channel].push_back(std::move(values));
            }
        }
        // All tables exist before resolving cross-channel declaration names.
        for(unsigned channel=0;channel<4;++channel)for(std::size_t i=0;i<next[channel].size();++i)
            for(std::size_t j=0;j<next[channel][i].commands.size();++j)for(unsigned k=0;k<2;++k) {
                const auto& text=strings[channel][i][j][k];auto& value=next[channel][i].commands[j].arguments[k];
                if(!text.empty()&&text.front()=='A') {
                    bool found=false;
                    for(const auto& table:next)for(const auto& d:table)if(d.name==text){value=d.id;found=true;}
                    if(!found)throw 1;
                } else value=Numeric(text);
            }
        ai_declarations_=std::move(next);ai_loaded_=true;return true;
    } catch(...) {return false;} // failed replacement preserves the previous complete archive
}
const AiDeclarationDefinition* DefinitionStore::FindAiDeclaration(std::uint8_t channel,std::uint8_t index) const {
    return ai_loaded_&&channel<4&&index<ai_declarations_[channel].size()?&ai_declarations_[channel][index]:nullptr;
}
} // namespace fates::runtime::native
