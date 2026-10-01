#include "fates/support/native_pair_bonus.hpp"
#include "fates/support/native_reliance.hpp"
#include "fates/runtime/native_current_item_eligibility.hpp"
#include <algorithm>
namespace fates::support::native {
namespace rn=fates::runtime::native;
int PersonGuardTotalExact(const GuardTable& t,int cap,int level) noexcept {
    if(cap<0 || cap>7 || level<0 || level>4)return 0;
    int sum=0;for(int row=0;row<=level;++row)sum+=t[std::size_t(row*8+cap)];return sum;
}
int PersonGuardEditExact(const GuardTable& t,int cap,int level,const GuardEdit& e) noexcept {
    if(!e.enabled || cap<0 || cap>7 || level<0 || level>4)return 0;
    int total=0;
    for(unsigned i=0;i<3;++i) {
        const auto boon=e.boon[i],bane=e.bane[i];
        if(cap && boon && bane && boon<8 && bane<8 && boon!=bane && (cap==boon || cap==bane)) {
            int moved=0;for(int row=1;row<=level;++row)moved+=t[std::size_t(row*8+bane)];
            total+=cap==bane?-moved:moved;
        }
        // This original branch is independent of the transfer guards, even
        // when the two indices coincide (or are zero in a direct helper call).
        if(i==0 && cap==boon && level>=4)++total;
    }
    return total;
}
int FamilyGuardTotalExact(const GuardTable& a,const GuardTable& b,int cap,int level,const GuardEdit& ae,const GuardEdit& be) noexcept {
    if(cap<0 || cap>7 || level<0 || level>4)return 0;
    int total=b[std::size_t(cap)];
    for(int row=1;row<=level;++row) {
        const auto& t=(row&1)?a:b;const auto& e=(row&1)?ae:be;
        total+=t[std::size_t(row*8+cap)]+PersonGuardEditExact(t,cap,row,e)-PersonGuardEditExact(t,cap,row-1,e);
    }
    return total;
}
namespace {
bool Edit(const rn::DefinitionStore& d,const SupportEditState& e,GuardEdit& out) {
    const auto* a=d.FindPersonality(e.boon);const auto* b=d.FindPersonality(e.bane);
    if(!a || !b)return false;
    for(const auto* p:{a,b})for(auto cap:p->guard_stance_capabilities)if(cap>=8)return false;
    out={a->guard_stance_capabilities,b->guard_stance_capabilities,true};return true;
}
}
CarriedBonusStatus ResolveCarriedGuardBonuses(const rn::DefinitionStore& d,std::uint16_t id,
    const SupportFamilyState* f,const SupportEditState* edit,std::array<std::array<std::int16_t,8>,5>& result) {
    const auto* own=d.FindPerson(id);if(!own)return CarriedBonusStatus::MissingPerson;
    const auto* fixed=own->parent?d.FindPerson(own->parent):nullptr;
    if(own->parent && !fixed)return CarriedBonusStatus::MissingPerson;
    const rn::PersonDefinition* first=fixed?fixed:own;const rn::PersonDefinition* second=nullptr;
    GuardEdit ae{},be{};
    if(fixed && f && f->parents[0].person && f->parents[1].person) {
        const rn::PersonDefinition* p[2]{};GuardEdit edits[2]{};
        for(unsigned i=0;i<2;++i) {
            const auto& parent=f->parents[i];p[i]=d.FindPerson(*parent.person);
            if(!p[i])return CarriedBonusStatus::MissingPerson;
            const auto* pf=p[i]->parent?d.FindPerson(p[i]->parent):nullptr;
            if(p[i]->parent && !pf)return CarriedBonusStatus::MissingPerson;
            const auto grand=i==0?parent.father:parent.mother;
            if(pf && grand) {p[i]=d.FindPerson(*grand);if(!p[i])return CarriedBonusStatus::MissingPerson;}
            else if(parent.edit.boon && parent.edit.bane && !Edit(d,parent.edit,edits[i]))return CarriedBonusStatus::MissingPersonality;
        }
        first=p[0];second=p[1];ae=edits[0];be=edits[1];
    } else if(!fixed && edit && !Edit(d,*edit,ae))return CarriedBonusStatus::MissingPersonality;
    if(!first->guard_stance_bonuses_present || (second && !second->guard_stance_bonuses_present))return CarriedBonusStatus::MissingTable;
    auto next=result;
    for(int rank=0;rank<5;++rank)for(int cap=0;cap<8;++cap)
        next[rank][cap]=std::int16_t(second?FamilyGuardTotalExact(first->guard_stance_increments,second->guard_stance_increments,cap,rank,ae,be):
            PersonGuardTotalExact(first->guard_stance_increments,cap,rank)+PersonGuardEditExact(first->guard_stance_increments,cap,rank,ae));
    result=next;return CarriedBonusStatus::Ok;
}
PairBonusProjection ProjectCurrentPairBonuses(const rn::NativeRuntime& r,std::uint16_t slot) {
    PairBonusProjection out{};auto fail=[&](PairBonusStatus s){out.status=s;return out;};
    if(slot>=r.game.units.size())return fail(PairBonusStatus::InvalidPair);
    const auto& a=r.game.units[slot];const auto source=a.pair.partner_slot;
    if(!a.occupied || a.defeated || a.force_type>=9 || !a.pair.bound || source>=r.game.units.size() || source==slot ||
       (a.pair.role!=rn::PairRole::Lead && a.pair.role!=rn::PairRole::Partner))return fail(PairBonusStatus::InvalidPair);
    const auto& b=r.game.units[source];
    if(!b.occupied || b.defeated || b.force_type!=a.force_type || !b.pair.bound || b.pair.partner_slot!=slot ||
       b.pair.role!=(a.pair.role==rn::PairRole::Lead?rn::PairRole::Partner:rn::PairRole::Lead))return fail(PairBonusStatus::InvalidPair);
    for(std::uint16_t i=0;i<r.game.units.size();++i)if(i!=slot && i!=source && r.game.units[i].occupied &&
        (r.game.units[i].pair.partner_slot==slot || r.game.units[i].pair.partner_slot==source))return fail(PairBonusStatus::InvalidPair);
    return ProjectCurrentGuardBonusesFromSource(r,slot,source);
}
PairBonusProjection ProjectCurrentGuardBonusesFromSource(const rn::NativeRuntime& r,std::uint16_t slot,std::uint16_t source) {
    PairBonusProjection out{};auto fail=[&](PairBonusStatus s){out.status=s;return out;};
    if(slot>=r.game.units.size()||source>=r.game.units.size()||slot==source)return fail(PairBonusStatus::InvalidPair);
    const auto& a=r.game.units[slot];const auto& b=r.game.units[source];
    if(!a.occupied||!b.occupied)return fail(PairBonusStatus::InvalidPair);
    const auto* job=r.definitions.FindJob(b.job_id);const auto* person=r.definitions.FindPerson(b.person_id);
    if(!job || !person || !r.definitions.FindPerson(a.person_id) || !r.definitions.FindJob(a.job_id))return fail(PairBonusStatus::MissingDefinition);
    const auto& c=r.game.support_context;
    if(!c.bound || !c.ordinary_source_bound)return fail(PairBonusStatus::MissingSupport);
    if(!OrdinarySupportSourceMatches(r) || c.phase_revision!=r.game.phase.revision || c.chapter_index!=r.game.campaign.current_chapter_index ||
       c.user_flags!=r.game.campaign.game_user_flags || (r.game.phase.stage!=rn::PhaseAccessStage::Unbound && c.control!=r.game.phase.situation.control))return fail(PairBonusStatus::StaleSupport);
    const rn::ResolvedSupportRelationInput* relation=nullptr;
    for(const auto& v:c.relations)if(v.subject==source && v.source==slot && v.subject_person==b.person_id && v.source_person==a.person_id) {
        if(relation)return fail(PairBonusStatus::MissingRelation);relation=&v;
    }
    if(!relation || relation->level<0 || relation->level>4)return fail(PairBonusStatus::MissingRelation);
    if(!b.lineage.bound || b.lineage.person_id!=b.person_id)return fail(PairBonusStatus::UnboundLineage);
    const int rank=relation->score_relation_found?relation->level:0;
    const auto& lineage=b.lineage.value;std::array<std::array<std::int16_t,8>,5> bonuses{};
    const auto status=ResolveCarriedGuardBonuses(r.definitions,b.person_id,lineage.family?&*lineage.family:nullptr,lineage.edit?&*lineage.edit:nullptr,bonuses);
    if(status==CarriedBonusStatus::MissingPerson)return fail(PairBonusStatus::MissingDefinition);
    if(status==CarriedBonusStatus::MissingTable)return fail(PairBonusStatus::MissingTable);
    if(status==CarriedBonusStatus::MissingPersonality)return fail(PairBonusStatus::MissingPersonality);
    const auto* guardian=r.definitions.FindSkill("SEID_\x8e\xe7\x82\xe8\x8e\xe8");
    if(!guardian || guardian->id>32767)return fail(PairBonusStatus::MissingDefinition);
    const auto equipped=rn::ProjectCurrentEquippedSkill(r,a,std::int16_t(guardian->id));
    if(!equipped)return fail(PairBonusStatus::UnresolvedSkill);
    for(unsigned i=1;i<8;++i)out.total[i]=std::int16_t(int(bonuses[rank][i])+std::int8_t(job->pair_up_bonuses[i])+int(*equipped));
    out.source_slot=source;out.support_level=rank;return out;
}
}
