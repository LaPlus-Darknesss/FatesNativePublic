#pragma once
#include "fates/runtime/native_runtime.hpp"
#include "fates/cmvm/native_trick_archive.hpp"
#include <span>
#include <string_view>
namespace fates::map::native {
enum class NativeScenarioKind : std::uint8_t { OrdinaryCampaign, Versus, Castle };
struct ScenarioScriptInput {
    std::string_view identity;
    std::span<const std::uint8_t> original_bytes;
};
enum class ScenarioTrickStatus : std::uint8_t {
    Ok, InvalidMap, InvalidMode, AlreadyBound, InvalidArchiveSet,
    InvalidArchive, RegistrationSideEffectRequired, MissingContext, StaleContext,
    InvalidActor, IntrinsicCannon, MountedCannonPresent
};
struct ScenarioTrickResult {
    ScenarioTrickStatus status{ScenarioTrickStatus::MissingContext};
    fates::cmvm::TrickArchiveStatus archive_status{fates::cmvm::TrickArchiveStatus::Ok};
    std::uint16_t archive_index{0xffffu},function_index{0xffffu};
    std::uint32_t declarations{},mounted_cannons{};
};
std::array<std::uint8_t,15> ConstructInitialTrickBytes(
    const std::array<std::uint32_t,13>& arguments,std::uint8_t flags=1) noexcept;
// This is an initial script-only native scenario owner, not a save importer or
// claim that the supplied list is every script attached by the retail VM.
// Order is explicit attachment order; repeated identity / relocated archives /
// initializers / Castle / Versus / special user mode / type-14 Done are refused.
// The caller must invalidate before executing any unmodeled event/world service.
ScenarioTrickResult RegisterInitialScenarioScripts(fates::runtime::native::NativeRuntime&,
    NativeScenarioKind,std::uint8_t difficulty,std::span<const ScenarioScriptInput>);
void InvalidateScenarioScripts(fates::runtime::native::NativeRuntime&) noexcept;
bool ScenarioScriptsMatch(const fates::runtime::native::NativeRuntime&) noexcept;
// A necessary empty-input proof, NOT AICannon Update/GetScore. Any registered
// cannon stays unresolved even if currently unreachable or apparently disabled.
// Intrinsic Ballistician action is checked separately from mounted declarations.
bool IntrinsicCannonCategory(std::uint16_t category) noexcept;
ScenarioTrickResult InspectScriptCannonAbsence(const fates::runtime::native::NativeRuntime&,std::uint16_t);
// Three retail early gates only. Fresh decision Clear still belongs to the
// caller. Hard/Lunatic candidates are not ranked by this function.
bool DualAlternativeEarlyRejectedExact(bool private_no_dual,bool can_dual,std::uint8_t difficulty) noexcept;
bool ScenarioDualAlternativeEarlyRejected(const fates::runtime::native::NativeRuntime&,std::uint16_t) noexcept;
} // namespace fates::map::native
