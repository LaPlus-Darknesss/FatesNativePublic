#include "fates/game/capability.hpp"
#include "fates/game/private_skill.hpp"
#include "fates/detail/unit_ownership_runtime.hpp"

const wchar_t* Capability::GetName(int capabilityIndex) {
    // PROVEN: retail indexes a global table of message identifiers and passes
    // the selected identifier to Mess::Get. The table itself is not yet
    // source-owned, so only that table lookup remains behind the boundary.
    return fates::decomp_detail::UnitOwnershipValue<const wchar_t*>("Capability.GetName", capabilityIndex);
}

const PrivateSkill* PrivateSkill::Get(const char* identifier) {
    // PROVEN: GetSurely semantics through the PrivateSkill IdentHash.
    return fates::decomp_detail::UnitOwnershipValue<const PrivateSkill*>("PrivateSkill.GetSurely", identifier);
}
