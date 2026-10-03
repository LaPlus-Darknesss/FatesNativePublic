#include "fates/graphics/portable_texture_resource.hpp"
#include "fates/graphics/portable_bch.hpp"
#include <algorithm>
#include <set>
namespace fates::graphics::portable {
using namespace io::native;
namespace {
std::uint32_t Word(std::span<const std::uint8_t> bytes,std::size_t at) {
    return std::uint32_t(bytes[at])|(std::uint32_t(bytes[at+1])<<8)|
        (std::uint32_t(bytes[at+2])<<16)|(std::uint32_t(bytes[at+3])<<24);
}
bool Fits(std::size_t at,std::size_t count,std::size_t extent) {return at<=extent && count<=extent-at;}
// TexCommand's retail helpers inspect the first packet with the requested
// header, not the last flattened PICA write. Keeping this walk separate from
// portable renderer state composition preserves repeated-register behavior.
std::optional<std::size_t> FirstPacket(const BchFile& file,std::size_t at,std::size_t count,std::uint16_t wanted) {
    const auto bytes=file.Bytes();
    if(count>bytes.size()/4 || !Fits(at,count*4,bytes.size()))return {};
    std::size_t index=0;
    while(index+1<count) {
        const auto header=Word(bytes,at+4*(index+1));
        if(static_cast<std::uint16_t>(header)==wanted)return at+4*index;
        const auto next=index+2+((header>>20)&255u);
        index=std::min(next,count);
        if(index!=count && (index&1u))++index;
    }
    return {};
}
bool Catalog(const BchFile& file,std::vector<NativeTextureDescription>& output,std::string& error) {
    auto fail=[&](const char* reason){error=reason;return false;};
    for(unsigned category=0;category<15;++category) {
        if(category==3)continue;
        std::vector<std::size_t> entries;
        if(!file.Category(category,entries,error))return false;
        if(!entries.empty())return fail("Resource requires a non-texture H3D owner");
    }
    std::vector<std::size_t> entries;
    if(!file.Category(3,entries,error))return false;
    if(entries.size()>65535)return fail("Resource texture count exceeds original u16 domain");
    const auto bytes=file.Bytes();std::vector<NativeTextureDescription> result;std::set<std::string> names;
    for(const auto at:entries) {
        if(!Fits(at,32,bytes.size()))return fail("Truncated texture content");
        std::optional<std::string> name;
        if(!file.ReadString(at+28,name,error))return false;
        if(!name || !names.insert(*name).second)return fail("Unnamed or duplicate texture requires dictionary ownership");
        const auto commands=file.Resolve(at);const auto count=Word(bytes,at+4);
        if(!commands || !commands->offset || commands->target_type!=2)return fail("Texture commands have no owned relocation");
        const auto dimensions=FirstPacket(file,*commands->offset,count,0x82);
        const auto address=FirstPacket(file,*commands->offset,count,0x85);
        if(!dimensions || !address)return fail("Missing original unit0 texture packet");
        const auto pointer=file.Resolve(*address);
        if(!pointer || !pointer->offset || pointer->target_type!=5)return fail("Texture address requires non-inline storage");
        NativeTextureDescription description;description.name=*name;
        const auto size=Word(bytes,*dimensions);
        description.width=static_cast<std::uint16_t>(size>>16);description.height=static_cast<std::uint16_t>(size);
        description.format=bytes[at+24];description.mip_count=bytes[at+25];
        description.source_offset=*pointer->offset;
        std::uint32_t packed_size{};
        if(!ComputeNativeTextureImageSize(description.width,description.height,description.format,description.mip_count,packed_size))
            return fail("Texture size arithmetic outside original admitted domain");
        description.packed_size=packed_size;
        const auto section=file.SectionOffsets()[3];const auto length=Word(bytes,0x2c);
        if(description.source_offset<section || !Fits(description.source_offset-section,description.packed_size,length))
            return fail("Texture mip storage escapes the owned data section");
        result.push_back(std::move(description));
    }
    output=std::move(result);error.clear();return true;
}
class Resource final:public NativeTextureResource {
public:
    std::shared_ptr<const BchFile> file;
    std::vector<NativeTextureDescription> descriptions;
    std::span<const NativeTextureDescription> contents() const noexcept override {return descriptions;}
    bool DecodeBase(std::size_t index,TextureImage& output,std::string& error) const override {
        if(index>=descriptions.size()){error="Texture index outside retained resource";return false;}
        const auto& description=descriptions[index];
        return fates::graphics::portable::DecodeTextureBase(file->Bytes().subspan(description.source_offset,description.packed_size),
            description.format,description.width,description.height,output,error);
    }
};
}
NativeTextureResourceBackend CreatePortableTextureResourceBackend() {
    return {[](std::span<const std::uint8_t> bytes,std::shared_ptr<const NativeTextureResource>& output,std::string& error){
        auto resource=std::make_shared<Resource>();
        if(!BchFile::Decode(bytes,resource->file,error) || !Catalog(*resource->file,resource->descriptions,error))return false;
        output=std::move(resource);return true;
    }};
}
}
