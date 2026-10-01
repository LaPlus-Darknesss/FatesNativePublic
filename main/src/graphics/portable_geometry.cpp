#include "fates/graphics/portable_geometry.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace fates::graphics::portable {
namespace {
// Layout/commands ported from GraphicsLab3, serialized index-width correction
// from Lab4 Repair1, complete culling ranges from Lab17, quantizers from Lab19.
// Vendor decoding is a replacement boundary, not a new Fates function promotion.
class Reader {
public:
    explicit Reader(const BchFile& value):file(value),bytes(value.Bytes()){}
    const BchFile& file;
    std::span<const std::uint8_t> bytes;
    void Need(std::size_t at,std::size_t size) const {
        if(at>bytes.size()||size>bytes.size()-at)throw std::runtime_error("BCH geometry span outside file");
    }
    std::uint32_t Word(std::size_t at) const {
        Need(at,4);return std::uint32_t(bytes[at])|(std::uint32_t(bytes[at+1])<<8)|
            (std::uint32_t(bytes[at+2])<<16)|(std::uint32_t(bytes[at+3])<<24);
    }
    std::size_t Pointer(std::size_t field,unsigned type) const {
        Need(field,4);auto pointer=file.Resolve(field);
        if(!pointer||pointer->target_type!=type||!pointer->offset)throw std::runtime_error("BCH geometry pointer unresolved");
        Need(*pointer->offset,0);return *pointer->offset;
    }
    std::size_t Array(std::size_t field,std::uint32_t count,std::size_t stride) const {
        if(!count)return 0;
        if(count>bytes.size()/stride)throw std::runtime_error("BCH geometry array count outside file");
        const auto at=Pointer(field,0);Need(at,std::size_t(count)*stride);return at;
    }
    std::vector<BchCommandWrite> Commands(std::size_t field) const {
        const auto count=Word(field+4);if(!count)return {};
        const auto at=Pointer(field,2);std::vector<BchCommandWrite> values;std::string error;
        if(!file.Commands(at,count,values,error))throw std::runtime_error(error);return values;
    }
    MeshIndexRange Indices(std::size_t field,std::uint32_t count) const {
        Need(field,4);const auto pointer=file.Resolve(field);
        if(!pointer||!pointer->offset||(pointer->target_type!=12&&pointer->target_type!=13))
            throw std::runtime_error("BCH indices require a serialized U8/U16 relocation");
        MeshIndexRange next;next.source_element_bytes=pointer->target_type==12?2u:1u;
        if(count>bytes.size()/next.source_element_bytes)throw std::runtime_error("BCH index count outside file");
        Need(*pointer->offset,std::size_t(count)*next.source_element_bytes);next.indices.reserve(count);
        for(std::size_t i=0;i<count;++i) {
            const auto at=*pointer->offset+i*next.source_element_bytes;
            const unsigned index=bytes[at]|(next.source_element_bytes==2?(unsigned(bytes[at+1])<<8):0u);
            next.indices.push_back(static_cast<std::uint16_t>(index));
        }
        return next;
    }
};
using Registers=std::map<unsigned,const BchCommandWrite*>;
Registers Last(const std::vector<BchCommandWrite>& commands) {
    Registers result;for(const auto& value:commands)result[value.register_index]=&value;return result;
}
std::uint32_t Register(const Registers& registers,unsigned reg) {
    const auto it=registers.find(reg);if(it==registers.end())return 0;
    if(it->second->byte_mask!=15)throw std::runtime_error("BCH geometry needs masked register composition: "+std::to_string(reg)+" mask "+std::to_string(it->second->byte_mask));
    return it->second->value;
}
unsigned ScalarBytes(VertexScalar type) {
    return type==VertexScalar::Float32?4u:(type==VertexScalar::SignedShort?2u:1u);
}
void ReadLayout(const Reader& reader,const Registers& registers,BchMesh& mesh) {
    const auto high=Register(registers,0x202);
    const auto formats=std::uint64_t(Register(registers,0x201))|(std::uint64_t(high)<<32);
    const auto permutation=std::uint64_t(Register(registers,0x2bb))|(std::uint64_t(Register(registers,0x2bc))<<32);
    mesh.fixed_attribute_mask=(high>>16)&0xfffu;
    for(unsigned index=0;index<12;++index) {
        const auto base=0x203+3*index;
        if(!registers.contains(base)&&!registers.contains(base+1)&&!registers.contains(base+2))continue;
        const auto first=Register(registers,base+1),second=Register(registers,base+2);
        const auto count=second>>28;
        if(count>12)throw std::runtime_error("BCH vertex buffer component count exceeds twelve");
        VertexBuffer buffer;buffer.index=index;buffer.stride=(second>>16)&255u;
        unsigned cursor=0;
        for(unsigned slot=0;slot<count;++slot) {
            const auto component=(slot<8?(first>>(4*slot)):(second>>(4*(slot-8))))&15u;
            if(component>=12){cursor=(cursor+3)&~3u;cursor+=(component-11)*4;continue;}
            if(mesh.fixed_attribute_mask&(1u<<component))continue;
            const auto format=unsigned((formats>>(4*component))&15u);
            const auto type=static_cast<VertexScalar>(format&3u);const auto width=ScalarBytes(type);
            cursor=(cursor+width-1)&~(width-1);
            const auto elements=(format>>2)+1;
            buffer.attributes.push_back({component,unsigned((permutation>>(4*component))&15u),elements,cursor,type});
            cursor+=width*elements;
        }
        if(cursor>buffer.stride)throw std::runtime_error("BCH vertex layout exceeds stride");
        const auto address=registers.find(base);
        if(address==registers.end()||address->second->byte_mask!=15||!address->second->relocation||
            address->second->relocation->target_type!=11||!address->second->relocation->offset)
            throw std::runtime_error("BCH vertex storage unresolved");
        buffer.data_offset=*address->second->relocation->offset;
        if(mesh.vertex_count&&(!buffer.stride||mesh.vertex_count>reader.bytes.size()/buffer.stride))
            throw std::runtime_error("BCH vertex count outside file");
        reader.Need(buffer.data_offset,std::size_t(mesh.vertex_count)*buffer.stride);
        mesh.buffers.push_back(std::move(buffer));
    }
}
std::optional<MeshQuantizer> ReadQuantizer(const std::vector<BchCommandWrite>& commands) {
    std::map<unsigned,std::vector<float>> uniforms;std::optional<unsigned> index;bool float32=false;
    for(const auto& value:commands) {
        if(value.register_index==0x2c0) {
            index=value.value&255u;float32=(value.value&0x80000000u)!=0;
            if(*index==6||*index==7) {
                if(value.byte_mask!=15||!float32)throw std::runtime_error("BCH quantizer requires float32 uniforms");
                uniforms[*index].clear();
            }
        } else if(value.register_index>=0x2c1&&value.register_index<=0x2c8&&index&&(*index==6||*index==7)) {
            if(value.byte_mask!=15)throw std::runtime_error("BCH quantizer needs masked uniform composition");
            uniforms[*index].push_back(std::bit_cast<float>(value.value));
        }
    }
    auto six=uniforms[6],seven=uniforms[7];if(six.size()<4||seven.size()<8)return {};
    for(auto* values:{&six,&seven})for(std::size_t i=0;i+4<=values->size();i+=4)
        std::reverse(values->begin()+i,values->begin()+i+4);
    return MeshQuantizer{{six[0],six[1],six[2]},seven[0],seven[1],seven[2],seven[3],{seven[4],seven[5],seven[6]}};
}
void StandardRanges(const Reader& reader,std::size_t mesh,BchMesh& output) {
    const auto count=reader.Word(mesh+0x14);const auto base=reader.Array(mesh+0x10,count,0x34);
    for(std::size_t i=0;i<count;++i) {
        const auto commands=reader.Commands(base+i*0x34+0x2c);const auto registers=Last(commands);
        const auto indices=registers.find(0x227),number=registers.find(0x228);
        if(indices==registers.end()||number==registers.end())throw std::runtime_error("BCH draw index registers missing");
        (void)Register(registers,0x227);const auto size=Register(registers,0x228);
        auto range=reader.Indices(indices->second->source_offset,size);
        if(const auto primitive=registers.find(0x25e);primitive!=registers.end()) {
            range.primitive_command=primitive->second->value;range.primitive_byte_mask=primitive->second->byte_mask;
        }
        output.ranges.push_back(std::move(range));
    }
}
void CullingRanges(const Reader& reader,std::size_t culling,BchMesh& output) {
    const auto count=reader.Word(culling+0x14);const auto base=reader.Array(culling+0x10,count,8);
    for(std::size_t i=0;i<count;++i)output.ranges.push_back(reader.Indices(base+i*8,reader.Word(base+i*8+4)));
}
}
bool ReadBchGeometry(const BchFile& file,std::vector<BchModelGeometry>& output,std::string& error) {
    std::vector<BchModelDescriptor> descriptors;if(!ReadBchModels(file,descriptors,error))return false;
    std::vector<BchModelGeometry> next;Reader reader(file);
    try {
        for(const auto& descriptor:descriptors) {
            BchModelGeometry model;model.model=descriptor;const auto at=descriptor.offset;
            model.material_count=reader.Word(at+0x38);
            const auto count=reader.Word(at+0x44),culling_count=reader.Word(at+0x6c);
            const auto meshes=reader.Array(at+0x40,count,0x38),cullings=reader.Array(at+0x68,culling_count,0x1c);
            if(culling_count&&culling_count!=count)throw std::runtime_error("BCH culling table does not match mesh table");
            for(unsigned i=0;i<count;++i) {
                const auto mesh_at=meshes+std::size_t(i)*0x38;
                BchMesh mesh;mesh.index=i;mesh.material_index=reader.Word(mesh_at)&0xffffu;mesh.culling_ranges=culling_count!=0;
                if(mesh.material_index>=model.material_count)throw std::runtime_error("BCH mesh material index outside model");
                if(mesh.culling_ranges)CullingRanges(reader,cullings+std::size_t(i)*0x1c,mesh);
                else StandardRanges(reader,mesh_at,mesh);
                for(const auto& range:mesh.ranges)for(auto index:range.indices)mesh.vertex_count=std::max(mesh.vertex_count,unsigned(index)+1);
                const auto commands=reader.Commands(mesh_at+8);const auto registers=Last(commands);
                ReadLayout(reader,registers,mesh);mesh.quantizer=ReadQuantizer(commands);
                if(mesh.vertex_count&&mesh.buffers.empty())throw std::runtime_error("BCH indexed mesh has no vertex buffers");
                model.meshes.push_back(std::move(mesh));
            }
            next.push_back(std::move(model));
        }
    }catch(const std::runtime_error& failure){error=failure.what();return false;}
    output=std::move(next);error.clear();return true;
}
}
