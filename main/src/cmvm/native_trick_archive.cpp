#include "fates/cmvm/native_trick_archive.hpp"
#include <algorithm>
#include <limits>
#include <set>
namespace fates::cmvm {
namespace {
bool Fits(std::size_t a,std::size_t n,std::size_t size) {return a<=size && n<=size-a;}
std::uint32_t U32(std::span<const std::uint8_t> b,std::size_t p) {
    return std::uint32_t(b[p])|(std::uint32_t(b[p+1])<<8u)|
        (std::uint32_t(b[p+2])<<16u)|(std::uint32_t(b[p+3])<<24u);
}
}
TrickArchiveStatus ReadScriptTrickDeclarations(std::span<const std::uint8_t> b,
    std::vector<ScriptTrickDeclaration>& out) {
    using S=TrickArchiveStatus;
    if(b.size()<0x28 || b.size()>16u*1024u*1024u || U32(b,0)!=0x00626d63u ||
       U32(b,4)!=0x20110819u)return S::InvalidHeader;
    // +0x10 is the relocated archive link. +0x24 is the attach count.
    if(U32(b,0x10)!=0 || b[0x24]!=0 || b[0x25]!=0)return S::RelocatedOrAttached;
    if(b[0x26]!=0)return S::InitializerRequired;
    const auto table=std::size_t(U32(b,0x1c)),strings=std::size_t(U32(b,0x20));
    if(table<0x28 || (table&3u) || !Fits(table,4,b.size()) || strings>b.size())return S::InvalidTable;
    std::vector<ScriptTrickDeclaration> next;std::set<std::uint32_t> records;
    bool terminated=false;
    for(std::size_t index=0;index<32768u;++index) {
        const auto entry=table+index*4u;
        if(!Fits(entry,4,b.size()))return S::InvalidTable;
        const auto record=U32(b,entry);
        if(!record){terminated=true;break;}
        if(record<0x28 || (record&3u) || !Fits(record,0x18,b.size()) || !records.insert(record).second)
            return S::InvalidRecord;
        const auto kind=b[record+8];
        if(kind!=21 && kind!=22)continue;
        const unsigned count=kind==21?8u:13u;
        if(b[record+9]!=count)return S::InvalidArguments;
        const auto args=std::size_t(U32(b,record+0x14));
        if((args&3u) || args<0x28 || !Fits(args,count*4,b.size()))return S::InvalidArguments;
        ScriptTrickDeclaration d{};d.function_index=static_cast<std::uint16_t>(index);d.event_type=kind;
        const auto numbers=kind==21?7u:13u;
        for(unsigned k=0;k<numbers;++k)d.arguments[k]=U32(b,args+k*4);
        if(kind==21) {
            const auto relative=std::size_t(U32(b,args+7*4));
            // EvArgAsStr uses the archive string base, not the archive base.
            if(relative>b.size()-strings || !Fits(strings+relative,1,b.size()))return S::InvalidString;
            const auto start=b.begin()+static_cast<std::ptrdiff_t>(strings+relative);
            const auto end=std::find(start,b.end(),std::uint8_t(0));
            if(end==b.end() || end-start>4096)return S::InvalidString;
            d.label.assign(start,end);
        }
        next.push_back(std::move(d));
        if(next.size()>4096)return S::LimitExceeded;
    }
    if(!terminated)return S::LimitExceeded;
    out=std::move(next);return S::Ok;
}
} // namespace fates::cmvm
