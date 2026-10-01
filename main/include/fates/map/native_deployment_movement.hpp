#pragma once
#include "fates/map/native_deployment_fill.hpp"
#include <functional>
namespace fates::map::native {
struct DeploymentMoveTile {
    std::uint8_t change_id_1{},change_id_2{},cost_index{};
    std::uint32_t flags{};
};
using DeploymentTileLookup=std::function<std::optional<DeploymentMoveTile>(std::uint8_t)>;
// The projected-terrain path: flag400 follows change2/flag1, THEN flag200
// follows change1/flag2 on the resulting tile. Zero change IDs mean no change.
// Ordinary movement instead uses the owned image's stored cost-index plane.
std::optional<std::uint8_t> ResolveDeploymentMoveCostIndexExact(
    std::uint8_t terrain_index,std::uint32_t flags,const DeploymentTileLookup&);
struct DeploymentMoveOccupantServices {
    virtual ~DeploymentMoveOccupantServices()=default;
    virtual std::optional<bool> AlliedToActingForce()=0;
    virtual std::optional<bool> SameAsActiveForce()=0;
    virtual std::optional<bool> PublicFlag1()=0;
    virtual std::optional<bool> PolicyFlag400000()=0;
};
// Called for an occupied cell. Preserves original lazy flag/force/state gates.
std::optional<bool> DeploymentMoveOccupantAllowedExact(std::uint32_t flags,DeploymentMoveOccupantServices&);
struct DeploymentRouteSnapshot {
    std::array<std::uint8_t,128> steps{};
    std::uint8_t count{};
    std::int8_t start_x{},start_y{};
};
// GetCross walks count positions BEFORE each step, with signed-byte origin.
// Bit80 is not a terminator here. Count>128 exceeds native backing and refuses.
std::optional<int> DeploymentRouteCrossExact(const DeploymentRouteSnapshot&,int x,int y) noexcept;
enum class DeploymentMovementStatus : std::uint8_t {
    Ok,InvalidGeometry,InvalidOrigin,MissingCostIndex,MissingTerrainCost,
    MissingOccupancy,MissingRoute,FrontierOverflow,InvalidUnit,MissingDefinition,
    MissingPhase,MissingUnitState,TerrainImageUnavailable,UnitImageUnavailable
};
struct DeploymentMovementServices {
    virtual ~DeploymentMovementServices()=default;
    virtual std::optional<std::uint8_t> CostIndexAt(int x,int y,std::uint32_t flags)=0;
    virtual std::optional<std::int8_t> TerrainCost(std::uint8_t index,bool fallback)=0;
    virtual std::optional<bool> OccupantAllows(int x,int y,std::uint32_t flags)=0;
    virtual std::optional<int> RouteCross(int x,int y)=0;
};
// Original Move/SearchDir: two alternating frontiers, up/down/left/right with
// immediate reverse omitted, unsigned byte improvement/budget but SIGNED byte
// expansion. Costs128..254 can be stored but cannot expand;255 stays unreachable.
// Power narrows to its low byte. Origin need only lie in the32x32 backing domain.
// Each original1024-byte frontier holds255 nodes plus terminator; overflow and
// unresolved native inputs refuse without publishing any output byte.
DeploymentMovementStatus BuildDeploymentMovementExact(DeploymentMovementPlane&,
    TerrainImageGeometry,int origin_x,int origin_y,int movement_power,std::uint32_t flags,
    DeploymentMovementServices&);
struct CurrentDeploymentMovementResult {
    DeploymentMovementStatus status{DeploymentMovementStatus::InvalidUnit};
    TerrainImageStatus terrain_status{TerrainImageStatus::UninitializedImage};
    UnitImageStatus unit_status{UnitImageStatus::UnboundImage};
    std::optional<DeploymentMovementPlane> image;
};
// Mirrors generic Deploy::Move inputs. Uses current image/table data and current
// occupant/phase fields. A route snapshot is required only if flag20 is reached.
// No inferred route or UnitMoveXY private/skill/movement-power transformations.
CurrentDeploymentMovementResult BuildCurrentDeploymentMovement(
    const fates::runtime::native::NativeRuntime&,int x,int y,int cost_row,int power,
    std::uint32_t flags,std::uint8_t acting_force,const DeploymentRouteSnapshot* route=nullptr);
struct CurrentDeploymentFieldsResult {
    DeploymentMovementStatus movement_status{DeploymentMovementStatus::InvalidUnit};
    DeploymentFillStatus fill_status{DeploymentFillStatus::InvalidUnit};
    std::optional<DeploymentFields> fields;
};
// Composes current Job cost row and Unit force, full original movement and139
// range geometry. Power/flags and GetRangeBit masks remain explicitly resolved.
// No persistent Deploy, Danger, position publication or gameplay-state writes.
CurrentDeploymentFieldsResult BuildCurrentDeploymentFields(
    const fates::runtime::native::NativeRuntime&,std::uint16_t unit_slot,int x,int y,
    int resolved_movement_power,std::uint32_t resolved_flags,const DeploymentRangeMasks&,
    const DeploymentRouteSnapshot* route=nullptr);
}
