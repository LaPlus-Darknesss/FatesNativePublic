#include "fates/runtime/native_unit_capabilities.hpp"
#include "fates/runtime/native_runtime.hpp"
#include "fates/runtime/native_current_item_eligibility.hpp"
#include "fates/unit/native_unit_semantics.hpp"
#include <limits>
#include <algorithm>

namespace fates::runtime::native {
namespace {
using S=UnitCapabilityStatus;
namespace sn=fates::support::native;
namespace un=fates::unit::native;
bool RevisionAvailable(std::uint64_t revision) noexcept {return revision!=std::numeric_limits<std::uint64_t>::max();}
bool StatLineageEqual(const UnitLineageSnapshot& a,const UnitLineageSnapshot& b) noexcept {
    if(a.edit!=b.edit||a.family.has_value()!=b.family.has_value())return false;
    if(a.family)for(unsigned i=0;i<2;++i) {
        const auto& x=a.family->parents[i];const auto& y=b.family->parents[i];
        if(x.person!=y.person||x.father!=y.father||x.mother!=y.mother||x.edit!=y.edit)return false;
    }
    return true;
}
S ValidateEdit(const DefinitionStore& d,const sn::SupportEditState& edit) noexcept {
    return d.FindPersonality(edit.boon)&&d.FindPersonality(edit.bane)?S::Ok:S::MissingPersonality;
}
S CurrentInputs(const NativeRuntime& r,const UnitState& u) noexcept {
    if(!u.occupied)return S::InvalidUnit;
    if(!u.capabilities.bound)return S::UnboundCapability;
    if(u.capabilities.person_id!=u.person_id||u.capabilities.job_id!=u.job_id)return S::StaleCapability;
    if(!u.lineage.bound)return S::UnboundLineage;
    if(u.lineage.person_id!=u.person_id)return S::StaleLineage;
    if(!r.definitions.FindPerson(u.person_id)||!r.definitions.FindJob(u.job_id))return S::MissingDefinition;
    return S::Ok;
}
}
UnitLineageRestorePlan PlanUnitLineageRestore(const DefinitionStore& d,const UnitState& u,const UnitLineageSnapshot& input) noexcept {
    UnitLineageRestorePlan out{};out.state=u.lineage;
    auto fail=[&](S status){out.status=status;return out;};
    if(!u.occupied)return fail(S::InvalidUnit);
    if(!d.FindPerson(u.person_id))return fail(S::MissingDefinition);
    if(input.edit_face&&!input.edit)return fail(S::InvalidEditState);
    if(input.edit_name&&(!input.edit||std::find(input.edit_name->begin(),input.edit_name->end(),char16_t{})==input.edit_name->end()))
        return fail(S::InvalidEditState);
    if(input.edit&&ValidateEdit(d,*input.edit)!=S::Ok)return fail(S::MissingPersonality);
    if(input.family)for(const auto& parent:input.family->parents) {
        for(const auto& id:{parent.person,parent.father,parent.mother})if(id&&!d.FindPerson(*id))return fail(S::MissingDefinition);
        // An absent parent does not resolve its unused personality lanes.
        if(input.family->parents[0].person&&input.family->parents[1].person&&ValidateEdit(d,parent.edit)!=S::Ok)return fail(S::MissingPersonality);
    }
    out.changed=!u.lineage.bound||u.lineage.person_id!=u.person_id||u.lineage.value!=input;
    out.stat_inputs_changed=u.lineage.bound&&(u.lineage.person_id!=u.person_id||!StatLineageEqual(u.lineage.value,input));
    if(out.changed) {
        if(!RevisionAvailable(u.lineage.revision))return fail(S::RevisionExhausted);
        out.state={true,u.person_id,input,u.lineage.revision+1};
    }
    return out;
}
UnitCapabilityStatus RestoreUnitLineageSnapshot(NativeRuntime& r,std::uint16_t slot,const UnitLineageSnapshot& input) noexcept {
    if(slot>=r.game.units.size())return S::InvalidUnit;
    auto& u=r.game.units[slot];const auto plan=PlanUnitLineageRestore(r.definitions,u,input);
    if(plan.status!=S::Ok)return plan.status;
    u.lineage=plan.state;if(plan.stat_inputs_changed)u.combat_state_valid=false;
    return S::Ok;
}
UnitCapabilityStatus RestoreUnitCapabilitySnapshot(NativeRuntime& r,std::uint16_t slot,const UnitCapabilitySnapshot& input) noexcept {
    if(slot>=r.game.units.size()||!r.game.units[slot].occupied)return S::InvalidUnit;
    auto& u=r.game.units[slot];auto& state=u.capabilities;
    if(state.bound)return S::AlreadyBound;
    if(!RevisionAvailable(state.revision))return S::RevisionExhausted;
    if(!r.definitions.FindPerson(u.person_id)||!r.definitions.FindJob(u.job_id))return S::MissingDefinition;
    state={true,u.person_id,u.job_id,input,state.revision+1};u.combat_state_valid=false;
    return S::Ok;
}
UnitCapabilityStatus InvalidateUnitCapabilitySnapshot(NativeRuntime& r,std::uint16_t slot) noexcept {
    if(slot>=r.game.units.size()||!r.game.units[slot].occupied)return S::InvalidUnit;
    auto& u=r.game.units[slot];auto& state=u.capabilities;
    if(!RevisionAvailable(state.revision))return S::RevisionExhausted;
    state.bound=false;++state.revision;u.combat_state_valid=false;return S::Ok;
}
UnitCapabilityResult ProjectEditCapability(const DefinitionStore& d,const sn::SupportEditState& edit,std::uint8_t cap,bool limit) noexcept {
    if(cap>=8)return {S::InvalidCapability};
    const auto* boon=d.FindPersonality(edit.boon);const auto* bane=d.FindPersonality(edit.bane);
    if(!boon||!bane)return {S::MissingPersonality};
    return {S::Ok,limit?int(boon->boon_modifiers[cap])+bane->bane_modifiers[cap]:int(boon->boon_bases[cap])+bane->bane_bases[cap]};
}
UnitCapabilityResult ProjectFamilyCapabilityLimit(const DefinitionStore& d,const sn::SupportFamilyState& family,std::uint8_t cap) noexcept {
    if(cap>=8)return {S::InvalidCapability};
    if(!family.parents[0].person||!family.parents[1].person)return {};
    int result=cap==0?0:1;
    for(const auto& parent:family.parents) {
        const auto* person=d.FindPerson(*parent.person);if(!person)return {S::MissingDefinition};
        const auto edit=ProjectEditCapability(d,parent.edit,cap,true);if(edit.status!=S::Ok)return edit;
        result+=static_cast<std::int8_t>(person->modifiers[cap])+edit.value;
        if(parent.father&&parent.mother) {
            const auto* father=d.FindPerson(*parent.father);const auto* mother=d.FindPerson(*parent.mother);
            if(!father||!mother)return {S::MissingDefinition};
            result+=int(static_cast<std::int8_t>(father->modifiers[cap]))+static_cast<std::int8_t>(mother->modifiers[cap]);
        }
    }
    return {S::Ok,result};
}
UnitCapabilityResult ProjectCurrentCapabilityLimit(const NativeRuntime& r,const UnitState& u,std::uint8_t cap) noexcept {
    if(cap>=8)return {S::InvalidCapability};
    const auto status=CurrentInputs(r,u);if(status!=S::Ok)return {status};
    const auto& d=r.definitions;const auto* person=d.FindPerson(u.person_id);const auto* job=d.FindJob(u.job_id);
    int limit=int(static_cast<std::int8_t>(person->modifiers[cap]))+static_cast<std::int8_t>(job->max_stats[cap])+u.capabilities.value.limit_changes[cap];
    if(u.lineage.value.family) {
        const auto family=ProjectFamilyCapabilityLimit(d,*u.lineage.value.family,cap);if(family.status!=S::Ok)return family;limit+=family.value;
    }
    if(u.lineage.value.edit) {
        const auto edit=ProjectEditCapability(d,*u.lineage.value.edit,cap,true);if(edit.status!=S::Ok)return edit;limit+=edit.value;
    }
    return {S::Ok,un::ClampFinalCapability(limit)};
}
UnitCapabilityResult ProjectCurrentBaseCapability(const NativeRuntime& r,const UnitState& u,std::uint8_t cap) noexcept {
    const auto limit=ProjectCurrentCapabilityLimit(r,u,cap);if(limit.status!=S::Ok)return limit;
    int edit_base=0;
    if(u.lineage.value.edit) {
        const auto edit=ProjectEditCapability(r.definitions,*u.lineage.value.edit,cap,false);if(edit.status!=S::Ok)return edit;edit_base=edit.value;
    }
    const auto* person=r.definitions.FindPerson(u.person_id);const auto* job=r.definitions.FindJob(u.job_id);
    return {S::Ok,un::ResolveBaseCapability({static_cast<std::int8_t>(person->bases[cap]),static_cast<std::int8_t>(job->bases[cap]),u.capabilities.value.stored[cap],edit_base,(u.flags&0x40000000u)!=0,u.capabilities.value.penalties[cap],limit.value})};
}
UnitCapabilityResult ProjectCurrentMaximumHp(const NativeRuntime& r,const UnitState& u,bool effects,std::int32_t type) noexcept {
    const auto base=ProjectCurrentBaseCapability(r,u,0);if(base.status!=S::Ok)return base;
    un::MhpPostInputs post{};
    if(effects) {
        if(!u.enhance.bound)return {S::UnboundEnhance};
        post.halveBeforeAddends=(u.enhance.flags[0]&0x20u)!=0;
        // Verified original SEID_最大HP+5=1 and SEID_よく効く薬=99.
        const auto plus5=ProjectCurrentEquippedSkill(r,u,1);if(!plus5)return {S::UnresolvedSkill};post.maxHpPlus5Skill=*plus5;
        post.medicineStatus=type==8||(u.enhance.flags[1]&1u)!=0;
        if(post.medicineStatus) {
            const auto medicine=ProjectCurrentEquippedSkill(r,u,99);if(!medicine)return {S::UnresolvedSkill};post.medicineBoostSkill=*medicine;
        }
        post.statusPlus2=(u.enhance.flags[4]&4u)!=0;post.statusMinus1=(u.enhance.flags[4]&8u)!=0;
    }
    return {S::Ok,un::ResolveMhp(base.value,post)};
}
UnitCapabilityStatus RefreshUnitMaximumHp(NativeRuntime& r,std::uint16_t slot) noexcept {
    if(slot>=r.game.units.size()||!r.game.units[slot].occupied)return S::InvalidUnit;
    auto& u=r.game.units[slot];const auto hp=ProjectCurrentMaximumHp(r,u);if(hp.status!=S::Ok)return hp.status;
    if(!RevisionAvailable(u.capabilities.revision))return S::RevisionExhausted;
    u.max_hp=static_cast<std::int16_t>(hp.value);++u.capabilities.revision;u.combat_state_valid=false;
    return S::Ok;
}
}
