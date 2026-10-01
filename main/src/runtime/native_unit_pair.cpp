#include "fates/runtime/native_unit_pair.hpp"
#include "fates/runtime/native_runtime.hpp"
#include <limits>
#include <memory>
namespace fates::runtime::native {
namespace {
using S=UnitPairStatus;
bool Available(std::uint64_t v) noexcept {return v!=std::numeric_limits<std::uint64_t>::max();}
bool Valid(const NativeGameState& s,std::uint16_t slot) noexcept {
    return slot<s.units.size()&&s.units[slot].occupied&&s.units[slot].force_type<9;
}
bool Unlinked(const UnitState& u) noexcept {
    return !u.pair.bound&&u.pair.role==PairRole::None&&u.pair.partner_slot==0xffffu&&(u.flags&6u)==0;
}
bool IncomingOnly(const NativeGameState& s,std::uint16_t a,std::uint16_t b,bool paired) noexcept {
    for(unsigned i=0;i<s.units.size();++i)if(s.units[i].occupied) {
        const auto ref=s.units[i].pair.partner_slot;
        if(ref==a&&(!paired||i!=b))return false;
        if(ref==b&&(!paired||i!=a))return false;
    }return true;
}
void InvalidateBonus(UnitState& u) noexcept {
    u.pair.person_guard_bonus_bound=false;u.pair.person_guard_bonus.fill(0);
}
std::uint64_t Flags(const std::array<std::uint8_t,8>& b) noexcept {
    std::uint64_t v=0;for(unsigned i=0;i<8;++i)v|=std::uint64_t(b[i])<<(8*i);return v;
}
}
bool CanPairUnitsExact(const UnitPairEligibilityFacts& a,const UnitPairEligibilityFacts& b,
    std::uint64_t mask) noexcept {
    return !((a.unit_flags|a.person_flags|a.job_flags)&mask)&&
           !((b.unit_flags|b.person_flags|b.job_flags)&mask)&&
           !a.linked&&!b.linked&&a.force==b.force;
}
std::optional<bool> CanPairCurrentUnits(const NativeRuntime& r,std::uint16_t a,std::uint16_t b) noexcept {
    if(!Valid(r.game,a)||!Valid(r.game,b))return std::nullopt;
    const auto mask=r.definitions.pair_prohibition_mask();if(!mask)return std::nullopt;
    const auto& x=r.game.units[a];const auto& y=r.game.units[b];
    const auto* xp=r.definitions.FindPerson(x.person_id);const auto* yp=r.definitions.FindPerson(y.person_id);
    const auto* xj=r.definitions.FindJob(x.job_id);const auto* yj=r.definitions.FindJob(y.job_id);
    if(!xp||!yp||!xj||!yj)return std::nullopt;
    return CanPairUnitsExact({Flags(x.private_skill_bits),Flags(xp->bitflags),Flags(xj->bitflags),x.pair.partner_slot!=0xffffu,x.force_type},
        {Flags(y.private_skill_bits),Flags(yp->bitflags),Flags(yj->bitflags),y.pair.partner_slot!=0xffffu,y.force_type},*mask);
}
UnitPairStatus LinkUnitPair(NativeGameState& s,std::uint16_t a,std::uint16_t b) noexcept {
    if(!Valid(s,a)||!Valid(s,b))return S::InvalidUnit;
    if(a==b)return S::SameUnit;
    auto& x=s.units[a];auto& y=s.units[b];
    if(!Unlinked(x)||!Unlinked(y))return S::ExistingPair;
    if(!IncomingOnly(s,a,b,false))return S::MalformedPair;
    if(x.x<-128||x.x>255||x.y<-128||x.y>255)return S::InvalidCoordinates;
    if(!Available(x.pair_revision)||!Available(y.pair_revision))return S::RevisionExhausted;
    x.pair.bound=y.pair.bound=true;x.pair.role=PairRole::Lead;y.pair.role=PairRole::Partner;
    x.pair.partner_slot=b;y.pair.partner_slot=a;x.flags|=2u;y.flags|=4u;
    y.x=x.x;y.y=x.y;y.has_position=x.has_position;
    InvalidateBonus(x);InvalidateBonus(y);++x.pair_revision;++y.pair_revision;return S::Ok;
}
UnitPairStatus UnlinkUnitPair(NativeGameState& s,std::uint16_t a) noexcept {
    if(!Valid(s,a))return S::InvalidUnit;
    auto& x=s.units[a];const auto b=x.pair.partner_slot;
    if(!Valid(s,b)||a==b||!x.pair.bound||x.pair.role!=PairRole::Lead)return S::MalformedPair;
    auto& y=s.units[b];
    if(!y.pair.bound||y.pair.role!=PairRole::Partner||y.pair.partner_slot!=a||!IncomingOnly(s,a,b,true))return S::MalformedPair;
    if(!Available(x.pair_revision)||!Available(y.pair_revision))return S::RevisionExhausted;
    x.flags&=~2u;y.flags&=~4u;
    x.pair.bound=y.pair.bound=false;x.pair.role=y.pair.role=PairRole::None;
    x.pair.partner_slot=y.pair.partner_slot=0xffffu;
    InvalidateBonus(x);InvalidateBonus(y);++x.pair_revision;++y.pair_revision;return S::Ok;
}
UnitPairStatus LinkUnitPairs(NativeGameState& s,std::span<const UnitPairRequest> requests) {
    auto next=std::make_unique<NativeGameState>(s);
    for(const auto& p:requests){const auto result=LinkUnitPair(*next,p.lead,p.partner);if(result!=S::Ok)return result;}
    s=std::move(*next);return S::Ok;
}
}
