#include "fates/support/native_reliance.hpp"
#include "fates/runtime/native_unit_capabilities.hpp"
namespace fates::support::native {
namespace rn=fates::runtime::native;
namespace {
constexpr std::uint32_t kUniquenessFlags=0x10040000u;
bool UniqueOrdinary(const rn::UnitState& u) { return (u.flags&kUniquenessFlags)==0; }
unsigned SameForceMask(unsigned force) {
    // Force::GetMaskSameForce includes player reserves in the player domain.
    switch(force) { case 0: case 3: case 4:return 0x19; case 1:return 2; case 2:return 4; default:return 0; }
}
}
int RelianceLevelExact(const std::array<std::uint8_t,4>& t,int points) noexcept {
    for(int level=4;level>0;--level)if(points>=t[std::size_t(level-1)])return level;
    return 0;
}
int RelianceMaxLevelExact(const std::array<std::uint8_t,4>& t) noexcept {
    for(int level=4;level>0;--level)if(t[std::size_t(level-1)]<99)return level;
    return 0;
}
OrdinaryRelianceScore ResolveOrdinaryRelianceExact(bool eligible,const rn::RelianceDefinition* record,std::uint8_t points) noexcept {
    if(!eligible || !record)return {};
    OrdinaryRelianceScore out{true,RelianceLevelExact(record->thresholds,points),points,0};
    if(out.level<RelianceMaxLevelExact(record->thresholds))out.points_to_next=int(record->thresholds[std::size_t(out.level)])-points;
    return out;
}
CarriedSupportRelation ResolveCarriedRelianceExact(bool eligible,std::uint16_t subject,std::uint16_t other,
    const SupportFamilyState* a,const SupportFamilyState* b,const rn::RelianceDefinition* record,std::uint8_t points) noexcept {
    const auto family=ResolveFamilySupportExact(eligible,subject,other,a,b);
    auto score=ResolveOrdinaryRelianceExact(eligible,record,points);
    bool is_reliance=score.found;
    if(family.kind!=FamilyRelationshipKind::None) {
        const int level=FamilySupportLevelExact(family.points);
        constexpr std::array<int,3> thresholds{1,5,10};
        score={true,level,family.points,level<3?thresholds[std::size_t(level)]-family.points:0};is_reliance=true;
    } else if(eligible && record && record->thresholds[3]<99 && IsUncleAuntSupportExact(subject,other,a,b)) {
        // Score lookup and support eligibility differ for uncle/aunt romance.
        is_reliance=false;
    }
    return {score,is_reliance};
}
bool OrdinarySupportSourceMatches(const rn::NativeRuntime& r) {
    const auto& c=r.game.support_context;
    if(!c.ordinary_source_bound)return true; // preserved explicit resolved-input API
    if(c.definition_revision!=r.definitions.support_revision())return false;
    std::size_t at=0;
    for(std::uint16_t slot=0;slot<r.game.units.size();++slot) {
        const auto& u=r.game.units[slot];if(!u.occupied)continue;
        if(at>=c.source_units.size())return false;
        const auto& g=c.source_units[at++];
        if(g.slot!=slot || g.person_id!=u.person_id || g.job_id!=u.job_id || g.force_type!=u.force_type ||
           g.uniqueness_flags!=(u.flags&kUniquenessFlags)||g.lineage_revision!=u.lineage.revision||g.transfer_revision!=u.transfer.revision||g.slot_generation!=r.game.unit_slot_generations[g.slot])return false;
    }
    return at==c.source_units.size();
}
SupportProviderStatus RestoreCarriedSupportSnapshot(rn::NativeRuntime& r,const CarriedSupportSnapshot& input) {
    if(input.user_flags!=r.game.campaign.game_user_flags ||
       (r.game.phase.stage!=rn::PhaseAccessStage::Unbound && input.control!=r.game.phase.situation.control))return SupportProviderStatus::InvalidContext;
    if(!r.definitions.support_tables_loaded())return SupportProviderStatus::MissingDefinition;
    std::array<const OrdinarySupportUnitState*,250> facts{};
    for(const auto& v:input.units) {
        if(v.slot>=facts.size() || facts[v.slot] || !r.game.units[v.slot].occupied ||
           r.game.units[v.slot].person_id!=v.person_id || r.game.units[v.slot].job_id!=v.job_id)return SupportProviderStatus::InvalidContext;
        if(v.points_storage>SupportStoragePresence::Present)return SupportProviderStatus::InvalidContext;
        if(v.points_storage!=SupportStoragePresence::Present && !v.points.empty())return SupportProviderStatus::InvalidContext;
        facts[v.slot]=&v;
    }
    std::array<rn::UnitLineageRestorePlan,250> lineage{};
    rn::ResolvedSupportContext next{};
    next.situation_flags=input.situation_flags;next.user_flags=input.user_flags;next.control=input.control;
    for(std::uint16_t slot=0;slot<facts.size();++slot) {
        const auto& u=r.game.units[slot];if(!u.occupied)continue;
        if(!facts[slot])return SupportProviderStatus::IncompleteUnitCensus;
        const auto& f=*facts[slot];
        if(f.family==SupportStoragePresence::Unknown)return SupportProviderStatus::MissingFamilyState;
        if(f.family>SupportStoragePresence::Present || (f.family==SupportStoragePresence::Present)!=f.family_state.has_value())return SupportProviderStatus::InvalidContext;
        if(f.edit==SupportStoragePresence::Unknown)return SupportProviderStatus::MissingEditState;
        if(f.edit>SupportStoragePresence::Present || (f.edit==SupportStoragePresence::Present)!=f.edit_state.has_value())return SupportProviderStatus::InvalidContext;
        // Person::IsDownload includes archive provenance not owned here. The
        // earlier clone rejection still resolves even if that flag is also set.
        if(!(u.flags&0x40000u) && (u.flags&0x10000000u))return SupportProviderStatus::UnsupportedDownloadIdentity;
        const auto* person=r.definitions.FindPerson(u.person_id);
        if(!person || !r.definitions.FindJob(u.job_id))return SupportProviderStatus::MissingDefinition;
        // Every supplied semantic reference must resolve before it can stand
        // for a retail Person pointer. No fallback from an ID to a nearby unit.
        if(f.family_state)for(const auto& parent:f.family_state->parents)
            for(const auto& id:{parent.person,parent.father,parent.mother})
                if(id && !r.definitions.FindPerson(*id))return SupportProviderStatus::MissingDefinition;
        rn::UnitLineageSnapshot carried{f.family_state,f.edit_state};
        // This provider supplies support facts, not a replacement Edit name.
        // Preserve unrelated carried data on the SAME currently present Edit.
        // A null Edit, new Person, or unknown prior state cannot inherit it.
        if(f.edit_state&&u.lineage.bound&&u.lineage.person_id==u.person_id&&u.lineage.value.edit) {
            carried.edit_name=u.lineage.value.edit_name;carried.edit_face=u.lineage.value.edit_face;
        }
        lineage[slot]=rn::PlanUnitLineageRestore(r.definitions,u,carried);
        if(lineage[slot].status==rn::UnitCapabilityStatus::MissingPersonality)return SupportProviderStatus::MissingPersonalityDefinition;
        if(lineage[slot].status==rn::UnitCapabilityStatus::RevisionExhausted)return SupportProviderStatus::RevisionExhausted;
        if(lineage[slot].status!=rn::UnitCapabilityStatus::Ok)return SupportProviderStatus::MissingDefinition;
        rn::ResolvedSupportUnitInput unit{};
        unit.slot=slot;unit.person_id=u.person_id;unit.job_id=u.job_id;
        unit.tie_key_known=f.constructor_key_known;unit.constructor_tie_key=f.constructor_key;unit.bonus_totals_known=true;
        const auto bonus=ResolveCarriedSupportBonuses(r.definitions,u.person_id,
            f.family_state?&*f.family_state:nullptr,f.edit_state?&*f.edit_state:nullptr,unit.bonus_totals);
        if(bonus==CarriedBonusStatus::MissingPerson)return SupportProviderStatus::MissingDefinition;
        if(bonus==CarriedBonusStatus::MissingTable)return SupportProviderStatus::MissingBonusTable;
        if(bonus==CarriedBonusStatus::MissingPersonality)return SupportProviderStatus::MissingPersonalityDefinition;
        next.units.push_back(unit);
        next.source_units.push_back({slot,u.person_id,u.job_id,u.force_type,u.flags&kUniquenessFlags,lineage[slot].state.revision,u.transfer.revision,r.game.unit_slot_generations[slot]});
    }
    for(const auto& a:next.units)for(const auto& b:next.units)if(a.slot!=b.slot) {
        const auto& subject=r.game.units[a.slot];const auto& source=r.game.units[b.slot];
        const auto* record=r.definitions.FindReliance(a.person_id,b.person_id);
        std::uint8_t points=0;const bool eligible=UniqueOrdinary(subject) && UniqueOrdinary(source);
        const auto* family_a=facts[a.slot]->family_state?&*facts[a.slot]->family_state:nullptr;
        const auto* family_b=facts[b.slot]->family_state?&*facts[b.slot]->family_state:nullptr;
        const auto family=ResolveFamilySupportExact(eligible,a.person_id,b.person_id,family_a,family_b);
        if(eligible && record && family.kind==FamilyRelationshipKind::None) {
            const auto* owner=facts[a.slot];
            if(record->tag[0]==0) {
                // UnitPool scans ascending slots; the nearby source need not
                // own the point array. A null lookup returns zero in retail.
                owner=nullptr;const auto mask=SameForceMask(subject.force_type);
                for(std::uint16_t i=0;i<facts.size();++i) {
                    const auto& candidate=r.game.units[i];
                    if(candidate.occupied && UniqueOrdinary(candidate) && candidate.force_type<32 &&
                       (mask&(1u<<candidate.force_type)) && candidate.person_id==record->character_id) {owner=facts[i];break;}
                }
            }
            if(owner) {
                if(owner->points_storage==SupportStoragePresence::Unknown)return SupportProviderStatus::MissingPointStorage;
                if(owner->points_storage==SupportStoragePresence::Present) {
                    if(record->id>=owner->points.size())return SupportProviderStatus::PointIndexOutOfRange;
                    points=owner->points[record->id];
                }
            }
        }
        const auto relation=ResolveCarriedRelianceExact(eligible,a.person_id,b.person_id,family_a,family_b,record,points);
        const auto& score=relation.score;
        next.relations.push_back({a.slot,b.slot,a.person_id,b.person_id,score.found,relation.is_reliance,score.level,score.points_to_next});
    }
    next.ordinary_source_bound=true;next.definition_revision=r.definitions.support_revision();
    if(RestoreResolvedSupportContext(r,next)!=LocalSupportStatus::Ok)return SupportProviderStatus::InvalidContext;
    // Publish the same Family/Edit inputs used above, only after every unit and
    // relation passes. No second persistent family/edit state is introduced.
    for(std::uint16_t slot=0;slot<facts.size();++slot)if(facts[slot]) {
        auto& u=r.game.units[slot];u.lineage=lineage[slot].state;
        if(lineage[slot].stat_inputs_changed)u.combat_state_valid=false;
    }
    return SupportProviderStatus::Ok;
}
SupportProviderStatus RestoreOrdinarySupportSnapshot(rn::NativeRuntime& r,const OrdinarySupportSnapshot& input) {
    for(const auto& unit:input.units) {
        if(unit.family==SupportStoragePresence::Present)return SupportProviderStatus::UnsupportedFamily;
        if(unit.edit==SupportStoragePresence::Present)return SupportProviderStatus::UnsupportedEdit;
    }
    return RestoreCarriedSupportSnapshot(r,input);
}
}
