#include "fates/graphics/portable_render_state.hpp"
#include <stdexcept>

namespace fates::graphics::portable {
namespace {
std::uint32_t Require(const MaterialRegisters& registers,unsigned reg,std::uint32_t bits) {
    const auto found=registers.find(reg);
    if(found==registers.end()||!found->second.Read(bits))throw std::runtime_error("Material register bits unresolved: "+std::to_string(reg));
    return *found->second.Read(bits);
}
MaterialRegister Retain(const MaterialRegisters& registers,unsigned reg) {
    const auto found=registers.find(reg);return found==registers.end()?MaterialRegister{}:found->second;
}
std::array<std::uint8_t,4> Color(std::uint32_t word) {
    return {static_cast<std::uint8_t>(word),static_cast<std::uint8_t>(word>>8),static_cast<std::uint8_t>(word>>16),static_cast<std::uint8_t>(word>>24)};
}
bool KnownSource(unsigned value){return value<=6||value>=13;}
bool KnownColorOperand(unsigned value){return value<=5||value==8||value==9||value==12||value==13;}
}
bool TranslateMaterialState(const BchMaterial& material,PortableMaterialState& output,std::string& error) {
    const auto texture=ComposeMaterialRegisters(material.texture_commands),fragment=ComposeMaterialRegisters(material.fragment_commands);
    PortableMaterialState next;
    try {
        constexpr std::array<unsigned,6> bases{0xc0,0xc8,0xd0,0xd8,0xf0,0xf8};
        const auto update=Require(fragment,0xe0,0xff00);
        next.initial_buffer=Color(Require(fragment,0xfd,0xffffffff));
        for(unsigned i=0;i<6;++i) {
            auto& stage=next.stages[i];const auto base=bases[i];
            const auto sources=Require(fragment,base,0x0fff0fff),operands=Require(fragment,base+1,0x00777fff);
            const auto combiner=Require(fragment,base+2,0x000f000f),scale=Require(fragment,base+4,0x00030003);
            for(unsigned j=0;j<3;++j) {
                const auto cs=(sources>>(4*j))&15u,as=(sources>>(16+4*j))&15u,co=(operands>>(4*j))&15u,ao=(operands>>(12+4*j))&7u;
                if(!KnownSource(cs)||!KnownSource(as)||!KnownColorOperand(co))throw std::runtime_error("Unsupported texture combiner enum");
                stage.color_sources[j]=static_cast<TextureSource>(cs);stage.alpha_sources[j]=static_cast<TextureSource>(as);
                stage.color_operands[j]=static_cast<ColorOperand>(co);stage.alpha_operands[j]=static_cast<AlphaOperand>(ao);
            }
            const auto cm=combiner&15u,am=(combiner>>16)&15u,sc=scale&3u,sa=(scale>>16)&3u;
            if(cm>9||am>9||sc>2||sa>2)throw std::runtime_error("Unsupported texture combiner mode or scale");
            stage.color_mode=static_cast<CombinerMode>(cm);stage.alpha_mode=static_cast<CombinerMode>(am);
            stage.color_scale=1u<<sc;stage.alpha_scale=1u<<sa;stage.constant=Color(Require(fragment,base+3,0xffffffff));
            stage.update_color_buffer=i>=1&&i<=4&&(update&(0x100u<<(i-1)))!=0;
            stage.update_alpha_buffer=i>=1&&i<=4&&(update&(0x1000u<<(i-1)))!=0;
        }
        const auto enabled=Require(texture,0x80,7);
        constexpr std::array<unsigned,3> parameters{0x83,0x93,0x9b},lods{0x84,0x94,0x9c},borders{0x81,0x91,0x99};
        for(unsigned i=0;i<3;++i) {
            auto& unit=next.textures[i];unit.enabled=(enabled&(1u<<i))!=0;
            if(!unit.enabled&&!texture.contains(parameters[i]))continue;
            const auto value=Require(texture,parameters[i],0x71007706u);const auto s=(value>>12)&7u,t=(value>>8)&7u,type=(value>>28)&7u;
            if(s>3||t>3||type>5)throw std::runtime_error("Unsupported texture sampler enum");
            unit.sampler=TextureSampler{static_cast<TextureWrap>(s),static_cast<TextureWrap>(t),static_cast<TextureKind>(type),(value&4)!=0,(value&2)!=0,(value&0x01000000)!=0,Retain(texture,lods[i]),Retain(texture,borders[i])};
        }
        const auto cull=Require(fragment,0x40,3);if(cull>2)throw std::runtime_error("Unsupported material cull enum");next.cull=static_cast<CullFace>(cull);
        const auto blend=Require(fragment,0x101,0xffff0707u);const auto ce=blend&7u,ae=(blend>>8)&7u;
        const auto cs=(blend>>16)&15u,cd=(blend>>20)&15u,as=(blend>>24)&15u,ad=(blend>>28)&15u;
        if(ce>4||ae>4||cs>14||cd>14||as>14||ad>14)throw std::runtime_error("Unsupported material blend enum");
        next.color_equation=static_cast<BlendEquation>(ce);next.alpha_equation=static_cast<BlendEquation>(ae);
        next.color_source=static_cast<BlendFactor>(cs);next.color_destination=static_cast<BlendFactor>(cd);
        next.alpha_source=static_cast<BlendFactor>(as);next.alpha_destination=static_cast<BlendFactor>(ad);
        const auto alpha=Require(fragment,0x104,0xff71),depth=Require(fragment,0x107,0x1f71);
        next.alpha_test=(alpha&1)!=0;next.alpha_function=static_cast<CompareFunction>((alpha>>4)&7u);next.alpha_reference=static_cast<std::uint8_t>(alpha>>8);
        next.depth_test=(depth&1)!=0;next.depth_function=static_cast<CompareFunction>((depth>>4)&7u);next.depth_write=(depth&0x1000)!=0;
        next.color_write_mask=static_cast<std::uint8_t>((depth>>8)&15u);
    }catch(const std::runtime_error& failure){error=failure.what();return false;}
    output=std::move(next);error.clear();return true;
}
}
