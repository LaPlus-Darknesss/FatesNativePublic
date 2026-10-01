#include "fates/ai/native_ai_shared_battle_preview.hpp"
#include "fates/battle/native_battle_transaction.hpp"
#include <algorithm>
#include <array>
#include <cstdint>

namespace fates::ai::native {
namespace {
bool ScoreFrameSkillSetSupported(const fates::runtime::native::UnitState& u) noexcept {
    // These B007-relevant skills are already represented outside special indication
    // proc lanes: seals/Draconic Hex/Inevitable End and Poison/Savage/Grisly are
    // post-combat; Wary Fighter is already folded into simple attack count.
    constexpr std::array<std::uint16_t,11> kSupported{{10,11,12,13,14,15,64,101,102,103,127}};
    for(const auto id:u.equipped_skill_ids) {
        if(id==0 || fates::battle::native::IsSupportedLocalAroundSkill(id)) continue;
        if(std::find(kSupported.begin(),kSupported.end(),id)==kSupported.end()) return false;
    }
    return true;
}
} // namespace
AiCandidatePreviewStatus ProjectSharedOrdinaryBattleCandidate(
    const fates::runtime::native::NativeRuntime& r,
    const AiCandidatePreviewRequest& req,
    AiOrdinaryTacticalCandidateInput& out,
    fates::battle::native::BattlePreviewResult* resolved) {
    if(req.unit_slot>=r.game.units.size() || req.target_slot>=r.game.units.size())
        return AiCandidatePreviewStatus::Rejected;
    const auto& actor=r.game.units[req.unit_slot];
    const auto& target=r.game.units[req.target_slot];
    if(!ScoreFrameSkillSetSupported(actor) || !ScoreFrameSkillSetSupported(target))
        return AiCandidatePreviewStatus::UnsupportedSpecialIndication;
    if(!actor.ai.runtime_tuning_bound || actor.ai.battle_rate>2u)
        return AiCandidatePreviewStatus::Rejected;
    if(actor.current_hp<0 || actor.current_hp>127 || target.current_hp<0 || target.current_hp>127)
        return AiCandidatePreviewStatus::Rejected;
    const auto preview=req.inventory_index==0xff
        ?fates::battle::native::PreviewOrdinaryBattleAt(r,req.unit_slot,req.target_slot,req.attack_x,req.attack_y)
        :fates::battle::native::PreviewOrdinaryBattleAtInventory(r,req.unit_slot,req.target_slot,req.attack_x,req.attack_y,req.inventory_index);
    if(resolved) *resolved=preview;
    if(preview.status!=fates::battle::native::BattleTransactionStatus::Ok)
        return AiCandidatePreviewStatus::Rejected;
    // The mechanical owner has resolved the local sources and both opposing
    // defensive lanes. Bind every present source; never drop active support from
    // the old two-primary frame, and never infer eligibility from inventory alone.
    for(std::size_t side=0;side<preview.assists.size();++side) {
        const auto& assist=preview.assists[side];
        if(!assist.present) {
            if(assist.active) return AiCandidatePreviewStatus::UnsupportedSpecialIndication;
            continue;
        }
        if(assist.unit_slot>=r.game.units.size() ||
           !ScoreFrameSkillSetSupported(r.game.units[assist.unit_slot]))
            return AiCandidatePreviewStatus::UnsupportedSpecialIndication;
        const auto& primary=side==0?preview.attacker:preview.defender;
        if(assist.side.attack_count>1 ||
           assist.active!=(assist.side.attack_count>0) ||
           (assist.active && primary.attack_count==0))
            return AiCandidatePreviewStatus::UnsupportedSpecialIndication;
    }
    auto facts=[](const fates::battle::native::BattlePreviewSide& p,
                  const std::int32_t opposing_hp) {
        AiBattleInfoOrdinaryFacts f{};
        f.simple_power=p.simple_damage;
        f.simple_hit=p.simple_hit;
        f.simple_critical=p.simple_critical;
        f.hit_denominator10000=static_cast<std::int32_t>(
            fates::battle::native::RetailHybridHitThreshold(p.simple_hit));
        f.simple_damage_rate=p.simple_damage_rate;
        f.simple_attack_count=p.attack_count;
        f.opposing_current_hp=opposing_hp;
        return f;
    };
    out={};
    out.candidate_index=req.candidate_index;
    out.target_slot=req.target_slot;
    out.attack_x=req.attack_x;
    out.attack_y=req.attack_y;
    out.side0=facts(preview.attacker,target.current_hp);
    out.side1=facts(preview.defender,actor.current_hp);
    for(std::size_t side=0;side<preview.assists.size();++side) {
        const auto& assist=preview.assists[side];
        if(!assist.present) continue;
        auto& bound=out.assists[side];
        bound.present=true;bound.active=assist.active;bound.unit_slot=assist.unit_slot;
        bound.side=facts(assist.side,side==0?target.current_hp:actor.current_hp);
        bound.indication_flags=0u; // repeated-strike items remain outside live admission
    }
    out.side0_current_hp=actor.current_hp;
    out.side1_current_hp=target.current_hp;
    // Retail score normalization and kill recursion both source current HP, but
    // Pass87 keeps the API concepts separate to prevent future adapter conflation.
    out.side0_score_cap=static_cast<std::int8_t>(actor.current_hp);
    out.side1_score_cap=static_cast<std::int8_t>(target.current_hp);
    // Vantage/SEID_待ち伏せ owns the reverse order path and is deliberately not
    // accepted by ScoreFrameSkillSetSupported in this ordinary provider.
    out.process_side1_first=false;
    out.strategy_mode=actor.ai.battle_rate;
    // Retail AIThink::IsClever is false for ENEMY force 1 in this executable.
    out.clever=actor.force_type!=1u;
    out.side0_flags=0u;
    out.side1_flags=0u;
    out.requires_special_indication=false;
    return AiCandidatePreviewStatus::Ok;
}
std::optional<AiDualBattleFacts> ProjectSharedDualBattleFacts(
    const fates::runtime::native::NativeRuntime& r,const AiDualPreviewRequest& req,
    fates::battle::native::BattlePreviewResult* resolved) {
    const auto preview=fates::battle::native::PreviewBattleWithForcedSource(r,
        {req.actor,req.target,req.source,req.attack_x,req.attack_y,req.actor_weapon,req.source_weapon});
    if(resolved)*resolved=preview;
    if(preview.status!=fates::battle::native::BattleTransactionStatus::Ok ||
       !preview.assists[0].present || preview.assists[0].unit_slot!=req.source)return std::nullopt;
    const auto& side=preview.assists[0].side;
    return AiDualBattleFacts{side.simple_damage,side.simple_hit,side.simple_critical,side.simple_damage_rate,side.attack_count};
}
namespace {
AiCandidatePreviewStatus BuildSharedOrdinaryPreview(
    const fates::runtime::native::NativeRuntime& r,const AiCandidatePreviewRequest& req,
    AiOrdinaryTacticalCandidateInput& out,void*) {
    if(req.unit_slot>=r.game.units.size()) return AiCandidatePreviewStatus::Rejected;
    // The older immediate-action path is not the newly composed per-weapon
    // planning path. Retain its authorization boundary; a projection alone is
    // not permission to execute a candidate or skip a Power0 rejection.
    const auto& actor=r.game.units[req.unit_slot];
    if(actor.force_type==1u && (actor.ai.policy_flags&0x40u)==0u)
        return AiCandidatePreviewStatus::UnsupportedSpecialIndication;
    return ProjectSharedOrdinaryBattleCandidate(r,req,out,nullptr);
}
} // namespace
AiCandidatePreviewProvider MakeSharedOrdinaryBattlePreviewProvider(
    const bool retail_enumeration_order_exact) noexcept {
    return {&BuildSharedOrdinaryPreview,nullptr,retail_enumeration_order_exact};
}
} // namespace fates::ai::native
