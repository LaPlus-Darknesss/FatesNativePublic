#pragma once
#include "fates/map/native_deployment_fields.hpp"
#include "fates/map/native_movement_field.hpp"
#include "fates/map/native_unit_image.hpp"
#include "fates/map/native_terrain_image.hpp"
#include <array>
#include <optional>
namespace fates::map::native {
// GetRangeBit produces combined masks and a copy of the primary Unit's masks
// before partner contributions. The latter apply where the partner cannot stand.
struct DeploymentRangeMasks {
    std::uint64_t attack{},rod{},without_partner_attack{},without_partner_rod{};
    int attack_max{},rod_max{};
};
enum class DeploymentFillMode : std::uint8_t {Attack,Rod,DoubleAttack,DoubleRod,Double,Unit};
enum class DeploymentFillStatus : std::uint8_t {
    Ok,InvalidBounds,InvalidRange,InvalidMode,MissingFillInput,MissingPartnerCost,
    InvalidUnit,MissingCurrentDefinition,MissingCurrentState,MalformedPair,
    TerrainImageUnavailable,UnitImageUnavailable
};
struct DeploymentFillServices {
    virtual ~DeploymentFillServices()=default;
    virtual std::optional<bool> IsFill(int x,int y)=0;
    // Called only for an admitted origin on a finite Double route.
    virtual std::optional<bool> PartnerCanStand(int x,int y)=0;
};
// Basic FillAttack/Rod use all64 mask bits. Finite Double variants use ONLY the
// low32 bits, as the original instructions do. Any Double sentinel max255 routes
// through basic fill; max255 sets the entire128-byte plane without origin queries.
// Basic/Double calls OR into existing planes. UnitFill clears BOTH planes first,
// then dispatches by nonzero maxima. Native failure publishes neither plane.
DeploymentFillStatus FillDeploymentRangesExact(DeploymentRangePlanes&,MovementBounds,
    DeploymentFillMode,const DeploymentRangeMasks&,bool has_partner,DeploymentFillServices&);
struct DeploymentFillOccupantServices {
    virtual ~DeploymentFillOccupantServices()=default;
    virtual std::optional<bool> MovementProhibited()=0;
    virtual std::optional<bool> State100()=0;
    virtual std::optional<bool> Allied()=0;
};
// Original IsFill short-circuit order. A zero movement value admits an occupied
// origin before any occupied/skill/alliance gate; terrain flag8 still rejects it.
std::optional<bool> IsDeploymentFillExact(bool terrain_blocks,std::int8_t movement,
    bool occupied,std::uint32_t flags,DeploymentFillOccupantServices&);
struct CurrentDeploymentFillResult {
    DeploymentFillStatus status{DeploymentFillStatus::InvalidUnit};
    TerrainImageStatus terrain_status{TerrainImageStatus::UninitializedImage};
    UnitImageStatus unit_status{UnitImageStatus::UnboundImage};
    DeploymentRangePlanes planes{};
};
// Current terrain/occupancy, current occupant private/AI flags and current
// partner terrain-cost rules. Movement values and GetRangeBit masks are explicit
// resolved inputs, not inferred from the old bounded AI/player movement views.
// Uses the acting Unit's force, matching UnitMoveXY's temporary Deploy+1628.
// No Unit/RNG/image mutation; no claim to complete UnitMove/Danger/placement.
CurrentDeploymentFillResult BuildCurrentDeploymentFill(
    const fates::runtime::native::NativeRuntime&,std::uint16_t unit_slot,
    const std::array<std::int8_t,1024>& movement,const DeploymentRangeMasks&,
    std::uint32_t deployment_flags);
}
