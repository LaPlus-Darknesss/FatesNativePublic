#pragma once
#include "fates/map/native_height_geometry.hpp"
#include "fates/map/native_field_pose.hpp"
#include <span>
#include <string>
#include <optional>
#include <vector>
namespace fates::map::native {
struct FieldHeightList {
    std::uint32_t archive_offset{}; // original payload-relative list identity
    std::array<float,6> bounds{};
    std::vector<HeightData> records;
};
struct FieldPolygonList {
    std::uint32_t archive_offset{};
    std::array<float,6> bounds{};
    std::vector<HeightPolygon> records;
};
// UpdateEffect consumes the same 44-byte name/label/MapPose record shape as
// ReferList. The two strings remain nullable original bytes, not host paths.
struct FieldEffectPlacement {
    std::optional<std::string> name,label;
    FieldPose pose;
};
struct FieldEffectPlacementList {
    std::uint32_t archive_offset{};
    std::vector<FieldEffectPlacement> records;
};
struct FieldHeightPart {
    std::string name; // original CP932 bytes, no host path interpretation
    std::optional<std::string> bch_container; // +0x04; absent is distinct from empty
    std::uint8_t type{}; // +0x08; raw FieldObject::Load selector, no invented enum
    std::array<float,6> bounds{}; // +0x0C; max < min means empty
    std::array<std::int8_t,4> model_indices{}; // +0x24; -1 means no model
    // FieldObject::UpdateDispos indexes each block by its signed state byte.
    // Paragon calls the second block "battle height"; the executable instead
    // passes it to ColsTree/PolyList, whose records are 20 bytes, not 12.
    // Aliases share storage within the height or polygon record family.
    std::array<std::shared_ptr<const FieldHeightList>,3> lists; // +0xB8
    std::array<std::shared_ptr<const FieldPolygonList>,3> geometry; // +0xC4
    std::array<std::shared_ptr<const FieldPolygonList>,3> collision; // +0xD0
    // Four 36-byte component records at +0x28. Paragon preserves the bytes;
    // UpdateDispos passes the selected record to MapPose::MakeSRT.
    std::array<FieldPose,4> level_poses;
    std::array<std::shared_ptr<const FieldEffectPlacementList>,3> effects; // +0xDC
};
struct FieldHeightArchive {std::vector<FieldHeightPart> parts;};
// Unrelocated, decompressed Field archive, MapHeader version 0x20140623.
// Atomic output replacement; unsupported versions or malformed
// pointers/records fail. This does not choose a lane or apply FieldRefer transforms.
bool DecodeFieldHeightArchive(std::span<const std::uint8_t>,FieldHeightArchive& output);
}
