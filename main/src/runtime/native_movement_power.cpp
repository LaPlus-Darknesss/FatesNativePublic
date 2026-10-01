#include "fates/runtime/native_movement_power.hpp"
#include "fates/runtime/native_runtime.hpp"
#include "fates/runtime/native_current_item_eligibility.hpp"
#include "fates/battle/native_battle_conditions.hpp"
#include "fates/event/typed_event_commands.hpp"
#include <limits>
namespace fates::runtime::native {
std::optional<int> MovementPowerExact(std::uint8_t base,std::uint32_t flags,bool enhanced,int type,
    std::uint16_t shooter,std::uint16_t flying,MovementPowerServices& s) {
    int value=base;
    if(!(flags&0x40000000)){const auto a=s.Adjustment();if(!a)return std::nullopt;value+=*a;}
    const auto penalty=s.Penalty();if(!penalty)return std::nullopt;if(*penalty>0)value-=*penalty;
    if(enhanced){
        if(type==30)++value;else{const auto flag=s.EnhanceFlag(3,0x40);if(!flag)return std::nullopt;if(*flag)++value;}
        if(flags&((flags&0x200000)?4u:2u)){const auto bonus=s.SelectedPartnerMovement();if(!bonus)return std::nullopt;value+=*bonus;}
        const auto plus=s.MovePlusOneSkill();if(!plus)return std::nullopt;if(*plus)++value;
        const auto category=s.Category();if(!category)return std::nullopt;
        if(*category&shooter){const auto skill=s.AmphibiousSkill();if(!skill)return std::nullopt;if(*skill)++value;}
        const auto a=s.EnhanceFlag(6,4);if(!a)return std::nullopt;
        if(*a){const auto c=s.Category();if(!c)return std::nullopt;value+=(*c&flying)?base/2:-int(base/2);}
        const auto b=s.EnhanceFlag(6,8);if(!b)return std::nullopt;
        if(*b){const auto c=s.Category();if(!c)return std::nullopt;value+=(*c&flying)?-int(base/2):base/2;}
        const auto stop=s.EnhanceFlag(0,2);if(!stop)return std::nullopt;if(*stop)return 0;
    }
    return value<0?0:value;
}
MovementPowerResult ProjectCurrentMovementPower(const NativeRuntime& r,const UnitState& u,bool enhanced,int type) {
    using S=MovementPowerStatus;if(!u.occupied)return {S::InvalidUnit};
    const auto* job=r.definitions.FindJob(u.job_id);if(!job)return {S::MissingDefinition};
    const auto& rules=r.definitions.movement_rules();if(enhanced&&!rules)return {S::MissingDefinition};
    struct Services:MovementPowerServices {
        const NativeRuntime& r;const UnitState& u;S error{S::Ok};
        Services(const NativeRuntime& r,const UnitState& u):r(r),u(u){}
        std::optional<std::int8_t> Adjustment() override {
            if(!u.movement.bound){error=S::UnboundAdjustment;return std::nullopt;}
            if(u.movement.person_id!=u.person_id){error=S::StaleAdjustment;return std::nullopt;}return u.movement.adjustment;
        }
        std::optional<std::int8_t> Penalty() override {
            if(!u.map_end.bound){error=S::UnboundPenalty;return std::nullopt;}return std::int8_t(u.map_end.fields.counters[0]);
        }
        std::optional<bool> EnhanceFlag(unsigned b,std::uint8_t mask) override {
            if(!u.enhance.bound){error=S::UnboundEnhance;return std::nullopt;}return (u.enhance.flags[b]&mask)!=0;
        }
        std::optional<int> SelectedPartnerMovement() override {
            const auto slot=u.pair.partner_slot;if(slot==0xffffu)return 0;
            if(slot>=r.game.units.size()){error=S::MalformedPair;return std::nullopt;}const auto& p=r.game.units[slot];
            if(!u.pair.bound||!p.occupied||!p.pair.bound||&p==&u||p.pair.partner_slot>=r.game.units.size()||&r.game.units[p.pair.partner_slot]!=&u){error=S::MalformedPair;return std::nullopt;}
            const auto* job=r.definitions.FindJob(p.job_id);if(!job){error=S::MissingDefinition;return std::nullopt;}return std::int8_t(job->pair_up_bonuses[0]);
        }
        std::optional<bool> Skill(const char* name){const auto* skill=r.definitions.FindSkill(name);if(!skill){error=S::MissingDefinition;return std::nullopt;}
            const auto value=ProjectCurrentEquippedSkill(r,u,std::int16_t(skill->id));if(!value)error=S::MissingSkill;return value;}
        std::optional<bool> MovePlusOneSkill() override {return Skill("SEID_\x88\xda\x93\xae+1");}
        std::optional<bool> AmphibiousSkill() override {return Skill("SEID_\x90\x85\x97\xa4\x97\xbc\x97\x70");}
        std::optional<std::uint16_t> Category() override {const auto value=fates::battle::native::ProjectUnitBattleCategory(r,u);if(!value)error=S::MissingCategory;return value;}
    } svc(r,u);
    const auto value=MovementPowerExact(job->movement,u.flags,enhanced,type,rules?rules->shooter_category:0,rules?rules->flying_category:0,svc);
    return value?MovementPowerResult{S::Ok,*value}:MovementPowerResult{svc.error};
}
MovementPowerStatus RestoreMovementAdjustment(NativeRuntime& r,std::uint16_t slot,std::int8_t value) {
    using S=MovementPowerStatus;if(slot>=r.game.units.size()||!r.game.units[slot].occupied)return S::InvalidUnit;
    auto& u=r.game.units[slot];if(u.movement.bound)return S::AlreadyBound;
    if(!r.definitions.FindPerson(u.person_id))return S::MissingDefinition;
    if(u.movement.revision==std::numeric_limits<std::uint64_t>::max())return S::RevisionExhausted;
    u.movement={true,u.person_id,value,u.movement.revision+1};return S::Ok;
}
MovementPowerStatus SetCurrentMovementPower(NativeRuntime& r,int key,int requested) {
    using S=MovementPowerStatus;if(key<=0||key>250)return S::Ignored;
    if(std::size_t(key)>r.game.units.size())return S::InvalidUnit;auto& u=r.game.units[key-1];
    if(u.force_type==9)return S::Ignored;
    if(!u.occupied||u.force_type>9)return S::InvalidUnit;
    const auto* job=r.definitions.FindJob(u.job_id);if(!job)return S::MissingDefinition;
    if(u.movement.revision==std::numeric_limits<std::uint64_t>::max())return S::RevisionExhausted;
    const auto value=fates::event::native::policy::EncodeMovePowerAdjustment(requested,job->movement);
    u.movement={true,u.person_id,value,u.movement.revision+1};return S::Ok;
}
}
