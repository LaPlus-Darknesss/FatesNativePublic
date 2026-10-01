#include "fates/ai/native_ai_score.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
namespace fates::ai::native {
namespace {
std::int32_t TruncPositive(const float value) {
    if(!(value > 0.0f)) return 0;
    if(value >= static_cast<float>(std::numeric_limits<std::int32_t>::max())) return std::numeric_limits<std::int32_t>::max();
    return static_cast<std::int32_t>(value);
}
std::int32_t Percent100(const float value) { return TruncPositive(value * 100.0f); }
std::int32_t ShiftLeftRetail(std::int32_t value, unsigned shift) {
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(value) << shift);
}
}
std::int32_t ExpectationScoreNormalizeExact(const std::uint8_t exponent, float value,
                                             const std::int32_t positive_cap) {
    if(positive_cap > 0 && value > static_cast<float>(positive_cap)) value=static_cast<float>(positive_cap);
    const bool positive=value>0.0f;
    float scaled=value;
    if(exponent>=7) scaled*=static_cast<float>(std::uint32_t{1} << (exponent-7));
    else scaled/=static_cast<float>(std::uint32_t{1} << (7-exponent));
    std::int32_t out=static_cast<std::int32_t>(scaled); // ARM VCVT.S32.F32-compatible for supported finite nonnegative lane domain.
    if(out==0 && positive) out=1;
    const std::uint32_t umax=(exponent>=31)?0x7fffffffu:((std::uint32_t{1} << exponent)-1u);
    if(out>static_cast<std::int32_t>(umax)) out=static_cast<std::int32_t>(umax);
    return out;
}
std::int32_t ComposeAIBattleSimulatorScoreExact(const AiBattleScoreFrame& f) {
    const std::uint8_t mode=f.clever?2:f.strategy_mode;
    std::int32_t score=0;
    if(mode==0) {
        if(f.lane0>=0.3f) score+=Percent100(f.lane0);
        score=ShiftLeftRetail(score,11);
        score+=ExpectationScoreNormalizeExact(11,f.lane2,f.side1_scale);
        score=ShiftLeftRetail(score,7);
        score+=(f.lane1<0.5f)?127:(127-Percent100(f.lane1));
        score=ShiftLeftRetail(score,7);
        score+=ExpectationScoreNormalizeExact(7,f.lane3,f.side0_scale);
        return score;
    }
    if(mode==1) {
        if(f.lane0>=0.3f) score+=Percent100(f.lane0);
        score=ShiftLeftRetail(score,2);
        if(f.lane1<0.3f) score+=2;
        else if(f.lane1<0.7f) score+=1;
        score=ShiftLeftRetail(score*2+1,22);
        score+=3*ExpectationScoreNormalizeExact(20,f.lane2,f.side1_scale);
        score-=ExpectationScoreNormalizeExact(20,f.lane3,f.side0_scale);
        return score;
    }
    if(mode==2) {
        // 0x001F6D30: preserve the seven-bit kill-chance field below survival.
        score=ShiftLeftRetail(127-Percent100(f.lane1),7);
        if(f.lane0>=0.3f) score+=Percent100(f.lane0);
        score=ShiftLeftRetail(score*2+1,17);
        score+=ExpectationScoreNormalizeExact(15,f.lane2,f.side1_scale);
        score-=ExpectationScoreNormalizeExact(15,f.lane3,f.side0_scale);
        return score;
    }
    return 0;
}
AiScoreSelectionResult SelectUniqueHighestRetailScore(std::span<const AiScoredCandidate> candidates) {
    AiScoreSelectionResult out{};
    if(candidates.empty()) return out;
    auto best=candidates.front(); std::uint16_t count=1;
    for(std::size_t i=1;i<candidates.size();++i) {
        if(RetailAttackScoreGreater(candidates[i].score,best.score)) { best=candidates[i]; count=1; }
        else if(candidates[i].score==best.score) { ++count; }
    }
    out.candidate_index=best.candidate_index; out.score=best.score; out.equal_best_count=count;
    out.status=(count==1)?AiScoreSelectionStatus::UniqueBest:AiScoreSelectionStatus::EqualBestTie;
    return out;
}
} // namespace fates::ai::native
