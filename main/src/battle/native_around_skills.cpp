#include "fates/battle/native_around_skills.hpp"
#include "fates/runtime/native_force_order.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdlib>

namespace fates::battle::native {
namespace {
using namespace fates::runtime::native;
// All nineteen cached skill references in the local CalculateAround routine.
// Recognition is NOT implementation. Unowned potential effects refuse below.
constexpr std::array<std::uint16_t,19> kLocalSkills{
    35,72,75,73,74,76,89,77,78,192,186,173,167,187,224,197,202,227,135};
bool ActiveImageUnit(const UnitState& u) noexcept {
    return u.occupied && !u.defeated && u.has_position && u.force_type<3 &&
           u.pair.role!=PairRole::Partner;
}
}
bool IsSupportedLocalAroundSkill(const std::uint16_t id) noexcept {
    return id==kSkillMaleficAura || id==kSkillHeartseeker ||
           id==kSkillLilysPoise || id==kSkillMisfortune;
}
bool LocalAuraForcesAllied(const std::uint8_t a,const std::uint8_t b) noexcept {
    return fates::runtime::native::ForcesAlliedExact(a,b);
}
bool IsKnownLocalAroundSkill(const std::uint16_t id) noexcept {
    return std::find(kLocalSkills.begin(),kLocalSkills.end(),id)!=kLocalSkills.end();
}
void ApplyHostileAroundSkill(AroundSkillModifiers& out,const std::uint16_t skill,
                            const std::int32_t distance,const bool present) noexcept {
    if(skill==kSkillMaleficAura && distance<=2 && !out.malefic_applied && present) {
        out.malefic_applied=true;
        out.resistance_addend=std::bit_cast<std::int32_t>(
            std::bit_cast<std::uint32_t>(out.resistance_addend)-2u);
    }
    if(skill==kSkillMisfortune && distance<=2 && !out.misfortune_applied && present) {
        out.misfortune_applied=true;
        out.dodge_addend=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(out.dodge_addend)-15u));
    }
    if(skill==kSkillHeartseeker && distance<=1 && !out.heartseeker_applied && present) {
        out.heartseeker_applied=true;
        out.avoid_addend=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(out.avoid_addend)-20u));
    }
}
void ApplySelfMisfortune(AroundSkillModifiers& out,const bool present) noexcept {
    if(!present) return;
    out.misfortune_applied=true;
    out.dodge_addend=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(out.dodge_addend)-5u));
}
void ApplyAlliedAroundSkill(AroundSkillModifiers& out,const std::uint16_t skill,
                           const std::int32_t distance,const bool present) noexcept {
    if(skill==kSkillLilysPoise && distance<=1 && !out.lilys_poise_applied && present) {
        out.lilys_poise_applied=true;
        out.attack_addend=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(out.attack_addend)+1u);
        out.defense_resistance_addend=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(out.defense_resistance_addend)+3u);
    }
}
static AroundSkillProjection ProjectAroundSkills(const NativeRuntime& r,
    const std::uint16_t self_slot,const std::uint16_t opponent_slot,
    const std::int16_t self_x,const std::int16_t self_y,
    const std::int16_t opponent_x,const std::int16_t opponent_y,bool explicit_pairs) {
    AroundSkillProjection out{};
    if(self_slot>=r.game.units.size() || opponent_slot>=r.game.units.size() ||
       self_slot==opponent_slot || self_x<0 || self_y<0 || opponent_x<0 || opponent_y<0)
        return out;
    const auto& self=r.game.units[self_slot];
    const auto& opponent=r.game.units[opponent_slot];
    if(!self.occupied || !opponent.occupied || self.defeated || opponent.defeated ||
       self.force_type>=3 || opponent.force_type>=3 ||
       (!explicit_pairs && (self.pair.role==PairRole::Partner || opponent.pair.role==PairRole::Partner)))
        return out;
    if(explicit_pairs)for(const auto slot:{self_slot,opponent_slot}) {
        const auto& u=r.game.units[slot];
        if(!u.pair.bound){if(u.pair.role!=PairRole::None)return out;continue;}
        if(u.pair.partner_slot>=r.game.units.size() || u.pair.partner_slot==slot ||
           (u.pair.role!=PairRole::Lead && u.pair.role!=PairRole::Partner))return out;
        const auto& p=r.game.units[u.pair.partner_slot];
        if(!p.occupied || p.defeated || p.force_type!=u.force_type || !p.pair.bound ||
           p.pair.partner_slot!=slot || p.pair.role!=(u.pair.role==PairRole::Lead?PairRole::Partner:PairRole::Lead))return out;
    }
    const auto* map=r.definitions.terrain_map();
    if(!map || map->width>32 || map->height>32 || map->min_x>=map->max_x ||
       map->min_y>=map->max_y || map->max_x>map->width || map->max_y>map->height) {
        out.status=AroundProjectionStatus::MissingTerrain; return out;
    }
    if(self_x>=static_cast<int>(map->width) || self_y>=static_cast<int>(map->height) ||
       opponent_x>=static_cast<int>(map->width) || opponent_y>=static_cast<int>(map->height))
        return out;
    auto fail=[&](AroundProjectionStatus status,std::uint16_t source,std::uint16_t skill=0) {
        out.status=status;out.rejected_source=source;out.rejected_skill=skill;return false;
    };
    auto collect=[&](std::uint16_t slot,std::array<bool,19>& present)->bool {
        const auto& source=r.game.units[slot];
        for(const auto id:source.equipped_skill_ids) {
            const auto it=std::find(kLocalSkills.begin(),kLocalSkills.end(),id);
            if(it!=kLocalSkills.end()) present[static_cast<std::size_t>(it-kLocalSkills.begin())]=true;
        }
        // Native carried states already resolve their equipped slots. Personal
        // skill is a separate retail lookup; do not ignore a known local aura
        // just because it is not duplicated in that array.
        if(const auto* person=r.definitions.FindPerson(source.person_id)) {
            const auto& p=person->personal_skills;
            const bool any=std::any_of(p.begin(),p.end(),IsKnownLocalAroundSkill);
            if(any && !(p[0]==p[1] && p[1]==p[2]))
                return fail(AroundProjectionStatus::UnresolvedPersonalSkill,slot);
            if(any) {
                const auto it=std::find(kLocalSkills.begin(),kLocalSkills.end(),p[0]);
                present[static_cast<std::size_t>(it-kLocalSkills.begin())]=true;
            }
        } else if(source.person_id!=0) {
            return fail(AroundProjectionStatus::MissingSourceDefinition,slot);
        }
        return true;
    };
    std::array<bool,19> self_skills{};
    if(!collect(self_slot,self_skills)) return out;
    ApplySelfMisfortune(out.modifiers,self_skills[13]);
    auto visit=[&](std::uint16_t slot,int x,int y,bool explicit_opponent)->bool {
        const auto& source=r.game.units[slot];
        const int distance=std::abs(x-self_x)+std::abs(y-self_y);
        if(distance>2) return true;
        std::array<bool,19> present{};
        if(!collect(slot,present)) return false;
        const bool allied=LocalAuraForcesAllied(self.force_type,source.force_type);
        for(std::size_t i=0;i<present.size();++i) {
            const auto id=kLocalSkills[i];
            if(!present[i] || IsSupportedLocalAroundSkill(id)) continue;
            // Branch reachability only, not implementation of the unowned effect.
            const bool hostile_only=id==77 || id==78 || id==197;
            const bool independent=id==202 || id==227;
            const bool adjacent_only=id==35 || id==192 || id==135 || id==227;
            if(adjacent_only && distance>1) continue;
            if(!independent && hostile_only==allied) continue;
            return fail(AroundProjectionStatus::UnsupportedAroundSkill,slot,id);
        }
        auto has=[&](std::uint16_t id) {return present[static_cast<std::size_t>(
            std::find(kLocalSkills.begin(),kLocalSkills.end(),id)-kLocalSkills.begin())];};
        const auto before=out.modifiers;
        if(!allied) {
            ApplyHostileAroundSkill(out.modifiers,kSkillMaleficAura,distance,has(kSkillMaleficAura));
            ApplyHostileAroundSkill(out.modifiers,kSkillMisfortune,distance,has(kSkillMisfortune));
            ApplyHostileAroundSkill(out.modifiers,kSkillHeartseeker,distance,has(kSkillHeartseeker));
        } else {
            ApplyAlliedAroundSkill(out.modifiers,kSkillLilysPoise,distance,has(kSkillLilysPoise));
        }
        out.sources.push_back({slot,static_cast<std::int16_t>(x),static_cast<std::int16_t>(y),
            static_cast<std::uint8_t>(distance),explicit_opponent,
            !before.malefic_applied && out.modifiers.malefic_applied,
            !before.heartseeker_applied && out.modifiers.heartseeker_applied,
            !before.lilys_poise_applied && out.modifiers.lilys_poise_applied,
            !before.misfortune_applied && out.modifiers.misfortune_applied});
        return true;
    };
    // Calculate's subsequent force-wide hit provider (Skill 204) is outside
    // this local-aura subset. Refuse it, including off-diamond providers, rather
    // than claiming that a radius-two scan supplies the entire calculator.
    for(std::uint16_t slot=0;slot<r.game.units.size();++slot) {
        const auto& u=r.game.units[slot];
        if(slot==self_slot || !u.occupied || u.defeated || u.force_type>=3 ||
           !LocalAuraForcesAllied(self.force_type,u.force_type)) continue;
        bool present=std::find(u.equipped_skill_ids.begin(),u.equipped_skill_ids.end(),204)!=u.equipped_skill_ids.end();
        if(const auto* person=r.definitions.FindPerson(u.person_id))
            present=present || std::find(person->personal_skills.begin(),person->personal_skills.end(),204)!=person->personal_skills.end();
        if(present) {fail(AroundProjectionStatus::UnsupportedAroundSkill,slot,204);return out;}
    }
    if(!visit(opponent_slot,opponent_x,opponent_y,true)) return out;
    const int xmin=std::max<int>(map->min_x,self_x-2),xmax=std::min<int>(map->max_x-1,self_x+2);
    const int ymin=std::max<int>(map->min_y,self_y-2),ymax=std::min<int>(map->max_y-1,self_y+2);
    for(int y=ymin;y<=ymax;++y) for(int x=xmin;x<=xmax;++x) {
        const int distance=std::abs(x-self_x)+std::abs(y-self_y);
        if(distance<1 || distance>2) continue;
        std::uint16_t occupant=0xffffu;
        for(std::uint16_t slot=0;slot<r.game.units.size();++slot) {
            const auto& u=r.game.units[slot];
            if(slot==self_slot || slot==opponent_slot ||
               (opponent.pair.bound && slot==opponent.pair.partner_slot) ||
               !ActiveImageUnit(u) || u.x!=x || u.y!=y) continue;
            if(occupant!=0xffffu) { fail(AroundProjectionStatus::AmbiguousCell,slot);return out; }
            occupant=slot;
        }
        if(occupant!=0xffffu && !visit(occupant,x,y,false)) return out;
    }
    out.status=AroundProjectionStatus::Ok;
    return out;
}
AroundSkillProjection ProjectLocalAroundSkills(const NativeRuntime& r,
    std::uint16_t self,std::uint16_t opp,std::int16_t sx,std::int16_t sy,std::int16_t ox,std::int16_t oy) {
    return ProjectAroundSkills(r,self,opp,sx,sy,ox,oy,false);
}
AroundSkillProjection ProjectBattleAroundSkills(const NativeRuntime& r,
    std::uint16_t self,std::uint16_t opp,std::int16_t sx,std::int16_t sy,std::int16_t ox,std::int16_t oy) {
    return ProjectAroundSkills(r,self,opp,sx,sy,ox,oy,true);
}
}
