#include "fates/support/native_local_support.hpp"
#include "fates/support/native_reliance.hpp"
#include "fates/runtime/native_current_item_eligibility.hpp"
#include "fates/battle/native_around_skills.hpp"
#include <algorithm>
#include <bit>
#include <cstdlib>
#include <set>
#include <utility>
namespace fates::support::native {
namespace rn=fates::runtime::native;
namespace {
std::int16_t Narrow(std::int32_t v) noexcept {return std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(v));}
}
bool CanDualExact(std::uint32_t s,std::uint32_t u,std::uint8_t c) noexcept {
    return (s&0x80000u)!=0 || (u&8u)==0 || c==1;
}
std::uint32_t RelianceScoreForDualExact(bool found,std::int32_t level,std::int32_t next) noexcept {
    return (std::uint32_t(level)<<8u)+(next?256u-std::uint32_t(next):0u)+(found?1u:0u);
}
std::int32_t PersonAttackStanceTotalExact(const std::array<std::int8_t,20>& table,
                                       std::uint8_t capability,std::int32_t level) noexcept {
    if(capability>=4 || level>4)return 0; // invalid public index cannot read beyond definition
    std::int32_t total=0;
    for(std::int32_t i=0;i<=level;++i)total+=table[std::size_t(i)*4+capability];
    return total;
}
void AddPrimarySupportBonuses(SupportBonuses& out,const std::array<std::int16_t,4>& v) noexcept {
    out.hit=Narrow(std::int32_t(out.hit)+v[0]);out.critical=Narrow(std::int32_t(out.critical)+v[1]);
    out.avoid=Narrow(std::int32_t(out.avoid)+v[2]);out.dodge=Narrow(std::int32_t(out.dodge)+v[3]);
}
void AddExtraSupportBonuses(SupportBonuses& out,const std::array<std::int16_t,4>& v) noexcept {
    out.hit=Narrow(std::int32_t(out.hit)+(std::int32_t(v[0])*2)/5);
    out.critical=Narrow(std::int32_t(out.critical)+std::int32_t(v[1])/3);
    out.avoid=Narrow(std::int32_t(out.avoid)+(std::int32_t(v[2])*2)/5);
    out.dodge=Narrow(std::int32_t(out.dodge)+(std::int32_t(v[3])*2)/5);
}
std::vector<std::uint16_t> EnumerateAdjacentSupportExact(std::uint8_t force,int x,int y,
    int min_x,int min_y,int max_x,int max_y,std::span<const SupportImageUnit> image,bool filtered) {
    std::vector<std::uint16_t> result;
    const int x0=std::max(x-1,min_x),x1=std::min(x+1,max_x-1);
    const int y0=std::max(y-1,min_y),y1=std::min(y+1,max_y-1);
    for(int cy=y0;cy<=y1;++cy)for(int cx=x0;cx<=x1;++cx) {
        if(std::abs(cx-x)+std::abs(cy-y)!=1)continue;
        for(const auto& u:image)if(u.x==cx && u.y==cy) {
            if(u.force==force && !u.no_dual && (!filtered || !(u.flags&0x400000u)))result.push_back(u.slot);
            break;
        }
    }
    return result;
}
std::int32_t SelectSupportCandidateExact(std::span<const SupportCandidate> list) noexcept {
    std::int32_t best=-1;
    for(std::size_t i=0;i<list.size();++i)
        if(best<0 || list[i].score>list[std::size_t(best)].score ||
           (list[i].score==list[std::size_t(best)].score && list[i].tie_key>list[std::size_t(best)].tie_key))best=std::int32_t(i);
    return best;
}
std::int32_t DualScoreExact(bool selected,bool weapon,bool reliance,std::int32_t level) noexcept {
    if(!selected)return 0;
    if(!weapon)return 1;
    if(!reliance)return 2;
    return level+3;
}
namespace {
bool Identity(const rn::NativeRuntime& r,std::uint16_t slot,std::uint16_t p,std::uint16_t j) {
    return slot<r.game.units.size() && r.game.units[slot].occupied &&
        r.game.units[slot].person_id==p && r.game.units[slot].job_id==j;
}
bool OnImage(const rn::UnitState& u) {
    return u.occupied && u.has_position && !u.defeated && u.force_type<3 && u.pair.role!=rn::PairRole::Partner;
}
bool NoDual(const rn::NativeRuntime& r,std::uint16_t s,bool& value) {
    const auto& u=r.game.units[s];const auto* p=r.definitions.FindPerson(u.person_id);const auto* j=r.definitions.FindJob(u.job_id);
    if(!p||!j)return false;
    value=((u.private_skill_bits[4]|p->bitflags[4]|j->bitflags[4])&0x10u)!=0;
    return true;
}
LocalSupportStatus Context(const rn::NativeRuntime& r) {
    const auto& c=r.game.support_context;if(!c.bound)return LocalSupportStatus::MissingContext;
    if(!OrdinarySupportSourceMatches(r))return LocalSupportStatus::StaleContext;
    if(c.phase_revision!=r.game.phase.revision || c.chapter_index!=r.game.campaign.current_chapter_index ||
       c.user_flags!=r.game.campaign.game_user_flags ||
       (r.game.phase.stage!=rn::PhaseAccessStage::Unbound && c.control!=r.game.phase.situation.control))return LocalSupportStatus::StaleContext;
    return LocalSupportStatus::Ok;
}
const rn::ResolvedSupportRelationInput* Relation(const rn::NativeRuntime& r,std::uint16_t a,std::uint16_t b) {
    for(const auto& v:r.game.support_context.relations)
        if(v.subject==a && v.source==b && r.game.units[a].person_id==v.subject_person && r.game.units[b].person_id==v.source_person)return &v;
    return nullptr;
}
const rn::ResolvedSupportUnitInput* UnitInput(const rn::NativeRuntime& r,std::uint16_t a) {
    for(const auto& v:r.game.support_context.units)if(v.slot==a && Identity(r,a,v.person_id,v.job_id))return &v;
    return nullptr;
}
LocalSupportStatus ImageNeighbors(const rn::NativeRuntime& r,std::uint16_t subject,int x,int y,
    std::uint16_t removed,bool enum_filter,std::vector<std::uint16_t>& result) {
    const auto* map=r.definitions.terrain_map();if(!map)return LocalSupportStatus::MissingDefinition;
    // Same 32-cell semantic map backing as the existing native movement owner.
    std::array<std::int16_t,1024> image;image.fill(-1);
    for(std::uint16_t s=0;s<r.game.units.size();++s) {
        const auto& u=r.game.units[s];if(s==removed || !OnImage(u))continue;
        if(u.x<0 || u.y<0 || u.x>=32 || u.y>=32)return LocalSupportStatus::InvalidContext;
        auto& cell=image[std::size_t(u.y*32+u.x)];if(cell>=0)return LocalSupportStatus::AmbiguousImage;cell=std::int16_t(s);
    }
    std::vector<SupportImageUnit> entries;
    for(std::size_t at=0;at<image.size();++at)if(image[at]>=0) {
        const auto slot=std::uint16_t(image[at]);const auto& u=r.game.units[slot];
        // Do not demand definitions for distant, nonparticipating occupants.
        if(std::abs(int(u.x)-x)+std::abs(int(u.y)-y)!=1 || u.force_type!=r.game.units[subject].force_type)continue;
        bool blocked=false;if(!NoDual(r,slot,blocked))return LocalSupportStatus::MissingDefinition;
        entries.push_back({slot,u.x,u.y,u.force_type,blocked,u.flags});
    }
    result=EnumerateAdjacentSupportExact(r.game.units[subject].force_type,x,y,
        int(map->min_x),int(map->min_y),int(map->max_x),int(map->max_y),entries,enum_filter);
    return LocalSupportStatus::Ok;
}
bool ValidPair(const rn::NativeRuntime& r,std::uint16_t slot) {
    const auto& u=r.game.units[slot];
    if(!u.pair.bound)return u.pair.role==rn::PairRole::None;
    if(u.pair.partner_slot>=r.game.units.size() || u.pair.partner_slot==slot ||
       (u.pair.role!=rn::PairRole::Lead && u.pair.role!=rn::PairRole::Partner))return false;
    const auto& p=r.game.units[u.pair.partner_slot];
    return p.occupied && p.force_type==u.force_type && p.pair.bound &&
        p.pair.partner_slot==slot &&
        p.pair.role==(u.pair.role==rn::PairRole::Lead?rn::PairRole::Partner:rn::PairRole::Lead);
}
LocalSupportStatus Armed(const rn::NativeRuntime& r,std::uint16_t slot,bool& result,bool battle_pairs=false) {
    const auto& u=r.game.units[slot];
    if(!battle_pairs && (u.pair.bound || u.pair.role!=rn::PairRole::None))return LocalSupportStatus::UnsupportedPartner;
    if(battle_pairs && !ValidPair(r,slot))return LocalSupportStatus::InvalidPairTopology;
    result=u.equipped_item_id!=0;if(!result)return LocalSupportStatus::Ok;
    const auto* p=r.definitions.FindPerson(u.person_id);const auto* j=r.definitions.FindJob(u.job_id);const auto* i=r.definitions.FindItem(u.equipped_item_id);
    if(!p || !j || !i)return LocalSupportStatus::InvalidEquipment;
    const auto group=r.definitions.WeaponExpGroupForItem(*i);if(group>=8 || group==6)return LocalSupportStatus::InvalidEquipment;
    if(rn::ProjectCurrentItemEligibility(r,u,*i,false,true)!=rn::CurrentItemEligibility::Yes)return LocalSupportStatus::InvalidEquipment;
    return LocalSupportStatus::Ok;
}
}
LocalSupportStatus RestoreResolvedSupportContext(rn::NativeRuntime& r,const rn::ResolvedSupportContext& input) {
    if(input.user_flags!=r.game.campaign.game_user_flags ||
       (r.game.phase.stage!=rn::PhaseAccessStage::Unbound && input.control!=r.game.phase.situation.control))return LocalSupportStatus::InvalidContext;
    std::set<std::uint16_t> units;std::set<std::pair<std::uint16_t,std::uint16_t>> pairs;
    for(const auto& u:input.units)if(!Identity(r,u.slot,u.person_id,u.job_id)||!units.insert(u.slot).second)return LocalSupportStatus::InvalidContext;
    for(const auto& v:input.relations) {
        if(v.subject>=r.game.units.size() || v.source>=r.game.units.size() || v.subject==v.source ||
           !r.game.units[v.subject].occupied || !r.game.units[v.source].occupied ||
           r.game.units[v.subject].person_id!=v.subject_person || r.game.units[v.source].person_id!=v.source_person ||
           v.level<0 || v.level>4 || v.points_to_next<0 || v.points_to_next>256 ||
           !pairs.insert({v.subject,v.source}).second)return LocalSupportStatus::InvalidContext;
    }
    auto next=input;next.bound=true;next.phase_revision=r.game.phase.revision;next.chapter_index=r.game.campaign.current_chapter_index;
    r.game.support_context=std::move(next);return LocalSupportStatus::Ok;
}
namespace {
LocalSupportSelection InspectSelection(const rn::NativeRuntime& r,std::uint16_t subject,
    std::int16_t x,std::int16_t y,std::uint16_t removed,bool battle_pairs) {
    LocalSupportSelection out{};auto finish=[&](LocalSupportStatus s){out.status=s;return out;};
    if(subject>=r.game.units.size() || !r.game.units[subject].occupied ||
       removed>=r.game.units.size() || !r.game.units[removed].occupied)return finish(LocalSupportStatus::InvalidContext);
    const auto& u=r.game.units[subject];if(u.force_type>=3)return finish(LocalSupportStatus::Ok);
    if(!battle_pairs && (u.pair.bound || u.pair.role!=rn::PairRole::None))return finish(LocalSupportStatus::UnsupportedPartner);
    if(battle_pairs && !ValidPair(r,subject))return finish(LocalSupportStatus::InvalidPairTopology);
    // The enumerator returns the actual pair before NoDual/CanDual/public gates.
    if(battle_pairs && u.pair.bound) {
        out.selected=u.pair.partner_slot;out.candidates.push_back(out.selected);
        auto status=Context(r);if(status!=LocalSupportStatus::Ok)return finish(status);
        const auto* rel=Relation(r,subject,out.selected);if(!rel)return finish(LocalSupportStatus::MissingRelationship);
        bool weapon=false;status=Armed(r,out.selected,weapon,true);if(status!=LocalSupportStatus::Ok)return finish(status);
        out.reliance_level=rel->level;out.dual_score=DualScoreExact(true,weapon,rel->is_reliance,rel->level);
        return finish(LocalSupportStatus::Ok);
    }
    bool blocked=false;if(!NoDual(r,subject,blocked))return finish(LocalSupportStatus::MissingDefinition);
    if(blocked || (u.flags&0x400000u))return finish(LocalSupportStatus::Ok);
    const auto& c=r.game.support_context;
    if(c.bound) {
        auto s=Context(r);if(s!=LocalSupportStatus::Ok)return finish(s);
        if(!CanDualExact(c.situation_flags,c.user_flags,c.control[u.force_type]))return finish(LocalSupportStatus::Ok);
    }
    auto status=ImageNeighbors(r,subject,x,y,removed,true,out.candidates);if(status!=LocalSupportStatus::Ok)return finish(status);
    if(out.candidates.empty())return finish(LocalSupportStatus::Ok); // zero under either CanDual outcome
    out.rejected_source=out.candidates.front();status=Context(r);if(status!=LocalSupportStatus::Ok)return finish(status);
    std::int32_t selected=-1;std::uint32_t best=0;
    for(std::size_t k=0;k<out.candidates.size();++k) {
        const auto slot=out.candidates[k];out.rejected_source=slot;
        const auto* rel=Relation(r,subject,slot);if(!rel)return finish(LocalSupportStatus::MissingRelationship);
        const auto score=RelianceScoreForDualExact(rel->score_relation_found,rel->level,rel->points_to_next);
        bool replace=selected<0 || score>best;
        if(selected>=0 && score==best) {
            const auto* a=UnitInput(r,slot);const auto* b=UnitInput(r,out.candidates[std::size_t(selected)]);
            if(!a||!b||!a->tie_key_known||!b->tie_key_known)return finish(LocalSupportStatus::MissingTieKey);
            replace=a->constructor_tie_key>b->constructor_tie_key;
        }
        if(replace){selected=std::int32_t(k);best=score;}
    }
    out.selected=out.candidates[std::size_t(selected)];bool weapon=false;
    status=Armed(r,out.selected,weapon,battle_pairs);if(status!=LocalSupportStatus::Ok)return finish(status);
    const auto* rel=Relation(r,subject,out.selected);
    out.reliance_level=rel->level;out.dual_score=DualScoreExact(true,weapon,rel->is_reliance,rel->level);out.rejected_source=kNoSupportUnit;
    return finish(LocalSupportStatus::Ok);
}
LocalSupportProjection ProjectChosenSupportBonuses(const rn::NativeRuntime& r,std::uint16_t subject,
    std::int16_t x,std::int16_t y,std::uint16_t removed,LocalSupportSelection selection,bool battle_pairs) {
    LocalSupportProjection out{};out.selection=std::move(selection);
    auto finish=[&](LocalSupportStatus s){out.status=s;return out;};
    if(out.selection.status!=LocalSupportStatus::Ok){out.rejected_source=out.selection.rejected_source;return finish(out.selection.status);}
    if(out.selection.selected==kNoSupportUnit)return finish(LocalSupportStatus::Ok);
    out.selected_has_weapon=r.game.units[out.selection.selected].equipped_item_id!=0;
    auto apply=[&](std::uint16_t source,bool extra) {
        out.rejected_source=source;
        const auto& current=r.game.units[source];
        if(!battle_pairs && (current.pair.bound || current.pair.role!=rn::PairRole::None))return LocalSupportStatus::UnsupportedPartner;
        if(battle_pairs && !ValidPair(r,source))return LocalSupportStatus::InvalidPairTopology;
        const auto* person=r.definitions.FindPerson(current.person_id);
        if(!person)return LocalSupportStatus::MissingDefinition;
        const auto skill_ok=[](std::uint16_t id){return id==0 || fates::battle::native::IsSupportedLocalAroundSkill(id);};
        // GetDualSupport reads Person/Family/Edit totals, not the equipped or
        // personal skill set. Preserve the older ordinary admission boundary;
        // explicit battle calculation validates its actual skill owners later.
        if(!battle_pairs) {
            for(const auto id:current.equipped_skill_ids)if(!skill_ok(id))return LocalSupportStatus::UnsupportedSourceSkill;
            for(const auto id:person->personal_skills)if(!skill_ok(id))return LocalSupportStatus::UnsupportedSourceSkill;
        }
        const auto* rel=Relation(r,subject,source);if(!rel)return LocalSupportStatus::MissingRelationship;
        const auto* unit=UnitInput(r,source);if(!unit||!unit->bonus_totals_known)return LocalSupportStatus::MissingBonusTotals;
        const auto& v=unit->bonus_totals[std::size_t(rel->level)];
        if(extra)AddExtraSupportBonuses(out.bonuses,v);else AddPrimarySupportBonuses(out.bonuses,v);
        return LocalSupportStatus::Ok;
    };
    auto status=LocalSupportStatus::Ok;
    const auto& primary=r.game.units[subject];
    if(battle_pairs && primary.pair.bound && primary.pair.partner_slot==out.selection.selected)
        AddPrimarySupportBonuses(out.bonuses,{0,0,0,5});
    else {status=apply(out.selection.selected,false);if(status!=LocalSupportStatus::Ok)return finish(status);}
    // Calculate's secondary scan is only reached for a nonnull selected source.
    // Unlike the enumerator, this scan does not exclude public flag 0x400000.
    const auto* map=r.definitions.terrain_map();if(!map)return finish(LocalSupportStatus::MissingDefinition);
    if(x>=0 && y>=0 && x<int(map->width) && y<int(map->height)) {
        bool blocked=false;if(!NoDual(r,subject,blocked))return finish(LocalSupportStatus::MissingDefinition);
        if(blocked){out.rejected_source=kNoSupportUnit;return finish(LocalSupportStatus::Ok);}
        std::vector<std::uint16_t> extras;status=ImageNeighbors(r,subject,x,y,removed,false,extras);
        if(status!=LocalSupportStatus::Ok)return finish(status);
        for(const auto source:extras)if(source!=subject && source!=out.selection.selected) {
            status=apply(source,true);if(status!=LocalSupportStatus::Ok)return finish(status);out.secondary_sources.push_back(source);
        }
    }
    out.rejected_source=kNoSupportUnit;return finish(LocalSupportStatus::Ok);
}
} // namespace
LocalSupportSelection InspectLocalSupportSelection(const rn::NativeRuntime& r,std::uint16_t subject,
    std::int16_t x,std::int16_t y,std::uint16_t removed) {
    return InspectSelection(r,subject,x,y,removed,false);
}
LocalSupportProjection ProjectLocalSupportBonuses(const rn::NativeRuntime& r,std::uint16_t subject,
    std::int16_t x,std::int16_t y,std::uint16_t removed) {
    return ProjectChosenSupportBonuses(r,subject,x,y,removed,InspectSelection(r,subject,x,y,removed,false),false);
}
LocalSupportProjection ProjectBattleSupportBonuses(const rn::NativeRuntime& r,std::uint16_t subject,
    std::int16_t x,std::int16_t y,std::uint16_t removed) {
    return ProjectChosenSupportBonuses(r,subject,x,y,removed,InspectSelection(r,subject,x,y,removed,true),true);
}
LocalSupportProjection ProjectSpecifiedSupportBonuses(const rn::NativeRuntime& r,std::uint16_t subject,
    std::uint16_t source,std::int16_t x,std::int16_t y,std::uint16_t removed) {
    LocalSupportProjection out{};
    auto fail=[&](LocalSupportStatus status){out.status=out.selection.status=status;return out;};
    if(subject>=r.game.units.size() || source>=r.game.units.size() || removed>=r.game.units.size() ||
       subject==source || !r.game.units[subject].occupied || !r.game.units[source].occupied ||
       !r.game.units[removed].occupied)return fail(LocalSupportStatus::InvalidContext);
    if(!ValidPair(r,subject)||!ValidPair(r,source))return fail(LocalSupportStatus::InvalidPairTopology);
    const auto status=Context(r);if(status!=LocalSupportStatus::Ok)return fail(status);
    const auto* rel=Relation(r,subject,source);if(!rel)return fail(LocalSupportStatus::MissingRelationship);
    out.selection.status=LocalSupportStatus::Ok;out.selection.selected=source;
    out.selection.reliance_level=rel->level;
    // No constructor key, CanDual, adjacency or source-selection score is needed.
    return ProjectChosenSupportBonuses(r,subject,x,y,removed,std::move(out.selection),true);
}
}
