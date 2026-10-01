#include "fates/ai/native_ai_candidate_scoring.hpp"
#include <cstdint>
namespace fates::ai::native {
namespace {
std::uint32_t NextRandom31(fates::runtime::native::NativeRandomState& s) noexcept {
    const std::uint32_t x=s.words[0], y=s.words[1], z=s.words[2], w=s.words[3];
    const std::uint32_t t=x ^ (x << 11u);
    s.words[0]=y; s.words[1]=z; s.words[2]=w;
    const std::uint32_t nw=(t ^ (t >> 8u)) ^ (w ^ (w >> 19u));
    s.words[3]=nw;
    return nw & 0x7fffffffu;
}
void SeedRandomExact(fates::runtime::native::NativeRandomState& s, const std::uint32_t seed) noexcept {
    constexpr std::uint32_t kMul=0x6c078965u;
    s.words[0]=kMul * (seed ^ (seed >> 30u));
    s.words[1]=kMul * (s.words[0] ^ (s.words[0] >> 30u)) + 1u;
    s.words[2]=kMul * (s.words[1] ^ (s.words[1] >> 30u)) + 2u;
    s.words[3]=kMul * (s.words[2] ^ (s.words[2] >> 30u)) + 3u;
    s.initialized=true;
    for (int i=0;i<20;++i) (void)NextRandom31(s);
}
}
AiOrdinaryFrameBinding BindOrdinaryNoSpecialBattleInfoExact(
    const AiBattleInfoOrdinaryFacts& facts, const std::uint32_t indication_flags) noexcept {
    AiOrdinaryFrameBinding out{};
    out.indication.magnitude0=facts.simple_power;
    out.indication.magnitude1=0;
    out.indication.magnitude2=facts.simple_damage_rate;
    out.indication.probabilities.hit_complement=CalculateHitComplementExact(facts.hit_denominator10000);
    // CalculateIndication 001F7848..001F78B4: a normal critical is not a
    // skill-proc lane. Preserve it even when every special skill outcome is zero.
    out.indication.probabilities=ComposeIndicationProbabilityPartitionExact(
        out.indication.probabilities.hit_complement,
        WeightProbabilityByHitExact(facts.simple_critical,out.indication.probabilities.hit_complement),0.0f);
    out.indication.extra_probability=0.0f;
    out.indication.special_secondary_branch=false;
    out.indication.flags=indication_flags;
    out.state_slot.flags=indication_flags;
    out.state_slot.special_branch=false;
    out.state_slot.sequence_count=facts.simple_attack_count;
    out.kill_threshold_current_hp=facts.opposing_current_hp;
    out.simple_hit=facts.simple_hit;
    out.hit_denominator10000=facts.hit_denominator10000;
    return out;
}
bool InitializeAiLocalRandomFromSystem(fates::runtime::native::NativeGameState& state) noexcept {
    if (!state.rng.system_state.initialized) return false;
    const std::uint32_t seed=NextRandom31(state.rng.system_state);
    ++state.rng.system;
    SeedRandomExact(state.rng.ai_state,seed);
    state.rng.ai += 20u;
    return true;
}
std::uint32_t DrawAiLocalRandomBoundedExact(fates::runtime::native::NativeGameState& state,
                                            const std::uint32_t max_exclusive) noexcept {
    if (!state.rng.ai_state.initialized || max_exclusive==0u) return 0u;
    const std::uint32_t raw=NextRandom31(state.rng.ai_state);
    ++state.rng.ai;
    return raw % max_exclusive;
}
AiTacticalCandidateSelectionResult SelectRetailScoredCandidateExact(
    const std::span<const AiTacticalScoredCandidate> candidates,
    fates::runtime::native::NativeGameState& state) noexcept {
    AiTacticalCandidateSelectionResult out{};
    if (candidates.empty()) return out;
    out.status=AiCandidateSelectionStatus::Selected;
    out.selected=candidates.front();
    for (std::size_t i=1;i<candidates.size();++i) {
        const auto& c=candidates[i];
        if (RetailAttackScoreGreater(c.score,out.selected.score)) { out.selected=c; continue; }
        if (c.score != out.selected.score) continue;
        ++out.equal_score_encounters;
        if (!state.rng.ai_state.initialized) { out.status=AiCandidateSelectionStatus::MissingAiRandom; return out; }
        ++out.tie_rng_draws;
        if (DrawAiLocalRandomBoundedExact(state,2u)==0u) out.selected=c;
    }
    return out;
}

AiResolvedBattleScoreInput BindOrdinaryResolvedBattleScoreInputExact(
    const AiOrdinaryTacticalCandidateInput& c) noexcept {
    AiResolvedBattleScoreInput in{};
    const auto s0=BindOrdinaryNoSpecialBattleInfoExact(c.side0,c.side0_flags);
    const auto s1=BindOrdinaryNoSpecialBattleInfoExact(c.side1,c.side1_flags);
    in.indications[0]=s0.indication;
    in.indications[1]=s1.indication;
    // Slot 2 assists primary 0, slot 3 assists primary 1. Powers already carry
    // GetSimplePower's assist half; do not halve them again in the AI adapter.
    // CalculateIndication itself can fill a present inactive source. Participation
    // is controlled separately by the primary Side +0x94 byte (CalculateDual).
    in.state_slots[0]=s0.state_slot;
    in.state_slots[1]=s1.state_slot;
    for(std::size_t side=0;side<2;++side) {
        const auto& source=c.assists[side];
        if(!source.present) continue;
        const auto bound=BindOrdinaryNoSpecialBattleInfoExact(source.side,source.indication_flags);
        in.indications[side+2]=bound.indication;
        in.state_slots[side+2]=bound.state_slot;
        in.state_slots[side].special_branch=source.active;
    }
    in.side0_current_hp=c.side0_current_hp;
    in.side1_current_hp=c.side1_current_hp;
    in.side0_scale=c.side0_score_cap;
    in.side1_scale=c.side1_score_cap;
    in.process_side1_first=c.process_side1_first;
    in.strategy_mode=c.strategy_mode;
    in.clever=c.clever;
    return in;
}
std::int32_t ScoreOrdinaryTacticalCandidateExact(
    const AiOrdinaryTacticalCandidateInput& c) noexcept {
    if(c.requires_special_indication) return 0;
    return ComposeResolvedAIBattleSimulatorScoreExact(
        BindOrdinaryResolvedBattleScoreInputExact(c)).score;
}
AiOrdinaryMultiCandidateResult ScoreAndSelectOrdinaryTacticalCandidatesExact(
    const std::span<const AiOrdinaryTacticalCandidateInput> candidates,
    fates::runtime::native::NativeGameState& state) {
    AiOrdinaryMultiCandidateResult out{};
    if(candidates.empty()) return out;
    out.scored_candidates.reserve(candidates.size());
    for(const auto& c:candidates) {
        if(c.requires_special_indication) {
            out.status=AiOrdinaryMultiCandidateStatus::UnsupportedSpecialIndication;
            return out;
        }
        out.scored_candidates.push_back({c.candidate_index,c.target_slot,c.attack_x,c.attack_y,
            ScoreOrdinaryTacticalCandidateExact(c)});
    }
    const auto sel=SelectRetailScoredCandidateExact(out.scored_candidates,state);
    out.equal_score_encounters=sel.equal_score_encounters;
    out.tie_rng_draws=sel.tie_rng_draws;
    if(sel.status==AiCandidateSelectionStatus::MissingAiRandom) {
        out.status=AiOrdinaryMultiCandidateStatus::MissingAiRandom;
        return out;
    }
    if(sel.status!=AiCandidateSelectionStatus::Selected) return out;
    out.status=AiOrdinaryMultiCandidateStatus::Selected;
    out.selected=sel.selected;
    return out;
}

} // namespace fates::ai::native
