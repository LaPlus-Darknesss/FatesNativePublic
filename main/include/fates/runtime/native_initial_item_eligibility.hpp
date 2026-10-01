#pragma once
#include "fates/runtime/native_definition_store.hpp"
#include <array>
namespace fates::runtime::native {
// Shared existing constructor subset. Transient private overrides, unique/sex
// restrictions and unrepresented equipment variants are NOT granted here.
enum class InitialItemEligibility { No, Yes, Unproved };
std::array<std::uint8_t,8> ResolveBaseWeaponExpLimits(
    const PersonDefinition&, const JobDefinition&, const std::array<std::uint8_t,6>&) noexcept;
InitialItemEligibility ResolveInitialItemEligibility(
    const DefinitionStore&, const PersonDefinition&, const JobDefinition&,
    const ItemDefinition&, const std::array<std::uint8_t,8>&) noexcept;
}
