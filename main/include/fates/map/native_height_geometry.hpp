#pragma once
#include "fates/map/native_height_state.hpp"
#include <array>
#include <optional>
#include <limits>
namespace fates::runtime::native {struct NativeRuntime;struct NativeGameState;}
namespace fates::map::native {
struct HeightVector {float x{},y{},z{};bool operator==(const HeightVector&) const=default;};
struct HeightData {
    std::int8_t x{},y{};
    std::uint8_t layer{},diagonal{2};
    std::array<std::int16_t,4> heights{};
    bool operator==(const HeightData&) const=default;
};
struct HeightMapRange {int min_x{32},min_y{32},max_x{},max_y{};};
struct HeightPolygon {
    std::array<std::array<std::int16_t,3>,3> vertices{};
    std::uint16_t attribute{};
};
struct HeightHit {
    std::uint16_t type{},attribute{};
    HeightVector point{},normal{0.0f,1.0f,0.0f};
    std::array<HeightVector,3> triangle{};
};
struct HeightMapGeometry {
    HeightMapGeometry();
    HeightMapRange range;
    std::array<float,6> bounds{
        std::numeric_limits<float>::max(),std::numeric_limits<float>::max(),std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max(),-std::numeric_limits<float>::max(),-std::numeric_limits<float>::max()};
    HeightData fallback;
    // ((y*32+x)*2+layer), independent from tactical Terrain/Unit images.
    std::array<HeightData,2048> cells{};
};
enum class HeightStatus {Ok,NoPolygon,InvalidValue,InvalidGeometry,MissingGeometry,StaleGeometry};
struct HeightPolygonResult {HeightStatus status{HeightStatus::NoPolygon};HeightPolygon polygon;};
struct HeightHitResult {HeightStatus status{HeightStatus::Ok};HeightHit hit;};
struct MapHeightResult {HeightStatus status{HeightStatus::Ok};float height{};};
void ClearHeightDataExact(HeightData&) noexcept;
void CommitHeightDataExact(HeightData& destination,const HeightData& incoming) noexcept;
HeightVector HeightDataPositionExact(const HeightData&,unsigned vertex) noexcept; // vertex 0..3
bool SetHeightPolygonPositionExact(HeightPolygon&,unsigned vertex,HeightVector) noexcept;
HeightVector HeightPolygonPositionExact(const HeightPolygon&,unsigned vertex) noexcept; // vertex 0..2
// Arithmetic primitives require finite inputs and finite intermediate results.
// The quantized height-map triangle domain satisfies this requirement.
bool SafeNormalizeHeightVectorExact(HeightVector* output,HeightVector input) noexcept;
bool IntersectHeightSegmentExact(HeightVector a,HeightVector b,const std::array<HeightVector,3>& triangle,HeightVector& output) noexcept;
HeightVector HeightPolygonNormalExact(const HeightPolygon&) noexcept;
bool ValidHeightGeometry(const HeightMapGeometry&) noexcept;
void ClearHeightGeometryExact(HeightMapGeometry&) noexcept;
// Bounded record merge, not FieldWorld::HeightMap::Update. Caller owns range,
// object order, selection and local-to-world transforms. Coordinates are 0..31.
bool MergeWorldHeightRecordExact(HeightMapGeometry&,const HeightData&) noexcept;
bool CommitUpperHeightExact(HeightMapGeometry&,int x,int y,float offset) noexcept;
HeightPolygonResult QueryHeightPolygonExact(const HeightMapGeometry&,HeightVector world_query,bool layer) noexcept;
HeightHitResult CalculateHeightHitExact(const HeightMapGeometry&,HeightVector world_query,bool layer) noexcept;
MapHeightResult QueryMapHeightExact(const HeightMapGeometry&,HeightVector map_query,bool layer) noexcept;
struct CurrentHeightGeometryView {HeightStatus status{HeightStatus::MissingGeometry};const HeightMapGeometry* geometry{};};
HeightStatus RestoreCurrentHeightGeometry(fates::runtime::native::NativeRuntime&,const HeightMapGeometry&);
void InvalidateCurrentHeightGeometry(fates::runtime::native::NativeGameState&) noexcept;
CurrentHeightGeometryView ReadCurrentHeightGeometry(const fates::runtime::native::NativeRuntime&) noexcept;
MapHeightResult QueryCurrentMapHeight(const fates::runtime::native::NativeRuntime&,HeightVector,bool layer) noexcept;
}
