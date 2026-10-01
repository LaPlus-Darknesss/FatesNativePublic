#pragma once
#include "fates/map/native_height_archive.hpp"
namespace fates::map::native {
bool IsIdentityFieldPoseExact(const FieldPose&) noexcept;
bool RoundFieldPoseExact(FieldPose&) noexcept; // truncation to 1/8192
bool MakeFieldPoseMatrixExact(const FieldPose&,FieldMatrix&) noexcept;
FieldMatrix MultiplyFieldMatrixExact(const FieldMatrix&,const FieldMatrix&) noexcept;
HeightVector TransformFieldPointExact(const FieldMatrix&,HeightVector) noexcept;
bool TransformFieldBoundsExact(const FieldMatrix&,const std::array<float,6>&,std::array<float,6>&) noexcept;
// Finite vector dot product, original clamp and acos polynomial (no normalization).
bool FieldVectorThetaExact(HeightVector,HeightVector,float&) noexcept;
// Atomic on invalid/non-convertible arithmetic, including in-place use. Original
// low-byte coordinates, low-halfword heights, corner/diagonal changes are retained.
bool TransformFieldHeightListExact(const FieldHeightList&,const FieldMatrix&,FieldHeightList&);
// HeightList::Copy iterates the existing destination count. A longer source is
// truncated; a shorter source is refused before mutation. No capacity invention.
bool CopyFieldHeightListExact(const FieldHeightList&,FieldHeightList&);
bool CopyFieldPolygonListExact(const FieldPolygonList&,FieldPolygonList&);
bool TransformFieldPolygonListExact(const FieldPolygonList&,const FieldMatrix&,FieldPolygonList&);
// Original Round accumulates bounds after EACH vertex mutation, including the
// other two vertices before their own rounding. Step is 10 in UpdateDispos.
bool RoundFieldPolygonListExact(const FieldPolygonList&,float step,FieldPolygonList&);
}
