#pragma once
#include "fates/map/native_movement_field.hpp"
#include <array>
#include <cstdint>
namespace fates::runtime::native {struct NativeRuntime;}
namespace fates::map::native {
// Explicit resolved map image, not a reconstruction from living Unit positions.
// Zero is empty; nonzero occupant keys identify Units, including the queried
// Unit's self key. Callers own current terrain/occupancy image provenance.
struct RescueTerrainImage {
    MovementBounds active;
    std::array<std::uint8_t,1024> terrain_cost_indices{};
    std::array<bool,1024> terrain_blocks_move_image{};
};
struct RescueMapImage : RescueTerrainImage {
    std::array<std::uint16_t,1024> occupants{};
};
struct RescueRequest {
    int origin_x{},origin_y{};
    std::int8_t unit_x{},unit_y{};
    std::uint16_t unit_key{};
    bool allow_origin{},prefer_unit_origin{};
};
enum class RescuePositionStatus : std::uint8_t {Ok,InvalidInput,MissingCurrentRules,MissingTerrainCost};
enum class RescuePositionRoute : std::uint8_t {NormalField,PreferredField,TerrainFallback,UncheckedOrigin};
struct RescuePositionResult {
    RescuePositionStatus status{RescuePositionStatus::InvalidInput};
    int x{-1},y{-1};
    RescuePositionRoute route{RescuePositionRoute::UncheckedOrigin};
};
using RescueField=std::array<std::int8_t,1024>;
using RescueTerrainCosts=std::array<std::int8_t,1024>;
struct RescueMovementFields {RescueField primary,secondary;};
std::optional<RescueMovementFields> BuildRescueMovementFields(MovementBounds,
    int origin_x,int origin_y,const RescueTerrainCosts&,bool movement_prohibited,bool cost_free);
// Exact ordered selection after the two UnitMoveXY fields. Terrain flag8 is
// applied when reading fields, not during propagation or terrain fallback.
// Exhaustion returns the requested origin unconditionally; this is not proof
// that a Unit can safely be published at the returned position.
RescuePositionResult SelectRescuePositionExact(const RescueMapImage&,
    const RescueTerrainCosts&,const RescueMovementFields&,const RescueRequest&);
// Full query: current Unit/Person/Job/inventory/skill rules plus caller-owned
// resolved map image. No occupancy inference, Unit mutation or action admission.
// Occupant identity convention for this adapter is native slot+1, zero empty.
RescuePositionResult QueryRescuePositionForCurrentUnit(const fates::runtime::native::NativeRuntime&,
    std::uint16_t unit_slot,const RescueMapImage&,int origin_x,int origin_y,
    bool allow_origin,bool prefer_unit_origin);
}
