#include "fates/map/native_actor_position.hpp"
#include "fates/map/native_actor_visual.hpp"
#include "fates/map/native_height_geometry.hpp"
#include "fates/runtime/native_runtime.hpp"
#include <algorithm>
#include <cmath>
namespace fates::map::native {
namespace {
using S=ActorPositionStatus;
int Signed(std::uint8_t b) noexcept {return b<128?int(b):int(b)-256;}
bool Finite(ActorPositionVector v) noexcept {return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
}
ActorPositionResult ActorBasePositionExact(ActorPositionUnit unit,ActorPositionServices& svc) {
    ActorPositionVector value{float(Signed(unit.x))+0.5f,0.0f,float(Signed(unit.y))+0.5f};
    for(const bool partner:{false,true})if(unit.flags&(partner?4u:2u)) {
        const auto offset=svc.PairOffset(partner);if(!offset)return {S::MissingOffset};
        if(!std::isfinite(offset->x)||!std::isfinite(offset->z))return {S::InvalidValue};
        value.x=value.x+offset->x;value.z=value.z+offset->z;
    }
    if(!Finite(value))return {S::InvalidValue};
    const auto height=svc.MapHeight(value,true);if(!height)return {S::MissingHeight};
    value.y=*height;if(!Finite(value))return {S::InvalidValue};return {S::Ok,value};
}
ActorPositionStatus UpdateActorPositionExact(std::uint16_t slot,bool update,ActorPositionServices& svc) {
    const auto unit=svc.ReadUnit(slot);if(!unit)return S::MissingUnit;
    if(update&&(unit->flags&2)) {
        if(!unit->partner_known)return S::MissingPair;
        if(unit->partner) {
            if(!svc.WriteCoordinates(*unit->partner,unit->x,unit->y))return S::MissingUnit;
            const auto actor=svc.HasActor(*unit->partner);if(!actor)return S::MissingActorPresence;
            if(*actor){const auto status=UpdateActorPositionExact(*unit->partner,false,svc);if(status!=S::Ok)return status;}
        }
    }
    // GetBasePos rereads the current Unit after partner propagation.
    const auto current=svc.ReadUnit(slot);if(!current)return S::MissingUnit;
    const auto position=ActorBasePositionExact(*current,svc);if(position.status!=S::Ok)return position.status;
    return svc.StorePosition(slot,position.position)?S::Ok:S::MissingActorBinding;
}
struct ActorPositionAccess {
    using Runtime=fates::runtime::native::NativeRuntime;
    using Record=NativeActorPositionState::Record;
    static bool Present(const Runtime& r,std::uint16_t slot) {
        if(slot>=r.game.units.size())return false;const auto& u=r.game.units[slot];
        return u.occupied&&u.transfer.bound&&u.transfer.person_id==u.person_id&&u.transfer.value.map_actor==fates::runtime::native::UnitMapActorPresence::Present;
    }
    static bool Identity(const Runtime& r,const Record& v) {
        return Present(r,v.slot)&&v.generation==r.game.unit_slot_generations[v.slot]&&v.person==r.game.units[v.slot].person_id&&v.map_active==r.game.map_active&&v.chapter==r.game.campaign.current_chapter_index;
    }
    static auto Find(const Runtime& r,std::uint16_t slot) {const auto& v=r.game.actor_positions.records_;return std::find_if(v.begin(),v.end(),[&](const auto& a){return a.slot==slot;});}
    static S Bind(Runtime& r,std::uint16_t slot,ActorPositionVector value,bool restore) {
        if(!Present(r,slot))return S::MissingActorPresence;if(!Finite(value))return S::InvalidValue;
        auto& records=r.game.actor_positions.records_;auto found=std::find_if(records.begin(),records.end(),[&](const auto& a){return a.slot==slot;});
        if(!restore&&(found==records.end()||!Identity(r,*found)))return S::MissingActorBinding;
        const auto& u=r.game.units[slot];Record next{slot,u.person_id,r.game.unit_slot_generations[slot],u.x,u.y,u.flags&6u,true,r.game.map_active,r.game.campaign.current_chapter_index,r.game.actor_position_offsets,value};
        if(found==records.end())records.push_back(next);else *found=next;return S::Ok;
    }
    static CurrentActorPositionView Read(const Runtime& r,std::uint16_t slot) {
        const auto found=Find(r,slot);if(found==r.game.actor_positions.records_.end())return {S::MissingActorBinding};
        if(!found->value_bound||!Identity(r,*found))return {S::StaleActor};const auto& u=r.game.units[slot];
        if(found->x!=u.x||found->y!=u.y||found->pair_flags!=(u.flags&6u)||found->offsets!=r.game.actor_position_offsets)return {S::StaleActor};
        return {S::Ok,&found->position};
    }
    static void Invalidate(fates::runtime::native::NativeGameState& g){for(auto& v:g.actor_positions.records_)v.value_bound=false;}
    static void Forget(fates::runtime::native::NativeGameState& g){g.actor_positions.records_.clear();}
};
ActorPositionStatus RestoreCurrentActorPosition(fates::runtime::native::NativeRuntime& r,std::uint16_t s,ActorPositionVector p){return ActorPositionAccess::Bind(r,s,p,true);}
void InvalidateCurrentActorPositions(fates::runtime::native::NativeGameState& g) noexcept {ActorPositionAccess::Invalidate(g);}
void ForgetCurrentActorPositions(fates::runtime::native::NativeGameState& g) noexcept {
    ActorPositionAccess::Forget(g);ForgetCurrentActorVisuals(g);
}
CurrentActorPositionView ReadCurrentActorPosition(const fates::runtime::native::NativeRuntime& r,std::uint16_t s){return ActorPositionAccess::Read(r,s);}
ActorPositionStatus UpdateCurrentActorPositionStaged(fates::runtime::native::NativeRuntime& r,std::uint16_t slot,bool partner,const CurrentActorHeightQuery* height) {
    namespace rn=fates::runtime::native;
    struct Services:ActorPositionServices {
        rn::NativeRuntime& r;const CurrentActorHeightQuery* height;
        Services(rn::NativeRuntime& r,const CurrentActorHeightQuery* h):r(r),height(h){}
        std::optional<ActorPositionUnit> ReadUnit(std::uint16_t s) override {
            if(s>=r.game.units.size()||!r.game.units[s].occupied)return std::nullopt;const auto& u=r.game.units[s];
            if(u.x<-128||u.x>255||u.y<-128||u.y>255)return std::nullopt;
            return ActorPositionUnit{std::uint8_t(u.x),std::uint8_t(u.y),u.flags,u.pair.bound,u.pair.partner_slot==0xffff?std::nullopt:std::optional<std::uint16_t>(u.pair.partner_slot)};
        }
        std::optional<bool> HasActor(std::uint16_t s) override {
            if(s>=r.game.units.size())return std::nullopt;const auto& u=r.game.units[s];
            if(!u.occupied||!u.transfer.bound||u.transfer.person_id!=u.person_id)return std::nullopt;
            return u.transfer.value.map_actor==rn::UnitMapActorPresence::Present;
        }
        bool WriteCoordinates(std::uint16_t s,std::uint8_t x,std::uint8_t y) override {
            if(s>=r.game.units.size()||!r.game.units[s].occupied)return false;auto& u=r.game.units[s];u.x=x;u.y=y;u.has_position=true;return true;
        }
        std::optional<ActorPairOffset> PairOffset(bool partner) override {return partner?r.game.actor_position_offsets.partner:r.game.actor_position_offsets.lead;}
        std::optional<float> MapHeight(ActorPositionVector q,bool layer) override {
            if(height)return height->MapHeight(r,q,layer);
            const auto result=QueryCurrentMapHeight(r,{q.x,q.y,q.z},layer);
            return result.status==HeightStatus::Ok?std::optional(result.height):std::nullopt;
        }
        bool StorePosition(std::uint16_t s,ActorPositionVector p) override {return ActorPositionAccess::Bind(r,s,p,false)==S::Ok;}
    } svc(r,height);
    return UpdateActorPositionExact(slot,partner,svc);
}
}
