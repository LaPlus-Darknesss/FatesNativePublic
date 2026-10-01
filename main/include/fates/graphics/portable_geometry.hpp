#pragma once
#include "fates/graphics/portable_bch.hpp"

namespace fates::graphics::portable {
enum class VertexScalar : std::uint8_t { SignedByte,UnsignedByte,SignedShort,Float32 };
struct VertexAttribute {
    unsigned attribute{},semantic{},elements{},byte_offset{};
    VertexScalar scalar{};
};
struct VertexBuffer {
    unsigned index{},stride{};
    std::size_t data_offset{};
    std::vector<VertexAttribute> attributes;
};
struct MeshIndexRange {
    // Lab4 repair: serialized relocation type, not pre-relocation bit31,
    // determines U8 versus U16. The native draw representation is always U16.
    unsigned source_element_bytes{};
    // Last raw topology write and byte mask are retained together. Partial
    // writes are not expanded into an invented initial register value.
    std::optional<std::uint32_t> primitive_command;
    std::uint8_t primitive_byte_mask{};
    std::vector<std::uint16_t> indices;
};
struct MeshQuantizer {
    std::array<float,3> position_offset{};
    float position_scale{},normal_scale{},tangent_scale{},color_scale{};
    std::array<float,3> texcoord_scale{};
};
struct BchMesh {
    unsigned index{},material_index{},vertex_count{},fixed_attribute_mask{};
    bool culling_ranges{};
    std::vector<VertexBuffer> buffers;
    std::vector<MeshIndexRange> ranges;
    std::optional<MeshQuantizer> quantizer;
};
struct BchModelGeometry {
    BchModelDescriptor model;
    unsigned material_count{};
    std::vector<BchMesh> meshes;
};
// Renderer-neutral serialized geometry. This preserves culling ranges rather
// than claiming the retail visibility traversal or skeletal shader is owned.
// Decode is atomic; offsets stay bounded file offsets in the retained BchFile.
bool ReadBchGeometry(const BchFile&,std::vector<BchModelGeometry>&,std::string& error);
}
