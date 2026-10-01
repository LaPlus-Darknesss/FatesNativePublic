#pragma once
#include "fates/map/native_field_configuration.hpp"
#include "fates/map/native_height_geometry.hpp"
#include <cstdint>

namespace fates::map::native {
// Semantic camera motion, separate from the renderer's view/projection matrices.
// A zero-initialized value is NOT evidence that a live map camera exists.
struct CameraScalarMotion {
    float destination{},origin{},target{},elapsed{};
};
struct CameraMotionState {
    HeightVector destination{},offset{},current{},limited{},origin{},target{};
    float elapsed{};
    CameraScalarMotion distance{},pitch{},yaw{};
    HeightVector position{},rotation_radians{};
    float view_distance{},stereo_depth{},stereo_intensity{};
    std::uint8_t mode{};
};
struct CameraPlayArea {std::uint32_t x{},y{},width{},height{};};
// Original FieldConfig numeric fields at80..B4. Null FieldConfig has actual
// executable defaults; an unavailable FieldWorld must not be passed as null.
struct CameraFieldParameters {
    std::array<float,3> distances{15.0f,22.0f,30.0f};
    std::array<float,3> near_limits{},far_limits{};
    std::array<float,3> angles{-60.0f,-65.0f,-75.0f};
    float stereo_depth{-3.0f},stereo_intensity{1.0f};
};
CameraFieldParameters ReadCameraFieldParameters(const FieldConfiguration&) noexcept;
// Finite, binary32 arithmetic domain. False refuses without changing output or
// state. Raw clock bits in the scroll predicate are compared as signed words,
// including negative zero and NaN clock encodings; they are not float compares.
bool CameraMoveRatioExact(std::int32_t frame,float& output) noexcept;
bool LimitCameraTargetExact(HeightVector,float distance,CameraPlayArea,
    const CameraFieldParameters&,HeightVector& output) noexcept;
bool CameraScrollProlixityExact(const CameraMotionState&,bool& output) noexcept;
bool UpdateCameraTargetExact(CameraMotionState&,CameraPlayArea,const CameraFieldParameters&) noexcept;
bool TickCameraTargetExact(CameraMotionState&,std::uint32_t frame_delta,
    CameraPlayArea,const CameraFieldParameters&) noexcept;
bool TickCameraDistanceExact(CameraMotionState&,std::uint32_t frame_delta,const CameraFieldParameters&) noexcept;
bool UpdateCameraDistanceExact(CameraMotionState&) noexcept;
bool UpdateCameraAngleExact(CameraMotionState&) noexcept;
bool TickCameraAngleExact(CameraMotionState&,std::uint32_t frame_delta) noexcept;
// CameraBase::Tick for a proven zero blend-duration state. Modes2/3 skip the
// callbacks; every successful tick clears mode. Active blend needs view/param
// ownership and is not silently reduced to this branch.
bool TickUnblendedCameraExact(CameraMotionState&,std::uint32_t frame_delta,
    CameraPlayArea,const CameraFieldParameters&) noexcept;
// Height is read from the shared HeightMap owner by the caller. This operation
// includes the complete Instant writes, including angles, distance and stereo.
bool InstantCameraExact(CameraMotionState&,float map_height,CameraPlayArea,
    const CameraFieldParameters&) noexcept;
bool SetCameraDestinationIntExact(CameraMotionState&,std::int32_t x,std::int32_t y,
    float map_height,CameraPlayArea,const CameraFieldParameters&) noexcept;
bool SetCameraDestinationFloatExact(CameraMotionState&,float x,float y,float map_height,
    CameraPlayArea,const CameraFieldParameters&) noexcept;
}
