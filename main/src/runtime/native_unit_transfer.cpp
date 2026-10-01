#include "fates/runtime/native_unit_transfer.hpp"
#include "fates/runtime/native_runtime.hpp"
#include "fates/runtime/native_force_order.hpp"
#include "fates/runtime/native_unit_pool.hpp"
#include <limits>
#include <memory>
namespace fates::runtime::native {
namespace {
using S=UnitTransferStatus;using E=UnitTransferEffect;
bool RevisionAvailable(std::uint64_t value) noexcept {return value!=std::numeric_limits<std::uint64_t>::max();}
UnitTransferResult RunUnit(NativeRuntime& r,std::uint16_t slot,std::uint8_t destination,bool last,bool enhance) {
    UnitTransferResult out{};out.failed_slot=slot;
    auto fail=[&](S status){out.status=status;return out;};
    if(destination>=10)return fail(S::InvalidForce);
    if(slot>=r.game.units.size())return fail(S::InvalidUnit);
    auto& u=r.game.units[slot];auto& state=u.transfer;
    if(!u.occupied||u.force_type>=9)return fail(S::InvalidUnit);
    if(!state.bound)return fail(S::UnboundTransfer);
    if(state.person_id!=u.person_id)return fail(S::StaleTransfer);
    if(!RevisionAvailable(state.revision))return fail(S::RevisionExhausted);
    if(state.value.map_actor>UnitMapActorPresence::Present)return fail(S::InvalidSnapshot);
    const auto plan=PlanUnitTransferExact({u.force_type,destination,last,enhance,u.flags,state.value.map_actor==UnitMapActorPresence::Present,state.value.chapter_points.has_value(),u.lineage.bound&&u.lineage.value.family.has_value()});
    // These effects require concrete owners, not an assumed successful callback.
    if(plan.destination==9&&!r.game.free_unit_pool.bound)return fail(S::FreePoolRequired);
    if(state.value.map_actor==UnitMapActorPresence::Present)return fail(S::UnresolvedMapActor);
    for(unsigned i=0;i<plan.count;++i)switch(plan.effects[i]) {
    case E::ResetMapEnd:
        out.map_end_status=ResetUnitEndOfMap(r,slot,true,enhance);
        if(out.map_end_status!=UnitMapEndStatus::Ok)return fail(S::UnresolvedMapEnd);
        break;
    case E::DeathCleanup:
        if(!u.map_end.bound){out.map_end_status=UnitMapEndStatus::UnboundMapEnd;return fail(S::UnresolvedMapEnd);}
        out.map_end_status=CleanupUnitEnhance(u,UnitEnhanceCleanup::Dead);
        if(out.map_end_status!=UnitMapEndStatus::Ok)return fail(S::UnresolvedMapEnd);
        u.map_end.fields.secondary_flags&=~0xa8u;u.flags&=0xf82481deu;
        u.map_end.fields.counters[0]=0;u.pair.guard_progress=0;
        break;
    case E::Heal:
        out.capability_status=RefreshUnitMaximumHp(r,slot);
        if(out.capability_status!=UnitCapabilityStatus::Ok)return fail(S::UnresolvedCapability);
        u.current_hp=u.max_hp;u.defeated=u.current_hp<=0;
        break;
    case E::DeleteChapterPoints:state.value.chapter_points.reset();break;
    case E::ClearFamilyChapterPoints:
        if(!RevisionAvailable(u.lineage.revision))return fail(S::RevisionExhausted);
        u.lineage.value.family->chapter_points.fill(0);u.lineage.value.family->chapter_points_known=true;++u.lineage.revision;
        break;
    case E::ClearChapterCounter:state.value.chapter_counter=0;break;
    case E::Remove:break; // native list owner commits the paired unlink/relink below
    case E::ClearUnit: {
        const auto status=RecycleUnitPoolSlot(r.game,slot,last);
        if(status==UnitPoolStatus::UnresolvedPair)return fail(S::UnresolvedPair);
        if(status==UnitPoolStatus::RevisionExhausted)return fail(S::RevisionExhausted);
        if(status==UnitPoolStatus::MissingForceOrder)return fail(S::MissingForceOrder);
        if(status!=UnitPoolStatus::Ok)return fail(S::UnresolvedPool);
        break;
    }
    case E::JoinFirst:case E::JoinLast:
        if(plan.destination==9)break; // shared recycle owner already joined Force9
        if(MoveUnitForceMembership(r.game,slot,plan.destination,last)!=ForceOrderStatus::Ok)return fail(S::MissingForceOrder);
        break;
    default:return fail(S::UnresolvedMapActor); // guarded actor/free-pool effects above
    }
    // Other stat getters can depend on current force; only MHP is recomputed.
    // has_position is the native tactical-presence bit, not a stored coordinate
    // byte. Reserve transfers preserve x/y where retail does, but leave the map.
    if(!IsTacticalForce(plan.destination))u.has_position=false;
    u.combat_state_valid=false;if(plan.destination!=9)++state.revision;
    out.failed_slot=0xffffu;out.units_moved=1;return out;
}
}
UnitTransferPlan PlanUnitTransferExact(const UnitTransferFacts& f) noexcept {
    UnitTransferPlan p{};if(f.source>=10||f.destination>=10)return p;
    p.valid=true;p.destination=f.destination;p.clear_enhance=f.clear_enhance;
    auto add=[&](E e){p.effects[p.count++]=e;};
    const bool cleanup=(f.source<3||f.source==4||f.source==8)&&f.destination>=3&&f.destination!=8&&f.destination!=9;
    if(cleanup) {
        add(f.destination==4?E::DeathCleanup:E::ResetMapEnd);add(E::Heal);
        if(f.map_actor)add(E::DestroyMapActor);
        if(f.destination!=3&&f.destination!=4) {
            if(f.chapter_points)add(E::DeleteChapterPoints);
            if(f.family)add(E::ClearFamilyChapterPoints);
            add(E::ClearChapterCounter);
        }
    }
    // Both cleanup masks preserve public40000. Direct Unit transfer redirects
    // after cleanup; Force transfer performs its own earlier redirection.
    if((f.public_flags&0x40000u)&&f.destination>=3)p.destination=9;
    add(E::Remove);if(p.destination==9)add(E::ClearUnit);
    add(f.last?E::JoinLast:E::JoinFirst);
    if(f.map_actor&&!cleanup&&p.destination!=9)add(E::RefreshMapActorIcon);
    return p;
}
UnitTransferStatus RestoreUnitTransferSnapshot(NativeRuntime& r,std::uint16_t slot,const UnitTransferSnapshot& input) {
    if(slot>=r.game.units.size()||!r.game.units[slot].occupied||r.game.units[slot].force_type>=9)return S::InvalidUnit;
    auto& u=r.game.units[slot];auto& state=u.transfer;
    if(state.bound)return S::AlreadyBound;
    if(input.map_actor>UnitMapActorPresence::Present)return S::InvalidSnapshot;
    if(!RevisionAvailable(state.revision))return S::RevisionExhausted;
    if(!r.definitions.FindPerson(u.person_id))return S::InvalidUnit;
    NativeUnitTransferState next{true,u.person_id,input,state.revision+1};state=std::move(next);return S::Ok;
}
std::vector<ForceTransferRequest> BuildForceTransferRequestsExact(std::span<const ForceTransferUnit> order,std::uint8_t destination,bool last) {
    std::vector<ForceTransferRequest> out;out.reserve(order.size());
    for(unsigned i=0;i<order.size();++i) {
        const auto& unit=order[last?i:order.size()-1-i];
        out.push_back({unit.slot,(destination>=3&&(unit.public_flags&0x40000u))?std::uint8_t(9):destination,last});
    }
    return out;
}
UnitTransferStatus InvalidateUnitTransferSnapshot(NativeRuntime& r,std::uint16_t slot) noexcept {
    if(slot>=r.game.units.size()||!r.game.units[slot].occupied)return S::InvalidUnit;
    auto& state=r.game.units[slot].transfer;if(!RevisionAvailable(state.revision))return S::RevisionExhausted;
    state.bound=false;++state.revision;return S::Ok;
}
UnitTransferResult TransferUnit(NativeRuntime& r,std::uint16_t slot,std::uint8_t destination,bool last,bool enhance) {
    auto image=std::make_unique<NativeRuntime>(r);auto result=RunUnit(*image,slot,destination,last,enhance);
    if(result.status==S::Ok)r.game=std::move(image->game);return result;
}
UnitTransferResult TransferForceUnits(NativeRuntime& r,std::uint8_t source,std::uint8_t destination,bool last) {
    if(source>=9||destination>=10)return {S::InvalidForce};
    auto image=std::make_unique<NativeRuntime>(r);
    const auto* order=GetVerifiedForceOrder(image->game,source);
    if(!order) {
        if(InitializeEmptyForceOrder(image->game,source)!=ForceOrderStatus::Ok)return {S::MissingForceOrder};
        order=GetVerifiedForceOrder(image->game,source);
    }
    const auto original=*order;
    if(original.count&&source==destination)return {S::SameForceBatch};
    std::vector<ForceTransferUnit> seeds;seeds.reserve(original.count);
    for(unsigned i=0;i<original.count;++i){const auto slot=original.slots[i];seeds.push_back({slot,image->game.units[slot].flags});}
    for(const auto& request:BuildForceTransferRequestsExact(seeds,destination,last)) {
        auto result=RunUnit(*image,request.slot,request.destination,last,true);
        if(result.status!=S::Ok){result.units_moved=0;return result;}
    }
    r.game=std::move(image->game);UnitTransferResult result{};result.units_moved=original.count;return result;
}
}
