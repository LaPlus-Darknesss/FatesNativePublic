#pragma once
#include "fates/map/native_deployment_workspace_state.hpp"
#include "fates/map/native_unit_deployment.hpp"
namespace fates::map::native {
enum class DeploymentWorkspaceStatus : std::uint8_t {
    Ok,InvalidPhase,StaleRevision,Unselected,ChangedContext,RevisionExhausted,
    AlreadyCarried,BuildFailed
};
struct DeploymentWorkspaceView {
    DeploymentWorkspaceStatus status{DeploymentWorkspaceStatus::Unselected};
    std::uint64_t selection_revision{};
    std::uint8_t force{};
    const DeploymentMovementPlane* movement{};
    const DeploymentRangePlanes* ranges{};
    TerrainImageView terrain;
};
// Deploy::TurnReset's selection only. The actual content/geometry owner is read
// lazily by users; missing terrain cannot be disguised as a cleared plane.
DeploymentWorkspaceStatus ResetCurrentDeploymentWorkspace(
    fates::runtime::native::NativeRuntime&,std::uint64_t expected_phase_revision);
// Explicit carried bytes, once per workspace. Does not select a phase or claim
// constructor/save ownership; intended for the shared carried-state boundary.
DeploymentWorkspaceStatus RestoreCurrentDeploymentWorkspaceContents(
    fates::runtime::native::NativeRuntime&,const DeploymentFields&);
// Borrowed current-call view. No pointer into a different runtime is retained.
DeploymentWorkspaceView ReadCurrentDeploymentWorkspace(const fates::runtime::native::NativeRuntime&);
struct WorkspaceUnitDeploymentResult {
    DeploymentWorkspaceStatus status{DeploymentWorkspaceStatus::Unselected};
    CurrentUnitDeploymentResult deployment;
};
// Uses the proven UnitMoveXY pipeline. Movement becomes known on success; range
// planes become known only if Fill was reached. Unreached unknown ranges stay so.
WorkspaceUnitDeploymentResult BuildCurrentUnitDeploymentInWorkspace(
    fates::runtime::native::NativeRuntime&,std::uint16_t slot,int x,int y,int power,
    std::uint32_t flags,std::uint32_t extra,const DeploymentRangeMasks* ranges=nullptr,
    const DeploymentRouteSnapshot* route=nullptr);
}
