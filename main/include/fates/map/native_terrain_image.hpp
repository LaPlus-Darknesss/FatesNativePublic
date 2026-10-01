#pragma once
#include "fates/map/native_terrain_image_types.hpp"
#include "fates/map/native_unit_image.hpp"
namespace fates::runtime::native {struct TerrainMapDefinition;}
namespace fates::map::native {
enum class TerrainImageStatus : std::uint8_t {
    Ok,InvalidDimensions,MissingTerrainDefinition,MissingTileDefinition,
    UninitializedImage,UnboundImage,StaleImage,InvalidActiveBounds
};
// Original Image::Update: narrow six metadata words to bytes, copy the selected
// source into both terrain planes, then project only the GLOBAL Image full area.
// Null or receiver-alias global means the singleton route. A distinct global is
// read-only and supplies dimensions/terrain IDs, exactly as in the executable.
// Padding costs survive. Inputs outside the safe 32x32 domain refuse atomically.
TerrainImageStatus RefreshTerrainImageExact(TerrainImageGrid& receiver,
    const fates::runtime::native::TerrainMapDefinition&,
    const CastleTerrainPlane* castle=nullptr,const TerrainImageGrid* global=nullptr);
// Narrow explicit plane initialization, not the complete map::Image constructor.
// Initializes zero terrain/cost planes and 32x32 / [1,1,31,31] shared geometry;
// occupancy cells and their existing validity binding are preserved.
void InitializeCurrentTerrainImage(fates::runtime::native::NativeGameState&) noexcept;
void InvalidateCurrentTerrainImage(fates::runtime::native::NativeGameState&) noexcept;
TerrainImageStatus RefreshCurrentTerrainImage(fates::runtime::native::NativeRuntime&,
    const CastleTerrainPlane* castle=nullptr);
struct TerrainImageView {
    TerrainImageStatus status{TerrainImageStatus::UninitializedImage};
    TerrainImageGeometry geometry{};
    const TerrainImagePlanes* planes{};
};
// Current-call view; checks map/chapter context, shared geometry changes and all
// relevant parsed source fields. Explicit Castle input is a carried snapshot:
// its generator/lifetime owner must refresh or invalidate when that world changes.
TerrainImageView ReadCurrentTerrainImage(const fates::runtime::native::NativeRuntime&) noexcept;
struct CurrentTacticalRescueResult {
    TerrainImageStatus terrain_status{TerrainImageStatus::UninitializedImage};
    UnitImageStatus unit_status{UnitImageStatus::UnboundImage};
    RescuePositionResult position{};
};
CurrentTacticalRescueResult QueryRescuePositionWithCurrentTacticalImage(
    const fates::runtime::native::NativeRuntime&,std::uint16_t slot,
    int origin_x,int origin_y,bool allow_origin,bool prefer_unit_origin);
}
