#pragma once
#include "fates/graphics/portable_material.hpp"

namespace fates::graphics::portable {
// Numeric values retain GraphicsLab8/9's verified material vocabulary. A PC
// backend maps these semantic values to its API and rejects unsupported modes.
enum class TextureSource : std::uint8_t {
    Primary=0,FragmentPrimary=1,FragmentSecondary=2,Texture0=3,Texture1=4,
    Texture2=5,Texture3=6,PreviousBuffer=13,Constant=14,Previous=15
};
enum class CombinerMode : std::uint8_t {
    Replace,Modulate,Add,AddSigned,Interpolate,Subtract,Dot3RGB,Dot3RGBA,MultiplyAdd,AddMultiply
};
enum class ColorOperand : std::uint8_t {
    Color=0,OneMinusColor=1,Alpha=2,OneMinusAlpha=3,Red=4,OneMinusRed=5,
    Green=8,OneMinusGreen=9,Blue=12,OneMinusBlue=13
};
enum class AlphaOperand : std::uint8_t {Alpha,OneMinusAlpha,Red,OneMinusRed,Green,OneMinusGreen,Blue,OneMinusBlue};
struct TextureStage {
    std::array<TextureSource,3> color_sources,alpha_sources;
    std::array<ColorOperand,3> color_operands;
    std::array<AlphaOperand,3> alpha_operands;
    CombinerMode color_mode{},alpha_mode{};
    unsigned color_scale{},alpha_scale{};
    std::array<std::uint8_t,4> constant;
    bool update_color_buffer{},update_alpha_buffer{};
};
enum class TextureWrap : std::uint8_t {ClampToEdge,ClampToBorder,Repeat,Mirror};
enum class TextureKind : std::uint8_t {TwoDimensional,Cube,Shadow2D,Projection,ShadowCube,Disabled};
struct TextureSampler {
    TextureWrap wrap_s{},wrap_t{};
    TextureKind kind{};
    bool min_linear{},mag_linear{},mip_linear{};
    MaterialRegister lod,border; // retained, not silently supplied as zero
};
struct TextureUnit {
    bool enabled{};
    std::optional<TextureSampler> sampler;
};
enum class CompareFunction : std::uint8_t {Never,Always,Equal,NotEqual,Less,LessEqual,Greater,GreaterEqual};
enum class BlendEquation : std::uint8_t {Add,Subtract,ReverseSubtract,Minimum,Maximum};
enum class BlendFactor : std::uint8_t {
    Zero,One,SourceColor,OneMinusSourceColor,DestinationColor,OneMinusDestinationColor,
    SourceAlpha,OneMinusSourceAlpha,DestinationAlpha,OneMinusDestinationAlpha,
    ConstantColor,OneMinusConstantColor,ConstantAlpha,OneMinusConstantAlpha,SourceAlphaSaturate
};
enum class CullFace : std::uint8_t {None,Front,Back};
struct PortableMaterialState {
    std::array<TextureStage,6> stages;
    std::array<std::uint8_t,4> initial_buffer;
    std::array<TextureUnit,3> textures;
    CullFace cull{};
    BlendEquation color_equation{},alpha_equation{};
    BlendFactor color_source{},color_destination{},alpha_source{},alpha_destination{};
    bool alpha_test{},depth_test{},depth_write{};
    CompareFunction alpha_function{},depth_function{};
    std::uint8_t alpha_reference{},color_write_mask{};
};
// Atomic translation. Only explicitly written bits may become host fields.
// Full lighting, stencil, logic-op and texture-coordinate mapping remain in the
// source commands and require explicit admission by the selected draw backend.
bool TranslateMaterialState(const BchMaterial&,PortableMaterialState&,std::string& error);
}
