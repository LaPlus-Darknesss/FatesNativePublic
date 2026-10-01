#include "fates/runtime/native_current_item_eligibility.hpp"
#include "fates/battle/native_battle_conditions.hpp"
#include <algorithm>
namespace fates::runtime::native {
namespace {
std::uint64_t Flags(const std::array<std::uint8_t,8>& bytes) noexcept {
    std::uint64_t out=0;for(unsigned i=0;i<8;++i)out|=std::uint64_t(bytes[i])<<(8*i);return out;
}
}
std::uint8_t CurrentWeaponExpLimitExact(std::uint8_t group,std::uint8_t limit,
    std::uint64_t flags,std::uint8_t threshold) noexcept {
    if(group==7&&(flags&(1ull<<34)))return 0;
    if(group==6&&(flags&(1ull<<42)))return std::max(limit,threshold);
    return limit;
}
bool JobCanEquipSubKindExact(std::int8_t subkind,std::int8_t group,
    std::uint16_t category,std::uint8_t origin,std::uint8_t limit) noexcept {
    if(group!=7)return limit!=0;
    switch(subkind) {
    case 14:return (category&0x40u)&&!(category&0x400u);
    case 15:return (category&0x80u)!=0;
    case 16:return (category&0x40u)&&(category&0x400u);
    case 17:return (category&0x10u)&&!(category&0x20u);
    case 18:return (category&0x20u)&&origin==2;
    case 19:return (category&0x20u)&&origin==1;
    default:return false;
    }
}
bool IsEquippedSkillExact(std::int16_t query,std::uint16_t personal,
    const std::array<std::uint16_t,5>& equipped) noexcept {
    if(query==0)return false;
    const auto id=std::uint16_t(query);
    return id==personal||std::find(equipped.begin(),equipped.end(),id)!=equipped.end();
}
CurrentItemEligibility CanEquipCurrentItemExact(const CurrentEquipmentUnit& u,
    const CurrentEquipmentItem& item,bool staff,bool current,CurrentEquipmentServices& svc) {
    using E=CurrentItemEligibility;
    if(!staff&&item.group==6)return E::No;
    if(item.group>=8)return E::No;
    if(item.group<0)return E::InvalidDefinition; // would index outside original WEXP records
    const auto group=std::uint8_t(item.group);const auto flags=u.private_flags|u.person_flags|u.job_flags;
    const auto limit=CurrentWeaponExpLimitExact(group,u.job_limits[group],flags,u.staff_threshold);
    if((current?std::min(limit,u.weapon_exp[group]):limit)<item.required_exp)return E::No;
    if(group==7&&!JobCanEquipSubKindExact(item.subkind,item.group,u.job_category,u.job_origin,u.job_limits[group]))return E::No;
    auto personal=[&](unsigned item_bit,unsigned unit_bit)->E {
        if(!(item.flags&(1ull<<item_bit)))return E::Yes;
        if(!(flags&(1ull<<unit_bit)))return E::No;
        if(u.public_flags&0x10000000u) {
            const auto download=svc.PersonIsDownload();if(!download)return E::UnresolvedDownload;
            if(!*download)return E::No;
        }return E::Yes;
    };
    auto result=personal(18,2);if(result!=E::Yes)return result;
    if(item.flags&(1ull<<19)) {
        const auto category=fates::battle::native::ResolveUnitCategoryExact(u.job_category,u.private_flags,u.person_flags,u.job_flags);
        if(!(category&0x200u)) {
            const auto skill=svc.HasEquippedSkill(141);if(!skill)return E::UnresolvedSkill;
            if(!*skill)return E::No;
        }
    }
    const bool female=(flags&1u)!=0;
    if((item.flags&(1ull<<20))&&female)return E::No;
    if((item.flags&(1ull<<21))&&!female)return E::No;
    for(const auto bits:std::array<std::array<unsigned,2>,5>{{{47,30},{48,31},{49,32},{50,33},{63,48}}}) {
        result=personal(bits[0],bits[1]);if(result!=E::Yes)return result;
    }
    if((flags&(1ull<<45))&&group==6&&(item.use_kind==13||item.use_kind==14))return E::No;
    return E::Yes;
}
std::optional<bool> ProjectCurrentEquippedSkill(const PersonDefinition& p,const UnitState& u,
    std::int16_t query,std::optional<std::uint8_t> difficulty) noexcept {
    if(query==0)return false;
    std::array<std::uint16_t,5> equipped{};std::copy_n(u.equipped_skill_ids.begin(),5,equipped.begin());
    if(difficulty) {
        if(*difficulty>=p.personal_skills.size())return std::nullopt;
        return IsEquippedSkillExact(query,p.personal_skills[*difficulty],equipped);
    }
    // No guessed GameUser selector: prove only selector-independent answers.
    if(IsEquippedSkillExact(query,0,equipped))return true;
    const auto id=std::uint16_t(query);const auto& skills=p.personal_skills;
    if(std::all_of(skills.begin(),skills.end(),[&](auto v){return v==id;}))return true;
    if(std::none_of(skills.begin(),skills.end(),[&](auto v){return v==id;}))return false;
    return std::nullopt;
}
std::optional<bool> ProjectCurrentEquippedSkill(const NativeRuntime& r,const UnitState& u,std::int16_t query) noexcept {
    const auto* p=r.definitions.FindPerson(u.person_id);if(!u.occupied||!p)return std::nullopt;
    const auto& state=r.game.game_user_difficulty;
    return ProjectCurrentEquippedSkill(*p,u,query,state.bound?std::optional<std::uint8_t>{state.value}:std::nullopt);
}
CurrentItemEligibility ProjectCurrentItemEligibility(const NativeRuntime& r,const UnitState& u,
    const ItemDefinition& item,bool staff,bool current,const CurrentEquipmentContext& context) {
    auto resolved=context;if(r.game.game_user_difficulty.bound)resolved.difficulty=r.game.game_user_difficulty.value;
    return ProjectCurrentItemEligibility(r.definitions,u,item,staff,current,resolved);
}
std::array<std::uint8_t,8> ResolveCurrentWeaponExpLimits(const PersonDefinition& p,
    const JobDefinition& j,const UnitState& u,const std::array<std::uint8_t,6>& ranks) noexcept {
    auto out=j.max_weapon_exp;const auto flags=Flags(u.private_skill_bits)|Flags(p.bitflags)|Flags(j.bitflags);
    for(std::uint8_t group=0;group<8;++group)out[group]=CurrentWeaponExpLimitExact(group,out[group],flags,ranks[2]);return out;
}
CurrentItemEligibility ProjectCurrentItemEligibility(const DefinitionStore& defs,const UnitState& u,
    const ItemDefinition& item,bool staff,bool current,const CurrentEquipmentContext& context) {
    const auto* p=defs.FindPerson(u.person_id);const auto* j=defs.FindJob(u.job_id);
    const auto* sub=defs.FindItemSubKind(item.weapon_category);
    if(!u.occupied||!p||!j||!sub)return CurrentItemEligibility::InvalidDefinition;
    struct Services:CurrentEquipmentServices {
        const DefinitionStore& defs;const PersonDefinition& p;const UnitState& u;const CurrentEquipmentContext& context;
        Services(const DefinitionStore& defs,const PersonDefinition& p,const UnitState& u,const CurrentEquipmentContext& c):defs(defs),p(p),u(u),context(c){}
        std::optional<bool> HasEquippedSkill(std::int16_t id) override{return ProjectCurrentEquippedSkill(p,u,id,context.difficulty);}
        std::optional<bool> PersonIsDownload() override {
            if(const auto download=defs.IsPersonDownload(p))return download;
            return context.person_is_download;
        }
    } services(defs,*p,u,context);
    CurrentEquipmentUnit unit{Flags(u.private_skill_bits),Flags(p->bitflags),Flags(j->bitflags),u.flags,
        defs.BaseJobCategoryMask(*j),j->origin,defs.weapon_rank_thresholds()[2],j->max_weapon_exp,u.weapon_exp};
    CurrentEquipmentItem facts{Flags(item.bitflags),std::int8_t(item.weapon_category),std::int8_t(sub->weapon_exp_group),item.required_weapon_exp,item.non_weapon_category};
    return CanEquipCurrentItemExact(unit,facts,staff,current,services);
}
}
