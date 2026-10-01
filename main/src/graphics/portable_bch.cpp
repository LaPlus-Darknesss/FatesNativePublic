#include "fates/graphics/portable_bch.hpp"
#include "fates/runtime/native_archive.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace fates::graphics::portable {
namespace {
bool Magic(std::span<const std::uint8_t> bytes) {
    return bytes.size()>=4&&bytes[0]=='B'&&bytes[1]=='C'&&bytes[2]=='H'&&bytes[3]==0;
}
bool Fail(std::string& error,const char* cause){error=cause;return false;}
std::uint32_t ReadWord(std::span<const std::uint8_t> bytes,std::size_t at) {
    return std::uint32_t(bytes[at])|(std::uint32_t(bytes[at+1])<<8)|
        (std::uint32_t(bytes[at+2])<<16)|(std::uint32_t(bytes[at+3])<<24);
}
}
bool BchFile::Fits(std::size_t at,std::size_t size) const noexcept {return at<=bytes_.size()&&size<=bytes_.size()-at;}
std::uint32_t BchFile::Word(std::size_t at) const {
    if(!Fits(at,4))throw std::out_of_range("BCH word outside file");return ReadWord(bytes_,at);
}
bool BchFile::Decode(std::span<const std::uint8_t> input,std::shared_ptr<const BchFile>& output,std::string& error) {
    auto next=std::make_shared<BchFile>();
    if(Magic(input))next->bytes_.assign(input.begin(),input.end());
    else if(input.size()>=8&&input[0]==0&&Magic(input.subspan(4)))next->bytes_.assign(input.begin()+4,input.end());
    else if(input.size()>=8&&input[0]==0x13&&input[4]==0x11) {
        if(!runtime::native::DecompressFe14Archive(input,next->bytes_))return Fail(error,"Invalid FE14 BCH transport");
    } else if(input.size()>=4&&input[0]==0x11) {
        std::vector<std::uint8_t> wrapped{0x13,0,0,0};wrapped.insert(wrapped.end(),input.begin(),input.end());
        if(!runtime::native::DecompressFe14Archive(wrapped,next->bytes_))return Fail(error,"Invalid LZ11 BCH transport");
    } else return Fail(error,"Unsupported BCH transport");
    if(!Magic(next->bytes_)||!next->Fits(0,0x44))return Fail(error,"Invalid BCH header");
    if(next->bytes_[4]!=0x22||next->bytes_[5]!=0x23)return Fail(error,"Unsupported BCH compatibility version");
    for(unsigned i=0;i<6;++i) {
        next->offsets_[i]=next->Word(8+4*i);next->lengths_[i]=next->Word(0x20+4*i);
        if(!next->Fits(next->offsets_[i],next->lengths_[i]))return Fail(error,"BCH section outside file");
    }
    if(next->lengths_[5]%4)return Fail(error,"Incomplete BCH relocation record");
    const std::size_t end=std::size_t(next->offsets_[5])+next->lengths_[5];
    for(std::size_t at=next->offsets_[5];at<end;at+=4) {
        const auto encoded=next->Word(at);const auto source=(encoded>>29)&7u;
        const auto target=static_cast<std::uint8_t>((encoded>>25)&15u);const auto low=encoded&0x1ffffffu;
        // Lab3/Lab19 serialized relocation contract. Other source categories are
        // retained in Bytes; no host pointer or guessed target is constructed.
        if(source!=0&&source!=2)continue;
        const std::size_t relative=std::size_t(low)*((source==0&&target==1)?1u:4u);
        if(relative>next->lengths_[source]||next->lengths_[source]-relative<4)return Fail(error,"BCH relocation source outside section");
        next->relocations_[std::size_t(next->offsets_[source])+relative]=target;
    }
    output=std::move(next);error.clear();return true;
}
std::optional<BchPointer> BchFile::Resolve(std::size_t field) const {
    const auto it=relocations_.find(field);if(it==relocations_.end())return {};
    const auto target=it->second;std::optional<unsigned> section;
    switch(target) {
    case 0:section=0;break;case 1:section=1;break;case 2:section=2;break;
    case 5:section=3;break;case 11:case 12:case 13:section=4;break;
    default:break;
    }
    BchPointer pointer{target,Word(field),{}};
    if(section)pointer.offset=std::size_t(offsets_[*section])+(pointer.stored&0x7fffffffu);
    return pointer;
}
bool BchFile::ReadString(std::size_t field,std::optional<std::string>& output,std::string& error) const {
    if(!Fits(field,4))return Fail(error,"BCH string pointer outside file");
    const auto pointer=Resolve(field);
    if(!pointer){if(Word(field))return Fail(error,"Unrelocated BCH string pointer");output.reset();error.clear();return true;}
    if(pointer->target_type!=1||!pointer->offset)return Fail(error,"BCH string target is not a string section");
    const auto start=*pointer->offset;const std::size_t end=std::size_t(offsets_[1])+lengths_[1];
    if(start<offsets_[1]||start>=end)return Fail(error,"BCH string outside section");
    auto stop=start;while(stop<end&&bytes_[stop])++stop;if(stop==end)return Fail(error,"Unterminated BCH string");
    output=std::string(reinterpret_cast<const char*>(bytes_.data()+start),stop-start);error.clear();return true;
}
bool BchFile::Category(unsigned index,std::vector<std::size_t>& output,std::string& error) const {
    if(index>=15||lengths_[0]<15u*12u)return Fail(error,"BCH category table missing");
    const std::size_t field=std::size_t(offsets_[0])+12*index;const auto count=Word(field+4);
    std::vector<std::size_t> next;
    if(count) {
        const auto table=Resolve(field);
        if(!table||table->target_type!=0||!table->offset||count>bytes_.size()/4||!Fits(*table->offset,std::size_t(count)*4))return Fail(error,"Invalid BCH category array");
        next.reserve(count);
        for(std::size_t i=0;i<count;++i) {
            const auto entry=Resolve(*table->offset+4*i);
            if(!entry||entry->target_type!=0||!entry->offset||!Fits(*entry->offset,4))return Fail(error,"Invalid BCH category object");
            next.push_back(*entry->offset);
        }
    }
    output=std::move(next);error.clear();return true;
}
bool BchFile::Commands(std::size_t offset,std::size_t count,std::vector<BchCommandWrite>& output,std::string& error) const {
    if(count>bytes_.size()/4||!Fits(offset,count*4))return Fail(error,"BCH command stream outside file");
    std::vector<BchCommandWrite> next;std::size_t i=0;
    while(i<count) {
        if(count-i<2)return Fail(error,"Incomplete BCH command packet");
        const auto head=Word(offset+4*(i+1));const auto reg=head&0x3ffu;const auto extra=(head>>20)&0x7ffu;
        const bool sequential=(head&0x80000000u)!=0;const std::size_t words=2+extra;
        if(words>count-i)return Fail(error,"Incomplete BCH command parameters");
        for(unsigned k=0;k<=extra;++k) {
            const auto at=offset+4*(k?i+1+k:i);
            next.push_back({reg+(sequential?k:0),Word(at),static_cast<std::uint8_t>((head>>16)&15u),at,Resolve(at)});
        }
        i+=words+(words&1u);if(i>count)return Fail(error,"Missing BCH command padding");
    }
    output=std::move(next);error.clear();return true;
}
bool ReadBchModels(const BchFile& file,std::vector<BchModelDescriptor>& output,std::string& error) {
    std::vector<std::size_t> objects;if(!file.Category(0,objects,error))return false;
    std::vector<BchModelDescriptor> next;const auto bytes=file.Bytes();
    for(auto at:objects) {
        if(at>bytes.size()||bytes.size()-at<0x88)return Fail(error,"BCH model descriptor outside file");
        BchModelDescriptor model;model.offset=at;if(!file.ReadString(at+0x84,model.name,error))return false;
        // Lab16: 12 serialized floats at +4, affine row-dot matrix. The material
        // pointer at +0x34 corroborates the end; this is not a second pose owner.
        for(unsigned i=0;i<12;++i)model.matrix[i]=std::bit_cast<float>(ReadWord(bytes,at+4+4*i));
        next.push_back(std::move(model));
    }
    output=std::move(next);error.clear();return true;
}
bool ReadBchTextures(const BchFile& file,std::vector<BchTextureDescriptor>& output,std::string& error) {
    std::vector<std::size_t> objects;if(!file.Category(3,objects,error))return false;
    std::vector<BchTextureDescriptor> next;const auto bytes=file.Bytes();
    for(auto at:objects) {
        if(at>bytes.size()||bytes.size()-at<0x20)return Fail(error,"BCH texture descriptor outside file");
        BchTextureDescriptor texture;if(!file.ReadString(at+0x1c,texture.name,error))return false;
        if(!texture.name)continue;
        const auto pointer=file.Resolve(at);const auto words=ReadWord(bytes,at+4);
        if(!pointer||pointer->target_type!=2||!pointer->offset||!words)return Fail(error,"BCH texture command unresolved");
        std::vector<BchCommandWrite> writes;if(!file.Commands(*pointer->offset,words,writes,error))return false;
        const BchCommandWrite *dimensions=nullptr,*address=nullptr,*type=nullptr;
        for(const auto& value:writes) {
            if(value.register_index==0x82)dimensions=&value;
            else if(value.register_index==0x85)address=&value;
            else if(value.register_index==0x8e)type=&value;
        }
        if(!dimensions||!address||!type||!address->relocation||!address->relocation->offset)return Fail(error,"BCH texture storage unresolved");
        if(dimensions->byte_mask!=15||address->byte_mask!=15||type->byte_mask!=15)return Fail(error,"BCH texture storage needs masked register composition");
        texture.width=dimensions->value>>16;texture.height=dimensions->value&0xffffu;
        texture.format=type->value&15u;texture.data_offset=*address->relocation->offset;
        if(texture.data_offset>bytes.size())return Fail(error,"BCH texture data outside file");
        next.push_back(std::move(texture));
    }
    output=std::move(next);error.clear();return true;
}
}
