#pragma once
#include <array>
#include <cstddef>
#include "fates/graphics/scene_node.hpp"
class RenderState; class UnitActor;
namespace map {
class UnitRender : public SceneNode {
public:
    struct Data {
        Data();
        // PROVEN size: OnDrawXlu indexes queued records with a 0x10-byte stride.
        // Exact member labels remain held until UnitActor/ObjectManager ownership is source-owned.
        std::array<std::byte,0x10> retailRecord{};
    };
    static_assert(sizeof(Data)==0x10);

    UnitRender();
    void Entry(const UnitActor* actor);
    void Reset(float iconScale);
    void OnUpdate(RenderState& state);
    void OnDrawOpa(RenderState& state);
    void OnDrawXlu(RenderState& state);
};

bool UnitSortCompare(UnitRender::Data* lhs, UnitRender::Data* rhs, void* context);
}
