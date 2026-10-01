#pragma once
#include "fates/map/native_deployment_fill.hpp"
#include "fates/runtime/native_unit_capabilities.hpp"
namespace fates::map::native {
struct DeploymentRangeServices {
    virtual ~DeploymentRangeServices()=default;
    // who0 is the primary Unit; who1 is its selected partner. Current experience
    // is always checked. Staff is allowed by GetRangeBit, disallowed by cannon.
    virtual std::optional<bool> CanEquip(unsigned who,unsigned index,bool staff)=0;
    virtual std::optional<int> EquippedIndex(unsigned who)=0;
    virtual std::optional<std::uint64_t> ItemFlags(unsigned who,unsigned index)=0;
    virtual std::optional<bool> Silenced(unsigned who)=0;
    virtual std::optional<bool> IsMagic(unsigned who,unsigned index)=0;
    virtual std::optional<int> Inner(unsigned who,unsigned index)=0;
    virtual std::optional<int> Outer(unsigned who,unsigned index)=0;
    virtual std::optional<std::uint8_t> Group(unsigned who,unsigned index)=0;
    virtual std::optional<bool> PartnerExists()=0;
    virtual std::optional<bool> CannonCategory()=0;
    virtual std::optional<bool> CannonModificationSkill()=0;
};
// Intrinsic Shooter inventory predicate. Stops at the first admitted weapon.
std::optional<bool> CannonItemAvailableExact(bool respect_silence,DeploymentRangeServices&);
// Member flags and argument flags are independent original inputs. Primary-only
// masks are captured before partner and cannon contributions. No partial result.
std::optional<DeploymentRangeMasks> BuildDeploymentRangesExact(std::uint32_t member_flags,
    std::uint32_t argument_flags,const fates::runtime::native::ItemRangeRuleDefinitions&,
    DeploymentRangeServices&);
enum class DeploymentRangeStatus : std::uint8_t {
    Ok,InvalidUnit,UnboundInventory,StaleInventory,MissingDefinition,
    UnresolvedEligibility,UnboundEnhance,MalformedPair,UnresolvedItemRange,
    UnresolvedCategory,UnresolvedSkill,InvalidRange
};
struct CurrentDeploymentRangeResult {
    DeploymentRangeStatus status{DeploymentRangeStatus::InvalidUnit};
    fates::runtime::native::UnitCapabilityStatus item_range_status{fates::runtime::native::UnitCapabilityStatus::Ok};
    DeploymentRangeMasks masks{};
};
struct CurrentCannonItemResult {DeploymentRangeStatus status{DeploymentRangeStatus::InvalidUnit};bool available{};};
CurrentCannonItemResult BuildCurrentCannonItemAvailability(
    const fates::runtime::native::NativeRuntime&,std::uint16_t unit_slot,bool respect_silence);
CurrentDeploymentRangeResult BuildCurrentDeploymentRanges(
    const fates::runtime::native::NativeRuntime&,std::uint16_t unit_slot,
    std::uint32_t member_flags,std::uint32_t argument_flags);
}
