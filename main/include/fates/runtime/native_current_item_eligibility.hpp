#pragma once
#include "fates/runtime/native_runtime.hpp"
#include <optional>
namespace fates::runtime::native {
struct CurrentEquipmentUnit {
    std::uint64_t private_flags{},person_flags{},job_flags{};
    std::uint32_t public_flags{};
    std::uint16_t job_category{};
    std::uint8_t job_origin{},staff_threshold{};
    std::array<std::uint8_t,8> job_limits{},weapon_exp{};
};
struct CurrentEquipmentItem {
    std::uint64_t flags{};
    std::int8_t subkind{},group{};
    std::uint8_t required_exp{},use_kind{};
};
struct CurrentEquipmentServices {
    virtual ~CurrentEquipmentServices()=default;
    virtual std::optional<bool> HasEquippedSkill(std::int16_t)=0;
    virtual std::optional<bool> PersonIsDownload()=0;
};
enum class CurrentItemEligibility : std::uint8_t {No,Yes,UnresolvedSkill,UnresolvedDownload,InvalidDefinition};
// Cached CanItemEquip body. Services are queried only at their original branch,
// including repeated download queries for multiple personal restrictions.
CurrentItemEligibility CanEquipCurrentItemExact(const CurrentEquipmentUnit&,
    const CurrentEquipmentItem&,bool allow_staff,bool check_current_exp,CurrentEquipmentServices&);
std::uint8_t CurrentWeaponExpLimitExact(std::uint8_t group,std::uint8_t job_limit,
    std::uint64_t merged_private_flags,std::uint8_t staff_threshold) noexcept;
bool JobCanEquipSubKindExact(std::int8_t subkind,std::int8_t group,
    std::uint16_t category,std::uint8_t origin,std::uint8_t group_limit) noexcept;
bool IsEquippedSkillExact(std::int16_t query,std::uint16_t personal_skill,
    const std::array<std::uint16_t,5>& equipped) noexcept;
// These explicit values are scoped inputs to the definition-only overload.
// Runtime queries prefer the owned GameUser selector. Download fallback is used
// only when definition registry provenance is
// unknown; it cannot override an owned registry answer.
struct CurrentEquipmentContext {
    std::optional<std::uint8_t> difficulty{};
    std::optional<bool> person_is_download{};
};
std::optional<bool> ProjectCurrentEquippedSkill(const NativeRuntime&,const UnitState&,std::int16_t query) noexcept;
CurrentItemEligibility ProjectCurrentItemEligibility(const NativeRuntime&,const UnitState&,
    const ItemDefinition&,bool allow_staff,bool check_current_exp,const CurrentEquipmentContext& context={});
std::optional<bool> ProjectCurrentEquippedSkill(const PersonDefinition&,const UnitState&,
    std::int16_t query,std::optional<std::uint8_t> difficulty={}) noexcept;
std::array<std::uint8_t,8> ResolveCurrentWeaponExpLimits(const PersonDefinition&,
    const JobDefinition&,const UnitState&,const std::array<std::uint8_t,6>& ranks) noexcept;
CurrentItemEligibility ProjectCurrentItemEligibility(const DefinitionStore&,const UnitState&,
    const ItemDefinition&,bool allow_staff,bool check_current_exp,const CurrentEquipmentContext& context={});
}
