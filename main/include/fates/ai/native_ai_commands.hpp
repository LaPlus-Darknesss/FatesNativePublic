#pragma once
#include "fates/runtime/native_runtime.hpp"
#include <optional>
#include <span>

namespace fates::ai::native {
// Identity lookup may include retail fallback semantics when its archive-list
// provider owns those facts. Unknown lookup is distinct from a negative match.
struct AiCommandIdentityLookup {
    virtual ~AiCommandIdentityLookup() = default;
    virtual std::optional<std::uint16_t> Person(std::uint16_t id) const = 0;
    virtual std::optional<std::uint16_t> Job(std::uint16_t id) const = 0;
};
struct AiCrossfireTarget {
    std::uint16_t person{},job{};
    std::uint8_t force{};
    std::optional<std::uint8_t> band;
    std::optional<std::uint64_t> private_skills;
};
enum class AiCrossfireStatus { Ready, InvalidUnit, MissingDeclaration, MissingActivity, MissingTargetFacts, MissingIdentityLookup, UnterminatedDeclaration };
struct AiCrossfireResult {
    AiCrossfireStatus status{AiCrossfireStatus::Ready};
    bool eligible{};
    std::uint32_t visited{}, argument_substitutions{}, target_queries{};
};
bool IsAiArgument(std::int16_t value) noexcept;
std::int16_t ResolveAiAttackArgument(std::int16_t value,const std::array<std::int16_t,4>& args) noexcept;
AiCrossfireResult EvaluateCrossfireCommandsExact(
    std::span<const runtime::native::AiCommandDefinition> commands,
    std::optional<std::uint8_t> activity,const std::array<std::int16_t,4>& attack_args,
    const AiCrossfireTarget& target,const AiCommandIdentityLookup& identities);
// Native world service for the Dual planner. No RNG, world mutation or chapter
// dispatch. Absent numeric Person/Job lookups remain refused until archive-list
// ordering/fallback is owned; no null/default-person substitution is invented.
AiCrossfireResult HasActiveAttackCommandForCrossfire(
    const runtime::native::NativeRuntime& runtime,std::uint16_t actor_slot,std::uint16_t target_slot);
} // namespace fates::ai::native
