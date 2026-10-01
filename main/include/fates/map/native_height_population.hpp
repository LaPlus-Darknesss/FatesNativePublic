#pragma once
#include "fates/map/native_height_archive.hpp"
namespace fates::map::native {
enum class HeightPopulationStatus {Ok,InvalidState,InvalidBounds,InvalidRecord,MissingGeometry,StaleGeometry};
struct FieldHeightSelection {
    HeightPopulationStatus status{HeightPopulationStatus::Ok};
    std::shared_ptr<const FieldHeightList> list;
};
// Height selection inside FieldObject::UpdateDispos. The result is still local
// archive data; selecting it does not establish a world transform or instance.
FieldHeightSelection SelectFieldHeightSourceExact(const FieldHeightPart*,int state,std::uint16_t flags);
struct WorldHeightContribution {
    std::uint16_t flags{};
    const FieldHeightList* world_list{}; // explicitly prepared world-space records
};
void ClampHeightRangeExact(HeightMapRange&) noexcept;
void AddHeightRangeExact(HeightMapRange&,const HeightMapRange&) noexcept;
bool AddHeightBoundsRangeExact(HeightMapRange&,const std::array<float,6>&) noexcept;
// HeightMap::Entry and Remove both queue the clamped list bounds into the
// world's pending range. Neither directly edits cells. Null lists do nothing.
bool QueueHeightListRangeExact(HeightMapRange& pending,const FieldHeightList*) noexcept;
// Original ordered HeightMap::Update, including clamped clearing, requested
// (unclamped) record membership, strict-mean merge and accumulating bounds.
// Unsafe addressed records refuse atomically; records outside the request are
// not addressed. Object construction, transforms and lifetime remain external.
HeightPopulationStatus UpdateHeightPopulationExact(HeightMapGeometry&,HeightMapRange requested,std::span<const WorldHeightContribution>);
HeightPopulationStatus RebuildCurrentHeightPopulation(fates::runtime::native::NativeRuntime&,std::span<const WorldHeightContribution>);
HeightPopulationStatus UpdateCurrentHeightPopulation(fates::runtime::native::NativeRuntime&,HeightMapRange,std::span<const WorldHeightContribution>);
}
