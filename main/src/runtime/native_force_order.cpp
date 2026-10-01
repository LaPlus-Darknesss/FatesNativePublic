#include "fates/runtime/native_force_order.hpp"
#include "fates/runtime/native_unit_pool.hpp"
#include "fates/runtime/native_runtime.hpp"
#include <algorithm>
#include <array>
namespace fates::runtime::native {
ForceSkillLookupResult FindForceUnitFromPrivateSkill(const NativeRuntime& r,std::uint8_t force,std::uint64_t mask) {
    using L=ForceSkillLookupStatus;ForceSkillLookupResult result;
    const auto* order=GetVerifiedForceOrder(r.game,force);
    if(!order){result.status=L::MissingOrder;return result;}
    const auto bits=[](const std::array<std::uint8_t,8>& b){std::uint64_t v{};
        for(unsigned i=0;i<8;++i)v|=std::uint64_t(b[i])<<(8*i);return v;};
    for(unsigned i=0;i<order->count;++i) {
        ++result.examined;const auto slot=order->slots[i];const auto& u=r.game.units[slot];
        // Original reads both definition flag words before testing the mask;
        // even a rejecting Unit flag does not short circuit those reads.
        const auto* person=r.definitions.ResolvePerson(u.person_record);
        if(!person || person->id!=u.person_id){result.status=L::MissingPerson;return result;}
        const auto* job=r.definitions.FindJob(u.job_id);
        if(!job){result.status=L::MissingJob;return result;}
        if(!((bits(u.private_skill_bits)|bits(person->bitflags)|bits(job->bitflags))&mask))continue;
        if(u.flags&0x40000u)continue;
        if(u.flags&0x10000000u) {
            const auto download=r.definitions.IsPersonDownload(*person);
            if(!download){result.status=L::UnknownPersonArchives;return result;}
            if(!*download)continue;
        }
        result.slot=slot;return result;
    }
    return result;
}
bool ForcesAlliedExact(std::uint8_t a,std::uint8_t b) noexcept {
    return a==b||(a==0&&b==2)||(a==2&&b==0);
}
namespace {
using S=ForceOrderStatus;
bool Structure(const NativeForceOrderState& o) noexcept {
    if(!o.bound||o.count>o.slots.size())return false;
    std::array<bool,250> seen{};
    for(unsigned i=0;i<o.count;++i){if(o.slots[i]>=seen.size()||seen[o.slots[i]])return false;seen[o.slots[i]]=true;}
    return true;
}
const NativeForceOrderState* Order(const NativeGameState& s,std::uint8_t f) noexcept {
    if(f>=9)return nullptr;return f==0?&s.player_force_order:&s.other_force_orders[f-1];
}
NativeForceOrderState* Order(NativeGameState& s,std::uint8_t f) noexcept {
    return const_cast<NativeForceOrderState*>(Order(static_cast<const NativeGameState&>(s),f));
}
bool Empty(const NativeGameState& s,std::uint8_t f) noexcept {
    for(const auto& u:s.units)if(u.occupied&&u.force_type==f)return false;
    return true;
}
bool KnownOrEmpty(const NativeGameState& s,std::uint8_t f,NativeForceOrderState& out) noexcept {
    if(f>=9)return false;
    if(Empty(s,f)){out={};out.bound=true;return true;}
    if(!ForceOrderMatches(s,f))return false;out=*Order(s,f);return true;
}
}
ForceOrderStatus JoinForceOrderExact(NativeForceOrderState& o,std::uint16_t slot,std::uint16_t person,bool last) noexcept {
    if(!Structure(o))return S::InvalidOrder;
    if(slot>=250)return S::InvalidUnit;
    for(unsigned i=0;i<o.count;++i)if(o.slots[i]==slot)return S::AlreadyMember;
    if(o.count==o.slots.size())return S::Capacity;
    const unsigned at=last?o.count:0;
    for(unsigned i=o.count;i>at;--i){o.slots[i]=o.slots[i-1];o.person_ids[i]=o.person_ids[i-1];}
    o.slots[at]=slot;o.person_ids[at]=person;++o.count;return S::Ok;
}
ForceOrderStatus RemoveForceOrderExact(NativeForceOrderState& o,std::uint16_t slot) noexcept {
    if(!Structure(o))return S::InvalidOrder;
    unsigned at=0;while(at<o.count&&o.slots[at]!=slot)++at;
    if(at==o.count)return S::NotMember;
    for(unsigned i=at+1;i<o.count;++i){o.slots[i-1]=o.slots[i];o.person_ids[i-1]=o.person_ids[i];}
    --o.count;o.slots[o.count]=0;o.person_ids[o.count]=0;return S::Ok;
}
bool ForceOrderMatches(const NativeGameState& s,std::uint8_t f) noexcept {
    const auto* o=Order(s,f);if(!o||!Structure(*o))return false;
    std::array<bool,250> seen{};
    for(unsigned i=0;i<o->count;++i){const auto slot=o->slots[i];seen[slot]=true;const auto& u=s.units[slot];
        if(!u.occupied||u.force_type!=f||u.person_id!=o->person_ids[i])return false;}
    for(unsigned slot=0;slot<s.units.size();++slot)
        if((s.units[slot].occupied&&s.units[slot].force_type==f)!=seen[slot])return false;
    return true;
}
const NativeForceOrderState* GetVerifiedForceOrder(const NativeGameState& s,std::uint8_t f) noexcept {
    return ForceOrderMatches(s,f)?Order(s,f):nullptr;
}
ForceOrderStatus InitializeEmptyForceOrder(NativeGameState& s,std::uint8_t f) noexcept {
    if(f>=9)return S::InvalidForce;if(!Empty(s,f))return S::MissingOrder;
    auto& o=*Order(s,f);o={};o.bound=true;return S::Ok;
}
ForceOrderStatus JoinFreshUnitForceOrder(NativeGameState& s,std::uint16_t slot,std::uint8_t f,bool last) noexcept {
    if(f>=9)return S::InvalidForce;
    if(slot>=s.units.size()||!s.units[slot].occupied||s.units[slot].force_type!=9)return S::InvalidUnit;
    NativeForceOrderState next;if(!KnownOrEmpty(s,f,next))return S::MissingOrder;
    const auto result=JoinForceOrderExact(next,slot,s.units[slot].person_id,last);if(result!=S::Ok)return result;
    if(CompleteFreshUnitPoolSlot(s,slot)!=UnitPoolStatus::Ok)return S::InvalidOrder;
    *Order(s,f)=next;s.units[slot].force_type=f;return S::Ok;
}
ForceOrderStatus MoveUnitForceMembership(NativeGameState& s,std::uint16_t slot,std::uint8_t f,bool last) noexcept {
    if(f>=9)return S::InvalidForce;
    if(slot>=s.units.size()||!s.units[slot].occupied||s.units[slot].force_type>=9)return S::InvalidUnit;
    const auto source=s.units[slot].force_type;
    if(!ForceOrderMatches(s,source))return S::MissingOrder;
    auto from=*Order(s,source);NativeForceOrderState to;
    if(source!=f&&!KnownOrEmpty(s,f,to))return S::MissingOrder;
    auto result=RemoveForceOrderExact(from,slot);if(result!=S::Ok)return result;
    auto& destination=source==f?from:to;
    result=JoinForceOrderExact(destination,slot,s.units[slot].person_id,last);if(result!=S::Ok)return result;
    *Order(s,source)=from;if(source!=f)*Order(s,f)=to;s.units[slot].force_type=f;return S::Ok;
}
bool PlayerForceOrderMatches(const NativeGameState& s) noexcept {return ForceOrderMatches(s,0);}
void AppendPlayerSortieForceOrder(NativeGameState& s,std::span<const std::uint16_t> added) noexcept {
    NativeForceOrderState next;
    if(!KnownOrEmpty(s,0,next)){s.player_force_order={};return;}
    auto reserve=*Order(s,3);const bool reserve_known=ForceOrderMatches(s,3);
    for(auto slot:added){
        if(slot>=s.units.size()||!s.units[slot].occupied||s.units[slot].force_type!=3||
           JoinForceOrderExact(next,slot,s.units[slot].person_id,true)!=S::Ok){s.player_force_order={};return;}
        if(reserve_known&&RemoveForceOrderExact(reserve,slot)!=S::Ok){s.player_force_order={};return;}
    }
    s.player_force_order=next;if(reserve_known)*Order(s,3)=reserve;
}
}
