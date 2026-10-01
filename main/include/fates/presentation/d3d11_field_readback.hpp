#pragma once
#include "fates/presentation/live_field_scene.hpp"
#include "fates/graphics/portable_texture.hpp"
#include <memory>

namespace fates::presentation::portable {
// GraphicsLab19 positive-Z convention. All camera values must be supplied;
// this object does not select a tactical camera or infer one from scene bounds.
struct FieldReadbackCamera {
    field::FieldMatrix view;
    float focal_x{},focal_y{},near_z{},far_z{};
};
enum class FieldReadbackDevice {Warp,Hardware};
struct GeometryDiagnosticStyle {
    std::array<float,4> clear,color;
    bool wireframe{};
};
struct FieldReadbackStats {
    std::size_t actors{},meshes{},ranges{},indices{},retained_models{};
};
// Explicit static-geometry diagnostic consumer. The chosen presentation mode
// uses caller-supplied color and triangle lists, as in Lab19's geometry draw.
// It is NOT a retail material/lighting/skinning/culling result. This proves live
// field identity, transformation, retained upload and readback independently of
// those remaining presentation services. No gameplay state is written here.
class D3D11FieldReadback {
public:
    static std::unique_ptr<D3D11FieldReadback> Create(FieldReadbackDevice,std::string& error);
    ~D3D11FieldReadback();
    D3D11FieldReadback(const D3D11FieldReadback&)=delete;
    D3D11FieldReadback& operator=(const D3D11FieldReadback&)=delete;
    // Synchronous owner-thread use. Null frame draws an empty current scene.
    // On failure output is unchanged; do not present it as a successful frame.
    // Canonical native canvas must be 16:9. Legacy composition can consume this
    // canvas externally without changing the live game or field membership.
    bool Draw(std::shared_ptr<const FieldPresentationFrame>,const FieldReadbackCamera&,
        unsigned width,unsigned height,const GeometryDiagnosticStyle&,
        fates::graphics::portable::TextureImage& output,FieldReadbackStats&,std::string& error);
private:
    struct Impl;
    explicit D3D11FieldReadback(std::unique_ptr<Impl>);
    std::unique_ptr<Impl> impl_;
};
}
