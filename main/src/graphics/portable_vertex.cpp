#include "fates/graphics/portable_vertex.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>

namespace fates::graphics::portable {
namespace {
struct AttributeBinding {const VertexBuffer* buffer{};const VertexAttribute* attribute{};};
unsigned ScalarBytes(VertexScalar scalar) {
    return scalar==VertexScalar::Float32?4u:scalar==VertexScalar::SignedShort?2u:1u;
}
double Scalar(std::span<const std::uint8_t> bytes,std::size_t at,VertexScalar type) {
    if(type==VertexScalar::SignedByte)return std::bit_cast<std::int8_t>(bytes[at]);
    if(type==VertexScalar::UnsignedByte)return bytes[at];
    const auto half=static_cast<std::uint16_t>(bytes[at]|(unsigned(bytes[at+1])<<8));
    if(type==VertexScalar::SignedShort)return std::bit_cast<std::int16_t>(half);
    const auto word=std::uint32_t(half)|(std::uint32_t(bytes[at+2])<<16)|(std::uint32_t(bytes[at+3])<<24);
    return std::bit_cast<float>(word);
}
bool Fail(std::string& error,const char* text){error=text;return false;}
}
bool DecodeStaticFieldVertices(const BchFile& file,const BchMesh& mesh,StaticFieldVertices& output,std::string& error) {
    const auto bytes=file.Bytes();std::array<AttributeBinding,16> bindings{};StaticFieldVertices next;
    for(const auto& buffer:mesh.buffers) {
        if(!buffer.stride||buffer.data_offset>bytes.size()||mesh.vertex_count>(bytes.size()-buffer.data_offset)/buffer.stride)
            return Fail(error,"Static vertex buffer outside file");
        for(const auto& attribute:buffer.attributes) {
            const auto scalar=static_cast<unsigned>(attribute.scalar);
            if(scalar>3||attribute.semantic>=bindings.size()||attribute.elements<1||attribute.elements>4)
                return Fail(error,"Invalid static vertex attribute");
            const auto width=ScalarBytes(attribute.scalar);
            if(attribute.byte_offset>buffer.stride||attribute.elements*width>buffer.stride-attribute.byte_offset)
                return Fail(error,"Static vertex attribute exceeds stride");
            if(bindings[attribute.semantic].attribute)return Fail(error,"Duplicate static vertex semantic");
            bindings[attribute.semantic]={&buffer,&attribute};next.supplied_semantics|=static_cast<std::uint16_t>(1u<<attribute.semantic);
        }
    }
    if(!bindings[0].attribute||bindings[0].attribute->elements<3)return Fail(error,"Static field position is missing");
    if(bindings[4].attribute&&bindings[4].attribute->elements<2)return Fail(error,"Static field texcoord is incomplete");
    for(const unsigned semantic:{0u,1u,3u,4u})if(bindings[semantic].attribute&&bindings[semantic].attribute->scalar!=VertexScalar::Float32&&!mesh.quantizer)
        return Fail(error,"Static field quantizer is unresolved");
    next.vertices.reserve(mesh.vertex_count);
    next.minimum.fill(std::numeric_limits<float>::infinity());next.maximum.fill(-std::numeric_limits<float>::infinity());
    for(unsigned index=0;index<mesh.vertex_count;++index) {
        auto values=[&](unsigned semantic,std::array<double,4> defaults) {
            const auto& binding=bindings[semantic];if(!binding.attribute)return defaults;
            const auto& attribute=*binding.attribute;
            const auto start=binding.buffer->data_offset+std::size_t(index)*binding.buffer->stride+attribute.byte_offset;
            for(unsigned c=0;c<attribute.elements;++c)defaults[c]=Scalar(bytes,start+c*ScalarBytes(attribute.scalar),attribute.scalar);
            return defaults;
        };
        auto position=values(0,{0,0,0,1}),normal=values(1,{0,1,0,0}),uv=values(4,{0,0,0,0}),color=values(3,{1,1,1,1});
        if(bindings[1].attribute&&bindings[1].attribute->elements<3)
            for(unsigned i=bindings[1].attribute->elements;i<3;++i)normal[i]=0;
        const auto integer=[&](unsigned semantic){return bindings[semantic].attribute&&bindings[semantic].attribute->scalar!=VertexScalar::Float32;};
        if(integer(0))for(unsigned i=0;i<3;++i)position[i]=position[i]*mesh.quantizer->position_scale+mesh.quantizer->position_offset[i];
        if(integer(1))for(unsigned i=0;i<std::min(3u,bindings[1].attribute->elements);++i)normal[i]*=mesh.quantizer->normal_scale;
        if(integer(4))for(unsigned i=0;i<2;++i)uv[i]*=mesh.quantizer->texcoord_scale[0];
        if(integer(3))for(unsigned i=0;i<bindings[3].attribute->elements;++i)color[i]*=mesh.quantizer->color_scale;
        const double length=std::sqrt(normal[0]*normal[0]+normal[1]*normal[1]+normal[2]*normal[2]);
        if(length>1e-8)for(unsigned i=0;i<3;++i)normal[i]/=length;else normal={0,1,0,0};
        StaticFieldVertex vertex;
        for(unsigned i=0;i<3;++i){vertex.position[i]=static_cast<float>(position[i]);vertex.normal[i]=static_cast<float>(normal[i]);}
        for(unsigned i=0;i<2;++i)vertex.texcoord[i]=static_cast<float>(uv[i]);
        for(unsigned i=0;i<4;++i)vertex.color[i]=static_cast<float>(color[i]);
        for(const auto* row:{&position,&normal,&uv,&color})for(double value:*row)if(!std::isfinite(value))return Fail(error,"Nonfinite static vertex input");
        for(float value:vertex.position)if(!std::isfinite(value))return Fail(error,"Static vertex float overflow");
        for(float value:vertex.color)if(!std::isfinite(value))return Fail(error,"Static color float overflow");
        for(float value:vertex.texcoord)if(!std::isfinite(value))return Fail(error,"Static texcoord float overflow");
        for(unsigned i=0;i<3;++i){next.minimum[i]=std::min(next.minimum[i],vertex.position[i]);next.maximum[i]=std::max(next.maximum[i],vertex.position[i]);}
        next.vertices.push_back(vertex);
    }
    if(next.vertices.empty()){next.minimum={};next.maximum={};}
    output=std::move(next);error.clear();return true;
}
}
