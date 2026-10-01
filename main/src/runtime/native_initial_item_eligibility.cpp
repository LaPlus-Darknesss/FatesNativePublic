#include "fates/runtime/native_initial_item_eligibility.hpp"
#include <algorithm>
namespace fates::runtime::native {
std::array<std::uint8_t,8> ResolveBaseWeaponExpLimits(
    const PersonDefinition& person, const JobDefinition& job,
    const std::array<std::uint8_t,6>& ranks) noexcept {
    auto limits=job.max_weapon_exp;
    // Named GetWeaponExpLimit predicates corroborated by Paragon's SPID bits.
    // This constructor slice has no transient Unit private-skill overrides.
    if (((person.bitflags[4]|job.bitflags[4])&4u)!=0) limits[7]=0;
    if (((person.bitflags[5]|job.bitflags[5])&4u)!=0)
        limits[6]=std::max(limits[6],ranks[2]);
    return limits;
}

InitialItemEligibility ResolveInitialItemEligibility(
    const DefinitionStore& defs, const PersonDefinition& person,
    const JobDefinition& job, const ItemDefinition& item,
    const std::array<std::uint8_t,8>& limits) noexcept {
    const auto* sub=defs.FindItemSubKind(item.weapon_category);
    if (!sub || sub->weapon_exp_group>=8) return InitialItemEligibility::No;
    const auto group=sub->weapon_exp_group;
    // SetDisposItem calls CanItemEquip(allow_staff=true, check_current_exp=false).
    if (limits[group]<item.required_weapon_exp) return InitialItemEligibility::No;
    const auto category=defs.BaseJobCategoryMask(job);
    if (group==7) {
        // Job::CanEquipFromSubKind: named JCID masks, not chapter/Job IDs.
        bool allowed=false;
        switch (item.weapon_category) {
        case 14: allowed=(category&0x40u)!=0 && (category&0x400u)==0; break;
        case 15: allowed=(category&0x80u)!=0; break;
        case 16: allowed=(category&0x40u)!=0 && (category&0x400u)!=0; break;
        case 17: allowed=(category&0x10u)!=0 && (category&0x20u)==0; break;
        case 18: allowed=(category&0x20u)!=0 && job.origin==2; break;
        case 19: allowed=(category&0x20u)!=0 && job.origin==1; break;
        default: break;
        }
        if (!allowed) return InitialItemEligibility::No;
    }
    // Unique-character and sex gates are deliberately outside this constructor
    // subset. Never approve them from weapon rank alone.
    if ((item.bitflags[2]&0x34u)!=0 || (item.bitflags[5]&0x80u)!=0 ||
        (item.bitflags[6]&7u)!=0 || (item.bitflags[7]&0x80u)!=0)
        return InitialItemEligibility::Unproved;
    // Dark-Mage-only items are proven when the current Job has that category.
    // Shadowgift and transient category overrides are not silently approximated.
    if ((item.bitflags[2]&8u)!=0 && (category&0x200u)==0)
        return InitialItemEligibility::Unproved;
    if (group==6 && ((person.bitflags[5]|job.bitflags[5])&0x20u)!=0)
        return InitialItemEligibility::Unproved; // cannot-warp staff-use subtype gate
    return InitialItemEligibility::Yes;
}

}
