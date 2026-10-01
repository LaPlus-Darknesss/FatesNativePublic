#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
namespace fates::ai::native {
// Portable arithmetic extracted from US-SE-v1.1 AIBattleSimulator::CalculateIndication.
// Names remain semantic-neutral where retail arithmetic is proved but the durable
// gameplay meaning of a workspace field is not yet earned.
inline constexpr std::size_t kAiIndicationCount = 4;
inline constexpr std::size_t kAiIndicationWorkspaceBytes = 44;
inline constexpr std::uint32_t kAiIndicationDoubleWeightFlag = 0x800u;

struct AiIndicationProbabilityPartition {
    float residual{};          // retail workspace +0x0C after partitioning
    float outcome_a{};         // +0x10 after overlap removal
    float outcome_b{};         // +0x14 after overlap removal
    float overlap{};           // +0x18
    float hit_complement{};    // +0x20 = 1 - denominator10000*0.0001
};

// Exact inputs consumed by the final +0x24 expected-value construction. The three
// integer magnitudes correspond to retail workspace +0x00/+0x04/+0x08. They are
// intentionally not renamed as damage/HP/etc. until the surrounding BattleInfo
// projection earns those semantic identities.
struct AiIndicationValueFrame {
    std::int32_t magnitude0{};          // workspace +0x00
    std::int32_t magnitude1{};          // workspace +0x04
    std::int32_t magnitude2{};          // workspace +0x08
    AiIndicationProbabilityPartition probabilities{};
    float extra_probability{};          // workspace +0x1C
    bool special_secondary_branch{};    // workspace +0x28 byte
    std::uint32_t flags{};              // source item/indication flags; 0x800 doubles final value
};

float CalculateHitComplementExact(std::int32_t denominator10000) noexcept;
float ProbabilityPercentExact(std::int32_t probability_percent) noexcept;
float WeightProbabilityByHitExact(std::int32_t probability_percent, float hit_complement) noexcept;
AiIndicationProbabilityPartition ComposeIndicationProbabilityPartitionExact(
    float hit_complement, float outcome_a_raw, float outcome_b_raw) noexcept;
float ApplyIndicationDoubleWeightExact(float value, std::uint32_t flags) noexcept;

// Retail complement accumulation used by the later probability lane:
// accumulated += percent * (1-accumulated) * 0.01.
float AccumulateIndicationExtraProbabilityExact(float accumulated,
                                                std::int32_t probability_percent) noexcept;

// Exact deterministic construction of retail workspace +0x24 from the already
// resolved workspace magnitudes/probabilities. Consumes no RNG.
float ComposeIndicationExpectedValueExact(const AiIndicationValueFrame& frame) noexcept;
std::array<float, kAiIndicationCount> ComposeFourIndicationExpectedValuesExact(
    const std::array<AiIndicationValueFrame, kAiIndicationCount>& frames) noexcept;
} // namespace fates::ai::native
