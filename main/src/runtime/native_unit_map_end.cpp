#include "fates/runtime/native_unit_map_end.hpp"
#include "fates/runtime/native_runtime.hpp"
#include <limits>

namespace fates::runtime::native {
namespace {
std::uint64_t Bits(const std::array<std::uint8_t,8>& bytes) noexcept {
    std::uint64_t value{};for(unsigned i=0;i<8;++i)value|=std::uint64_t(bytes[i])<<(8*i);return value;
}
bool Available(const UnitState& u) noexcept {return u.occupied&&u.force_type<9;}
bool RevisionAvailable(const UnitState& u) noexcept {return u.map_end_revision!=std::numeric_limits<std::uint64_t>::max();}
void StoreEnhance(UnitState& u,const UnitEnhanceSnapshot& s) noexcept {
    u.enhance.flags=s.flags;u.enhance.values=s.values;u.weakness=s.weakness;u.enhance.bound=true;
}
}
void ResetEnhanceConditionsExact(UnitEnhanceSnapshot& s) noexcept {s.flags[2]&=0x80u;}
void ApplyEnhanceDeathExact(UnitEnhanceSnapshot& s) noexcept {
    s.flags[4]&=0xfcu;s.flags[2]&=0x7fu;s.flags[3]=0;s.flags[0]&=1u;
    for(unsigned i=1;i<8;++i)s.weakness[i]=0;
}
void ClearEnhanceExact(UnitEnhanceSnapshot& s) noexcept {s={};}
void ResetUnitEndOfMapExact(UnitMapEndSnapshot& s,const UnitMapEndContext& c,
    bool position,bool enhance) noexcept {
    if(position) {
        s.position.fill(255);s.fields.previous_position.fill(255);
        constexpr auto defeat=std::uint64_t{1}<<5,escape=std::uint64_t{1}<<47;
        if((c.force==0||c.force>=3)&&c.person_resident&&!(s.flags&0x42000u)&&
           ((s.private_flags|c.person_private_flags|c.job_private_flags)&(defeat|escape)))s.private_flags&=~defeat;
    }
    s.fields.secondary_flags&=~0xa8u;s.flags&=0xf82481deu;
    if(enhance)ClearEnhanceExact(s.enhance);
    s.excluded_person_reference=0;s.excluded_forces=0;s.guard_progress=0;s.fields.counters.fill(0);
}
UnitEnhanceSnapshot CurrentUnitEnhanceSnapshot(const UnitState& u) noexcept {return {u.enhance.flags,u.enhance.values,u.weakness};}
UnitMapEndStatus RestoreUnitMapEndFields(UnitState& u,const UnitMapEndFields& s) noexcept {
    if(!Available(u))return UnitMapEndStatus::InvalidUnit;
    if(u.map_end.bound)return UnitMapEndStatus::AlreadyBound;
    if(!RevisionAvailable(u))return UnitMapEndStatus::RevisionExhausted;
    u.map_end={true,s};++u.map_end_revision;return UnitMapEndStatus::Ok;
}
UnitMapEndStatus RestoreUnitEnhance(UnitState& u,const UnitEnhanceSnapshot& s) noexcept {
    if(!Available(u))return UnitMapEndStatus::InvalidUnit;
    if(u.enhance.bound)return UnitMapEndStatus::AlreadyBound;
    if(!RevisionAvailable(u))return UnitMapEndStatus::RevisionExhausted;
    StoreEnhance(u,s);u.combat_state_valid=false;++u.map_end_revision;return UnitMapEndStatus::Ok;
}
UnitMapEndStatus CleanupUnitEnhance(UnitState& u,UnitEnhanceCleanup op) noexcept {
    if(!Available(u))return UnitMapEndStatus::InvalidUnit;
    if(op>UnitEnhanceCleanup::Clear)return UnitMapEndStatus::InvalidOperation;
    if(op!=UnitEnhanceCleanup::Clear&&!u.enhance.bound)return UnitMapEndStatus::UnboundEnhance;
    if(!RevisionAvailable(u))return UnitMapEndStatus::RevisionExhausted;
    auto s=CurrentUnitEnhanceSnapshot(u);
    switch(op) {
        case UnitEnhanceCleanup::ResetCondition:ResetEnhanceConditionsExact(s);break;
        case UnitEnhanceCleanup::Dead:ApplyEnhanceDeathExact(s);break;
        case UnitEnhanceCleanup::Clear:ClearEnhanceExact(s);break;
    }
    StoreEnhance(u,s);u.combat_state_valid=false;++u.map_end_revision;return UnitMapEndStatus::Ok;
}
UnitMapEndStatus ResetUnitEndOfMap(NativeRuntime& r,std::uint16_t slot,bool position,bool enhance) noexcept {
    if(slot>=r.game.units.size()||!Available(r.game.units[slot]))return UnitMapEndStatus::InvalidUnit;
    auto& u=r.game.units[slot];
    if(!u.map_end.bound)return UnitMapEndStatus::UnboundMapEnd;
    if(!RevisionAvailable(u))return UnitMapEndStatus::RevisionExhausted;
    UnitMapEndContext context{u.force_type};
    if(position&&(u.force_type==0||u.force_type>=3)) {
        const auto* person=r.definitions.FindPerson(u.person_id);
        if(!person)return UnitMapEndStatus::MissingDefinition;
        if(!r.definitions.person_archives_known())return UnitMapEndStatus::UnboundPersonArchives;
        context.person_resident=PersonIsResidentExact(r.definitions.person_archives(),person->id);
        if(context.person_resident&&!(u.flags&0x42000u)) {
            const auto* job=r.definitions.FindJob(u.job_id);
            if(!job)return UnitMapEndStatus::MissingDefinition;
            context.person_private_flags=Bits(person->bitflags);context.job_private_flags=Bits(job->bitflags);
        }
    }
    UnitMapEndSnapshot s{u.flags,Bits(u.private_skill_bits),{},u.map_end.fields,CurrentUnitEnhanceSnapshot(u)};
    // Positions need no byte projection: this policy preserves them unless the
    // caller requested removal. Native coordinates may be wider than retail.
    ResetUnitEndOfMapExact(s,context,position,enhance);
    u.flags=s.flags;for(unsigned i=0;i<8;++i)u.private_skill_bits[i]=std::uint8_t(s.private_flags>>(i*8));
    u.map_end.fields=s.fields;
    if(position){u.has_position=false;u.x=u.y=255;}
    if(enhance)StoreEnhance(u,s.enhance);
    // Unit+F8 and +12E already have a native owner. Both original stores fully
    // overwrite these fields, so even unknown prior restrictions become known.
    u.attack_restrictions={true,std::nullopt,0};
    u.pair.guard_progress=0;
    u.combat_state_valid=false;++u.map_end_revision;
    return UnitMapEndStatus::Ok;
}
}
