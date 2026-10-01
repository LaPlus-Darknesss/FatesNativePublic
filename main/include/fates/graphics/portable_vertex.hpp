#pragma once
#include "fates/graphics/portable_geometry.hpp"

namespace fates::graphics::portable {
// Lab19's initial static field input layout, kept in model-local coordinates.
// FieldObject world matrices already include the serialized model matrix.
struct StaticFieldVertex {
    std::array<float,3> position,normal;
    std::array<float,2> texcoord;
    std::array<float,4> color;
};
static_assert(sizeof(StaticFieldVertex)==48);
struct StaticFieldVertices {
    std::vector<StaticFieldVertex> vertices;
    std::uint16_t supplied_semantics{};
    std::array<float,3> minimum{},maximum{};
};
// Canonical numeric conversion from the verified Lab19 field decoder. This
// alone does not admit fixed attributes, skinning or all material UV mappings.
// Missing normal/UV/color defaults are explicit Lab19 input-layout defaults;
// supplied_semantics lets the renderer validate its actual shader requirements.
bool DecodeStaticFieldVertices(const BchFile&,const BchMesh&,StaticFieldVertices&,std::string& error);
}
