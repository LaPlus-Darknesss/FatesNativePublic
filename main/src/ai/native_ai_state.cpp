#include "fates/ai/native_ai_state.hpp"
#include <bit>
#include <memory>
namespace fates::ai::native {
using namespace runtime::native;
void InitializeFreshUnitAiState(UnitState& u) noexcept {
    u.ai={};u.ai.configured=true;u.ai.runtime_tuning_bound=true;
    u.ai.battle_rate=0;u.ai.move_limit_mode=0;
    u.ai_activity=0;u.ai_band=0;u.attack_restrictions={true,std::nullopt,0};
}
void BindDisposAiBand(UnitState& u,std::uint8_t band) noexcept {u.ai_band=band;}
namespace {
using S=AiStateWriteStatus;
bool Valid(const NativeGameState& s,std::uint16_t slot) {
    return slot<s.units.size()&&s.units[slot].occupied&&s.units[slot].force_type<9;
}
S SetPairActivity(NativeGameState& s,std::uint16_t slot,std::uint8_t value) {
    auto& u=s.units[slot];
    if(u.pair.role==PairRole::None) {
        if(u.pair.partner_slot!=0xffffu || (u.flags&6u))return S::InvalidPair;
    } else {
        const auto other=u.pair.partner_slot;
        if(!u.pair.bound||!Valid(s,other)||other==slot)return S::InvalidPair;
        auto& p=s.units[other];
        if(!p.pair.bound||p.pair.partner_slot!=slot||p.pair.role==PairRole::None||
           p.pair.role==u.pair.role||p.force_type!=u.force_type)return S::InvalidPair;
        p.ai_activity=value;
    }
    u.ai_activity=value;return S::Ok;
}
S BandActivate(NativeGameState& s,std::uint16_t slot) {
    const auto& source=s.units[slot];
    if(!source.ai_band)return S::MissingAiState;
    if(*source.ai_band==0)return S::Ok;
    if(!source.ai.runtime_tuning_bound)return S::MissingAiState;
    if(!(source.ai.policy_flags&0x20000u))return S::Ok;
    const bool copy=(source.ai.policy_flags&0x40000u)!=0;
    if(copy&&!source.ai.configured)return S::MissingAiState;
    for(std::size_t i=0;i<s.units.size();++i) {
        auto& member=s.units[i];
        if(i==slot||!member.occupied||member.force_type!=source.force_type)continue;
        if(!member.ai_band)return S::MissingAiState;
        if(member.ai_band!=source.ai_band)continue;
        if(!member.ai_activity)return S::MissingAiState;
        if(*member.ai_activity==0)member.ai_activity=1;
        if(copy) {
            // Copy only the movement declaration and its four arguments. Do not
            // turn an otherwise unconfigured descriptor into a complete one.
            member.ai.movement_id=source.ai.movement_id;
            member.ai.movement_args=source.ai.movement_args;
        }
    }
    return S::Ok;
}
S Activate(NativeGameState& s,std::uint16_t slot,bool force_band) {
    auto& u=s.units[slot];if(!u.ai_activity)return S::MissingAiState;
    if(*u.ai_activity==0){const auto result=SetPairActivity(s,slot,1);if(result!=S::Ok)return result;}
    if(!force_band) {
        if(!u.ai.runtime_tuning_bound)return S::MissingAiState;
        if(u.ai.policy_flags&0x80000u)return S::Ok;
    }
    return BandActivate(s,slot);
}
template<class Operation>S Atomic(NativeGameState& s,std::uint16_t slot,Operation operation) {
    if(!Valid(s,slot))return S::InvalidUnit;
    auto staged=std::make_unique<NativeGameState>(s);const auto result=operation(*staged);
    if(result==S::Ok)s=std::move(*staged);
    return result;
}
std::array<std::int16_t,4>& Arguments(UnitState& u,std::uint8_t channel) {
    switch(channel){case 0:return u.ai.action_args;case 1:return u.ai.mission_args;
        case 2:return u.ai.attack_args;default:return u.ai.movement_args;}
}
}
AiStateWriteStatus ActivateUnitAi(NativeGameState& s,std::uint16_t slot,bool force_band) {
    return Atomic(s,slot,[&](auto& next){return Activate(next,slot,force_band);});
}
AiStateWriteStatus ActivateUnitAiCauseAttacked(NativeGameState& s,std::uint16_t slot,bool second_policy) {
    return Atomic(s,slot,[&](auto& next){
        const auto& u=next.units[slot];if(!u.ai.runtime_tuning_bound)return S::MissingAiState;
        if(!(u.ai.policy_flags&(second_policy?2u:1u)))return S::Ok;
        return Activate(next,slot,true); // attacked cause bypasses the ordinary 0x80000 suppression
    });
}
AiStateWriteStatus CommitAiThinkUpdate(NativeGameState& s,std::uint16_t slot,std::uint8_t channel,UpdateCommitInput& pending) {
    if(channel>=4)return S::InvalidChannel;
    const auto update=ResolveUpdateCommit(pending);
    const auto status=Atomic(s,slot,[&](auto& next){
        if(update.writeActivity) {
            const auto result=SetPairActivity(next,slot,update.activity);if(result!=S::Ok)return result;
            if(update.activateUnit){const auto result2=Activate(next,slot,false);if(result2!=S::Ok)return result2;}
        }
        // Propagation observes the old movement arguments: dirty argument writes
        // occur after AIActivate, including when this update targets movement.
        auto& args=Arguments(next.units[slot],channel);
        for(unsigned i=0;i<4;++i)if(update.writeValue[i])args[i]=std::bit_cast<std::int16_t>(update.values[i]);
        return S::Ok;
    });
    if(status==S::Ok)pending.dirtyFlags=update.remainingFlags;
    return status;
}
bool IsDontAttackExact(std::uint32_t actor_flags,std::uint32_t target_flags,bool ignore,
    bool person_matches,std::uint8_t mask,std::uint8_t force) noexcept {
    if(!ignore&&((actor_flags|target_flags)&0x400000u))return true;
    if(person_matches)return true;
    // ARM register LSL gives zero for byte-sized counts >=32. Only the eight
    // low force bits can match the byte mask; never shift outside the C++ domain.
    return force<8u&&(mask&(1u<<force))!=0;
}
std::optional<bool> NativeAttackPermission(const NativeRuntime& r,std::optional<std::uint16_t> actor,
    std::uint16_t target_slot,bool ignore) {
    if(!Valid(r.game,target_slot))return std::nullopt;
    const auto& target=r.game.units[target_slot];
    if((target.flags&4u)||target.pair.role==PairRole::Partner)return false;
    if(!actor)return true;
    if(!Valid(r.game,*actor))return std::nullopt;
    const auto& source=r.game.units[*actor];
    if(!ignore&&((source.flags|target.flags)&0x400000u))return false;
    const auto& restriction=source.attack_restrictions;
    if(!restriction.bound)return std::nullopt;
    if(restriction.excluded_person) {
        if(!r.definitions.FindPerson(*restriction.excluded_person)||!r.definitions.FindPerson(target.person_id))return std::nullopt;
        if(*restriction.excluded_person==target.person_id)return false;
    }
    return !IsDontAttackExact(source.flags,target.flags,ignore,false,restriction.excluded_forces,target.force_type);
}
} // namespace fates::ai::native
