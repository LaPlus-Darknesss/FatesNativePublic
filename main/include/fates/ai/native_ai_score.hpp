#pragma once
#include <cstdint>
#include <span>
namespace fates::ai::native {
// Exact arithmetic primitive reconstructed from US-SE-v1.1 retail
// AIBattleSimulator::CalculateScore + anonymous_namespace::ExpectationScoreNormalize.
// The four lane names deliberately remain semantic-neutral until CalculateIndication
// projection is closed; no retail object offsets are exposed by this API.
struct AiBattleScoreFrame {
    std::uint8_t strategy_mode{}; // retail modes 0, 1, 2
    bool clever{};                // AIThink::IsClever forces mode 2
    float lane0{};                // final score workspace lane corresponding to retail +0xD4
    float lane1{};                // +0xD8
    float lane2{};                // +0xDC
    float lane3{};                // +0xE0
    std::int8_t side0_scale{};     // signed expectation cap read from one Unit-side descriptor
    std::int8_t side1_scale{};     // signed expectation cap read from the other Unit-side descriptor
};

std::int32_t ExpectationScoreNormalizeExact(std::uint8_t exponent, float value, std::int32_t positive_cap);
std::int32_t ComposeAIBattleSimulatorScoreExact(const AiBattleScoreFrame& frame);

enum class AiScoreSelectionStatus : std::uint8_t { Empty, UniqueBest, EqualBestTie };
struct AiScoredCandidate { std::uint16_t candidate_index{}; std::int32_t score{}; };
struct AiScoreSelectionResult {
    AiScoreSelectionStatus status{AiScoreSelectionStatus::Empty};
    std::uint16_t candidate_index{};
    std::int32_t score{};
    std::uint16_t equal_best_count{};
};
// GetAttackScore uses BHI after CMP: packed score words compare unsigned,
// even when the native carrier's int32 representation has its high bit set.
constexpr bool RetailAttackScoreGreater(std::int32_t candidate,std::int32_t incumbent) noexcept {
    return static_cast<std::uint32_t>(candidate)>static_cast<std::uint32_t>(incumbent);
}
AiScoreSelectionResult SelectUniqueHighestRetailScore(std::span<const AiScoredCandidate> candidates);
} // namespace fates::ai::native
