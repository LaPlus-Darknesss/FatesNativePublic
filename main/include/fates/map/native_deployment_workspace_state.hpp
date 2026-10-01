#pragma once
#include "fates/map/native_deployment_fields.hpp"
#include "fates/runtime/native_phase_state.hpp"
#include <optional>
namespace fates::map::native {
// Semantic internal work planes. Unknown carried bytes stay unknown. A reset
// selects this storage and the current terrain owner; it never clears a plane.
class NativeDeploymentWorkspaceState {
    friend struct DeploymentWorkspaceAccess;
    std::optional<DeploymentMovementPlane> movement_;
    std::optional<DeploymentRangePlanes> ranges_;
    std::optional<fates::runtime::native::TacticalPhaseContext> selected_;
    std::uint64_t selection_revision_{};
};
}
