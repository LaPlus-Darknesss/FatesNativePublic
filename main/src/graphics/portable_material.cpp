#include "fates/graphics/portable_material.hpp"
#include <stdexcept>

namespace fates::graphics::portable {
MaterialRegisters ComposeMaterialRegisters(std::span<const BchCommandWrite> commands) {
    MaterialRegisters registers;
    for(const auto& command:commands) {
        std::uint32_t bits=0;
        for(unsigned i=0;i<4;++i)if(command.byte_mask&(1u<<i))bits|=0xffu<<(8*i);
        auto& state=registers[command.register_index];
        state.value=(state.value&~bits)|(command.value&bits);state.known_bits|=bits;
    }
    return registers;
}
bool ReadBchMaterials(const BchFile& file,std::vector<BchMaterial>& output,std::string& error) {
    std::vector<std::size_t> models;if(!file.Category(0,models,error))return false;
    const auto bytes=file.Bytes();std::vector<BchMaterial> next;
    auto need=[&](std::size_t at,std::size_t size) {
        if(at>bytes.size()||size>bytes.size()-at)throw std::runtime_error("BCH material span outside file");
    };
    auto word=[&](std::size_t at) {
        need(at,4);return std::uint32_t(bytes[at])|(std::uint32_t(bytes[at+1])<<8)|
            (std::uint32_t(bytes[at+2])<<16)|(std::uint32_t(bytes[at+3])<<24);
    };
    auto pointer=[&](std::size_t at,unsigned type) {
        need(at,4);const auto value=file.Resolve(at);
        if(!value||value->target_type!=type||!value->offset)throw std::runtime_error("BCH material pointer unresolved");
        need(*value->offset,0);return *value->offset;
    };
    auto commands=[&](std::size_t field,std::vector<BchCommandWrite>& result) {
        const auto count=word(field+4);if(!count)return;
        const auto at=pointer(field,2);std::string problem;
        if(!file.Commands(at,count,result,problem))throw std::runtime_error(problem);
    };
    auto string=[&](std::size_t field,std::optional<std::string>& result) {
        std::string problem;if(!file.ReadString(field,result,problem))throw std::runtime_error(problem);
    };
    try {
        for(unsigned model_index=0;model_index<models.size();++model_index) {
            const auto model=models[model_index];const auto count=word(model+0x38);if(!count)continue;
            if(count>bytes.size()/0x2c)throw std::runtime_error("BCH material count outside file");
            const auto base=pointer(model+0x34,0);need(base,std::size_t(count)*0x2c);
            for(unsigned index=0;index<count;++index) {
                const auto at=base+std::size_t(index)*0x2c;
                BchMaterial material;material.model_index=model_index;material.material_index=index;
                string(at+0x28,material.name);
                for(unsigned slot=0;slot<3;++slot)string(at+0x1c+4*slot,material.textures[slot]);
                material.parameters_offset=pointer(at,0);need(material.parameters_offset,0xd0);
                if(file.Resolve(at+0x18))material.mapper_offset=pointer(at+0x18,0);
                else if(word(at+0x18))throw std::runtime_error("Unrelocated BCH material mapper");
                commands(at+0x10,material.texture_commands);commands(material.parameters_offset+0xc8,material.fragment_commands);
                next.push_back(std::move(material));
            }
        }
    } catch(const std::runtime_error& failure){error=failure.what();return false;}
    output=std::move(next);error.clear();return true;
}
}
