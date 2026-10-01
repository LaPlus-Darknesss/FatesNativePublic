#pragma once
#include "fates/map/native_deployment_movement.hpp"
#include "fates/map/native_deployment_ranges.hpp"
#include "fates/runtime/native_movement_power.hpp"
namespace fates::map::native {
enum class UnitDeploymentStatus : std::uint8_t {
    Ok,InvalidUnit,MissingPower,MissingProhibition,MissingCostFree,MissingFallback,
    MissingPassSkill,MovementFailed,FillFailed,MissingPhase,InvalidCurrentCell
};
struct UnitDeploymentServices {
    virtual ~UnitDeploymentServices()=default;
    virtual std::optional<int> CurrentMovementPower()=0;
    virtual std::optional<bool> MovementProhibited()=0;
    virtual std::optional<bool> CostFree()=0;
    virtual std::optional<bool> BaseCostFallback()=0;
    virtual std::optional<bool> PassSkill()=0;
    virtual bool Move(int power,std::uint32_t flags)=0;
    virtual bool Fill(std::uint32_t flags)=0;
    virtual std::optional<bool> SameAsActiveForce()=0;
    virtual bool EraseCurrentCell()=0;
};
struct UnitDeploymentResult {
    UnitDeploymentStatus status{UnitDeploymentStatus::Ok};
    int power{};std::uint32_t flags{};bool filled{},erased_current_cell{};
};
// UnitMoveXY control flow. Move/Fill use the acting Unit force; a native caller
// supplies it directly rather than mutating/restoring a shared Deploy force byte.
// The late erase happens AFTER fill and uses current Unit position, not origin.
UnitDeploymentResult RunUnitDeploymentExact(int requested_power,std::uint32_t flags,
    std::uint32_t additional_flags,UnitDeploymentServices&);
struct CurrentUnitDeploymentResult {
    UnitDeploymentResult operation{};
    fates::runtime::native::MovementPowerStatus power_status{fates::runtime::native::MovementPowerStatus::Ok};
    DeploymentMovementStatus movement_status{DeploymentMovementStatus::Ok};
    DeploymentFillStatus fill_status{DeploymentFillStatus::Ok};
    DeploymentRangeStatus range_status{DeploymentRangeStatus::Ok};
    fates::runtime::native::UnitCapabilityStatus item_range_status{fates::runtime::native::UnitCapabilityStatus::Ok};
};
// Current movement preparation, Move, GetRangeBit and geometric UnitFill.
// Range masks are derived only if flagmask27F0000 reaches Fill. The optional
// resolved override supports isolated composition callers. Unreached range
// planes are preserved. Both fields publish atomically, including late failures.
CurrentUnitDeploymentResult BuildCurrentUnitDeployment(DeploymentFields&,
    const fates::runtime::native::NativeRuntime&,std::uint16_t unit_slot,int x,int y,
    int requested_power,std::uint32_t flags,std::uint32_t additional_flags,
    const DeploymentRangeMasks* ranges=nullptr,const DeploymentRouteSnapshot* route=nullptr);
}
