#include "fates/map/native_actor_visual.hpp"
#include "fates/runtime/native_runtime.hpp"
#include "fates/runtime/native_force_order.hpp"
#include <algorithm>
#include <array>

namespace fates::map::native {
namespace rn=fates::runtime::native;
using S=ActorVisualStatus;
ActorVisualStatus SetUnitIconAnimationExact(UnitIconAnimationSnapshot& icon,std::uint32_t animation,bool force) noexcept {
    // Original immutable tables at627FC4 and627F94, twelve UnitAnim values.
    constexpr std::array<std::uint8_t,12> flip{0,2,1,3,4,6,5,8,7,9,10,11};
    constexpr std::array<bool,12> move{false,true,true,true,true,true,true,true,true,false,false,false};
    if(icon.flags&4) {
        if(animation>=flip.size())return S::UnknownAnimation;
        animation=flip[animation];
    }
    bool reset=force;
    if(!force && animation!=icon.animation) {
        if(icon.animation>=move.size())return S::UnknownAnimation;
        if(!move[icon.animation])reset=true;
        else {
            if(animation>=move.size())return S::UnknownAnimation;
            reset=!move[animation];
        }
    }
    icon.animation=static_cast<std::uint8_t>(animation);
    if(reset)icon.frame=0;
    return S::Ok;
}
ActorVisualStatus SetActorMotionExact(ActorVisualSnapshot& actor,std::uint32_t motion) noexcept {
    if(!actor.icon)return S::MissingIcon;
    if(auto s=SetUnitIconAnimationExact(*actor.icon,motion);s!=S::Ok)return s;
    actor.motion=static_cast<std::uint8_t>(motion);
    actor.countdown_2b=12;actor.counter_2e=0;actor.countdown_2c=12;actor.counter_30=0;
    return S::Ok;
}
struct ActorVisualAccess {
    using State=NativeActorVisualState;
    static S Presence(const rn::NativeRuntime& r,std::uint16_t slot) {
        if(slot>=r.game.units.size() || !r.game.units[slot].occupied)return S::InvalidUnit;
        const auto& u=r.game.units[slot];
        if(!u.transfer.bound || u.transfer.person_id!=u.person_id ||
           u.transfer.value.map_actor>rn::UnitMapActorPresence::Present)return S::MissingPresence;
        return u.transfer.value.map_actor==rn::UnitMapActorPresence::Present?S::Ok:S::ActorAbsent;
    }
    static bool Live(const rn::NativeRuntime& r,const State::Record& a) {
        const auto& u=r.game.units[a.slot];
        return a.person==u.person_id && a.generation==r.game.unit_slot_generations[a.slot] &&
            a.transfer_revision==u.transfer.revision && a.chapter==r.game.campaign.current_chapter_index &&
            a.map_active==r.game.map_active;
    }
    static S Find(const rn::NativeRuntime& r,State& state,std::uint16_t slot,State::Record*& out) {
        if(auto s=Presence(r,slot);s!=S::Ok)return s;
        auto it=std::find_if(state.records_.begin(),state.records_.end(),[&](const auto& a){return a.slot==slot;});
        if(it==state.records_.end())return S::MissingActor;
        if(!Live(r,*it))return S::StaleActor;
        out=&*it;return S::Ok;
    }
    static S Restore(rn::NativeRuntime& r,std::uint16_t slot,const ActorVisualSnapshot& value) {
        if(auto s=Presence(r,slot);s!=S::Ok)return s;
        auto& records=r.game.actor_visuals.records_;const auto& u=r.game.units[slot];
        auto it=std::find_if(records.begin(),records.end(),[&](const auto& a){return a.slot==slot;});
        if(it!=records.end() && Live(r,*it))return S::AlreadyBound;
        State::Record a{slot,u.person_id,r.game.unit_slot_generations[slot],u.transfer.revision,
            r.game.campaign.current_chapter_index,r.game.map_active,value};
        if(it==records.end())records.push_back(a);else *it=a;return S::Ok;
    }
    static S Read(const rn::NativeRuntime& r,std::uint16_t slot,ActorVisualSnapshot& out) {
        if(auto s=Presence(r,slot);s!=S::Ok)return s;
        const auto& records=r.game.actor_visuals.records_;
        const auto it=std::find_if(records.begin(),records.end(),[&](const auto& a){return a.slot==slot;});
        if(it==records.end())return S::MissingActor;
        if(!Live(r,*it))return S::StaleActor;
        out=it->value;return S::Ok;
    }
    static S Map(const rn::NativeRuntime& r,const State& state,bool& present) {
        if(!state.map_present_)return S::MissingMapPublication;
        if(state.map_chapter_!=r.game.campaign.current_chapter_index || state.map_active_!=r.game.map_active)return S::StaleMapPublication;
        present=*state.map_present_;return S::Ok;
    }
    static S RestoreMap(rn::NativeRuntime& r,bool present) {
        auto& state=r.game.actor_visuals;
        if(state.map_present_)return S::AlreadyBound;
        state.map_present_=present;state.map_chapter_=r.game.campaign.current_chapter_index;
        state.map_active_=r.game.map_active;return S::Ok;
    }
    static ActorVisualResult Reset(const rn::NativeRuntime& r,State& state,std::uint16_t slot,bool partner,bool motion) {
        State::Record* actor{};if(auto s=Find(r,state,slot,actor);s!=S::Ok)return {s,slot};
        // Both original reset methods read Unit+A8 even when propagation is off;
        // only the true branch follows that pointer to a partner actor.
        const auto& pair=r.game.units[slot].pair;
        if(partner) {
            // In the shared Pair owner, bound means linked, not availability.
            // Its complete unlinked representation is a known null Unit+A8.
            if(!pair.bound && (pair.role!=rn::PairRole::None || pair.partner_slot!=0xffffu ||
                (r.game.units[slot].flags&6u)))return {S::MissingPair,slot};
            if(pair.bound && pair.partner_slot==0xffffu)return {S::InvalidPair,slot};
            if(pair.partner_slot!=0xffffu) {
                const auto p=pair.partner_slot;
                if(p==slot || p>=r.game.units.size() || !r.game.units[p].occupied)return {S::InvalidPair,slot};
                const auto& other=r.game.units[p].pair;
                if(!other.bound || other.partner_slot!=slot ||
                   !((pair.role==rn::PairRole::Lead && other.role==rn::PairRole::Partner) ||
                     (pair.role==rn::PairRole::Partner && other.role==rn::PairRole::Lead)))return {S::InvalidPair,slot};
                const auto presence=Presence(r,p);
                if(presence==S::Ok) {
                    const auto result=Reset(r,state,p,false,motion);if(result.status!=S::Ok)return result;
                } else if(presence!=S::ActorAbsent)return {presence,p};
            }
        }
        if(motion) {
            if(auto s=SetActorMotionExact(actor->value,0);s!=S::Ok)return {s,slot};
            actor->value.flags_38&=~8u;
        } else actor->value.alpha=255;
        return {};
    }
    static ActorVisualResult ResetPublic(rn::NativeRuntime& r,std::uint16_t slot,bool partner,bool motion) {
        auto staged=r.game.actor_visuals;
        const auto result=Reset(r,staged,slot,partner,motion);
        if(result.status==S::Ok)r.game.actor_visuals=std::move(staged);
        return result;
    }
    static ActorVisualResult Consistency(rn::NativeRuntime& r) {
        auto staged=r.game.actor_visuals;
        for(std::uint8_t force=0;force<3;++force) {
            const auto* order=rn::GetVerifiedForceOrder(r.game,force);
            if(!order)return {S::MissingForceOrder,0xffffu};
            for(unsigned i=0;i<order->count;++i) {
                const auto slot=order->slots[i];const auto presence=Presence(r,slot);
                if(presence==S::ActorAbsent)continue;
                if(presence!=S::Ok)return {presence,slot};
                State::Record* actor{};if(auto s=Find(r,staged,slot,actor);s!=S::Ok)return {s,slot};
                if(actor->value.motion) {
                    const auto result=Reset(r,staged,slot,true,true);if(result.status!=S::Ok)return result;
                }
                bool map{};if(auto s=Map(r,staged,map);s!=S::Ok)return {s,slot};
                if(map && actor->value.alpha!=255) {
                    const auto result=Reset(r,staged,slot,true,false);if(result.status!=S::Ok)return result;
                }
            }
        }
        r.game.actor_visuals=std::move(staged);return {};
    }
};
ActorVisualStatus RestoreCurrentActorVisual(rn::NativeRuntime& r,std::uint16_t s,const ActorVisualSnapshot& v) {return ActorVisualAccess::Restore(r,s,v);}
ActorVisualStatus RestoreCurrentActorMapPublication(rn::NativeRuntime& r,bool p) {return ActorVisualAccess::RestoreMap(r,p);}
void ForgetCurrentActorVisuals(rn::NativeGameState& g) noexcept {g.actor_visuals={};}
ActorVisualStatus ReadCurrentActorVisual(const rn::NativeRuntime& r,std::uint16_t s,ActorVisualSnapshot& out) {
    return ActorVisualAccess::Read(r,s,out);
}
ActorVisualResult ResetCurrentActorMotion(rn::NativeRuntime& r,std::uint16_t s,bool p) {return ActorVisualAccess::ResetPublic(r,s,p,true);}
ActorVisualResult ResetCurrentActorAlpha(rn::NativeRuntime& r,std::uint16_t s,bool p) {return ActorVisualAccess::ResetPublic(r,s,p,false);}
ActorVisualResult ApplyCurrentActorConsistency(rn::NativeRuntime& r) {return ActorVisualAccess::Consistency(r);}
}
