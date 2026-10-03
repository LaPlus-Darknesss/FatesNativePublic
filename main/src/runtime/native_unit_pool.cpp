#include "fates/runtime/native_unit_pool.hpp"
#include "fates/runtime/native_definition_store.hpp"
#include "fates/runtime/native_force_order.hpp"
#include "fates/ai/native_ai_state.hpp"
#include <algorithm>
#include <limits>
#include <memory>
namespace fates::runtime::native {
namespace {
using S=UnitPoolStatus;using E=UnitClearEffect;
bool Available(std::uint64_t v) noexcept {return v!=std::numeric_limits<std::uint64_t>::max();}
// UnitState is semantic ownership, not a pointer-bearing retail byte image.
// Identity-dependent providers become unbound when Person/Job are cleared.
void ClearNativeUnit(UnitState& u) {
    u={};u.x=u.y=255;u.create_level=1;
    u.map_end.fields.previous_position.fill(255);
    fates::ai::native::InitializeFreshUnitAiState(u);
}
bool Matches(const NativeGameState& s,std::optional<std::uint16_t> constructing) noexcept {
    const auto& o=s.free_unit_pool.order;
    if(!s.free_unit_pool.bound||!o.bound||o.count>250)return false;
    std::array<bool,250> seen{};
    for(unsigned i=0;i<o.count;++i) {
        const auto slot=o.slots[i];if(slot>=250||seen[slot]||o.person_ids[i]!=0)return false;
        seen[slot]=true;const auto& u=s.units[slot];
        if(u.force_type!=9||((!constructing||slot!=*constructing)&&u.occupied))return false;
    }
    for(unsigned i=0;i<250;++i) {
        const bool free=(!s.units[i].occupied&&s.units[i].force_type==9)||(constructing&&i==*constructing);
        if(free!=seen[i])return false;
    }
    return true;
}
}
UnitPersonLookupStatus BindUnitPersonRecord(const DefinitionStore& definitions,UnitState& unit,
    const PersonRecordReference& reference) {
    using L=UnitPersonLookupStatus;
    if(!unit.occupied)return L::InvalidUnit;
    if(!definitions.person_archives_known())return L::UnknownPersonArchives;
    const auto* person=definitions.ResolvePerson(reference);
    if(!person || person->id!=unit.person_id)return L::InvalidPersonReference;
    unit.person_record=reference;return L::Ok;
}
UnitPersonLookupResult FindUnitFromPerson(const DefinitionStore& definitions,const NativeGameState& game,
    const PersonRecordReference& target) {
    using L=UnitPersonLookupStatus;UnitPersonLookupResult result;
    if(!definitions.person_archives_known()){result.status=L::UnknownPersonArchives;return result;}
    if(!definitions.ResolvePerson(target)){result.status=L::InvalidPersonReference;return result;}
    for(std::uint16_t slot=0;slot<game.units.size();++slot) {
        ++result.slots_examined;const auto& unit=game.units[slot];
        if(unit.force_type==9)continue;
        auto refuse=[&](L status){result.status=status;result.unavailable_slot=slot;return result;};
        if(unit.force_type>9 || !unit.occupied)return refuse(L::InvalidUnit);
        ++result.uniqueness_checks;
        // The rejecting flag avoids even a Person read. The download predicate
        // on a previous NONmatching Unit is nevertheless a reached dependency.
        if(unit.flags&0x40000u)continue;
        if(!unit.person_record)return refuse(L::MissingUnitPerson);
        const auto* person=definitions.ResolvePerson(unit.person_record);
        if(!person || person->id!=unit.person_id)return refuse(L::StaleUnitPerson);
        if(unit.flags&0x10000000u) {
            ++result.download_checks;const auto download=definitions.IsPersonDownload(*person);
            if(!download){result.status=L::UnknownPersonArchives;result.unavailable_slot=slot;return result;}
            if(!*download)continue;
        }
        if(unit.person_record==target){result.slot=slot;return result;}
    }
    return result;
}
PlayerUnitLookupResult NativePlayerUnitSelector::Find(const DefinitionStore& definitions,
    const NativeGameState& game) {
    using L=PlayerUnitLookupStatus;using A=ArchiveIdentifierStatus;
    PlayerUnitLookupResult result;
    if(!mask_) {
        // Literal SPID_Player from4F6430, read through the SAME shared index as
        // other archive labels. A host definition's numeric ID is not authority
        // for whichever value the actual first-hash lookup currently returns.
        const auto flag=identifiers_.ReadWord("SPID_\x83\x76\x83\x8c\x83\x43\x83\x84\x81\x5b");
        switch(flag.status) {
        case A::Ready:break;
        case A::Missing:result.status=L::MissingPlayerFlag;return result;
        case A::Retired:result.status=L::RetiredRegistry;return result;
        case A::StaleValue:result.status=L::StalePlayerFlag;return result;
        default:result.status=L::InvalidPlayerFlag;return result;
        }
        // The private-flag domain is64 bits. Invalid metadata is not a reason
        // to reproduce the vendor shift helper's out-of-domain register quirks.
        if(flag.value>=64){result.status=L::InvalidPlayerFlag;return result;}
        mask_=std::uint64_t{1}<<flag.value;
    }
    const auto bits=[](const std::array<std::uint8_t,8>& bytes) {
        std::uint64_t value{};
        for(unsigned i=0;i<8;++i)value|=std::uint64_t(bytes[i])<<(8*i);
        return value;
    };
    for(std::uint16_t slot=0;slot<game.units.size();++slot) {
        ++result.slots_examined;++result.uniqueness_checks;
        const auto& unit=game.units[slot];
        const auto refuse=[&](L status) {
            result.status=status;result.unavailable_slot=slot;return result;
        };
        // IsUnique precedes even the Force check. A prior out-of-force download
        // unit can therefore require its Person/archive owner before the match.
        if(unit.flags&0x40000u)continue;
        const PersonDefinition* person{};
        if(unit.flags&0x10000000u) {
            ++result.download_checks;
            if(!unit.person_record)return refuse(L::MissingPerson);
            person=definitions.ResolvePerson(unit.person_record);
            if(!person || person->id!=unit.person_id)return refuse(L::StalePerson);
            const auto download=definitions.IsPersonDownload(*person);
            if(!download)return refuse(L::UnknownPersonArchives);
            if(!*download)continue;
        }
        ++result.force_checks;
        if(unit.force_type>9)return refuse(L::InvalidUnit);
        // Force::GetMaskSameForceWithDefection(0), original literal39.
        if(!(0x39u&(1u<<unit.force_type)))continue;
        if(!unit.occupied)return refuse(L::InvalidUnit);
        ++result.flag_checks;
        if(!person) {
            if(!unit.person_record)return refuse(L::MissingPerson);
            person=definitions.ResolvePerson(unit.person_record);
            if(!person || person->id!=unit.person_id)return refuse(L::StalePerson);
        }
        const auto* job=definitions.FindJob(unit.job_id);
        if(!job)return refuse(L::MissingJob);
        if(!((bits(unit.private_skill_bits)|bits(person->bitflags)|bits(job->bitflags))&*mask_))continue;
        result.slot=slot;return result;
    }
    return result;
}
UnitClearPlan PlanUnitClearExact(UnitClearPresence f) noexcept {
    UnitClearPlan p{};auto add=[&](E e){p.effects[p.count++]=e;};
    if(f.edit){add(E::RemoveEditPrivateFlag);add(E::DeleteEdit);}
    if(f.family)add(E::DeleteFamily);if(f.actor)add(E::DestroyActor);
    if(f.ordinary_points)add(E::DeleteOrdinaryPoints);add(E::ClearOrdinaryCount);
    if(f.chapter_points)add(E::DeleteChapterPoints);add(E::ClearChapterCount);
    add(E::ResetFields);add(E::ClearIdentifier);add(E::ClearEnhance);
    add(E::ResetSkillPool);add(E::ClearAi);add(E::ClearCloth);add(E::ClearRecord);return p;
}
bool FreeUnitPoolMatches(const NativeGameState& s) noexcept {return Matches(s,std::nullopt);}
UnitPoolStatus RestoreFreeUnitPoolOrder(NativeGameState& s,std::span<const std::uint16_t> slots,std::uint32_t key) {
    if(s.free_unit_pool.bound)return S::AlreadyBound;
    auto next=std::make_unique<NativeGameState>(s);auto& pool=next->free_unit_pool;
    pool.bound=true;pool.clear_constructor_key=key;pool.order.bound=true;
    for(auto slot:slots)if(JoinForceOrderExact(pool.order,slot,0,true)!=ForceOrderStatus::Ok)return S::InvalidOrder;
    if(!FreeUnitPoolMatches(*next))return S::InvalidOrder;
    s.free_unit_pool=pool;return S::Ok;
}
UnitPoolStatus InitializeFreshUnitPool(NativeGameState& s,std::uint32_t key) {
    if(s.free_unit_pool.bound)return S::AlreadyBound;
    for(unsigned i=0;i<250;++i) {
        if(s.units[i].occupied||s.units[i].force_type!=9)return S::NotEmpty;
        if(!Available(s.unit_slot_generations[i]))return S::RevisionExhausted;
    }
    auto next=std::make_unique<NativeGameState>(s);auto& pool=next->free_unit_pool;
    pool={};pool.bound=true;pool.clear_constructor_key=key;pool.order.bound=true;
    next->player_force_order={};next->player_force_order.bound=true;
    for(auto& order:next->other_force_orders){order={};order.bound=true;}
    for(std::uint16_t i=0;i<250;++i) {
        ClearNativeUnit(next->units[i]);++next->unit_slot_generations[i];
        pool.cleared_constructor_keys[i]=key;
        if(JoinForceOrderExact(pool.order,i,0,true)!=ForceOrderStatus::Ok)return S::InvalidOrder;
    }
    s=std::move(*next);return S::Ok;
}
std::optional<std::uint16_t> FindFreeUnitPoolSlot(const NativeGameState& s) noexcept {
    if(s.free_unit_pool.bound) {
        if(!FreeUnitPoolMatches(s)||s.free_unit_pool.order.count==0)return std::nullopt;
        return s.free_unit_pool.order.slots[0];
    }
    // Historical unbound import compatibility, not a reconstructed free order.
    for(std::uint16_t i=0;i<250;++i)if(!s.units[i].occupied)return i;
    return std::nullopt;
}
UnitPoolStatus CompleteFreshUnitPoolSlot(NativeGameState& s,std::uint16_t slot) noexcept {
    if(slot>=250||!s.units[slot].occupied||s.units[slot].force_type!=9)return S::InvalidUnit;
    if(!Available(s.unit_slot_generations[slot]))return S::RevisionExhausted;
    if(s.free_unit_pool.bound) {
        auto& o=s.free_unit_pool.order;
        if(!Matches(s,slot)||!o.count||o.slots[0]!=slot)return S::InvalidOrder;
        if(RemoveForceOrderExact(o,slot)!=ForceOrderStatus::Ok)return S::InvalidOrder;
    }
    ++s.unit_slot_generations[slot];return S::Ok;
}
UnitPoolStatus RecycleUnitPoolSlot(NativeGameState& s,std::uint16_t slot,bool last) {
    if(slot>=250||!s.units[slot].occupied||s.units[slot].force_type>=9)return S::InvalidUnit;
    if(!s.free_unit_pool.bound)return S::UnboundPool;
    if(!FreeUnitPoolMatches(s))return S::InvalidOrder;
    const auto& u=s.units[slot];const auto force=u.force_type;
    if(!ForceOrderMatches(s,force))return S::MissingForceOrder;
    if(!Available(s.unit_slot_generations[slot]))return S::RevisionExhausted;
    if(!u.transfer.bound||u.transfer.person_id!=u.person_id||u.transfer.value.map_actor!=UnitMapActorPresence::Absent)return S::UnresolvedActor;
    if(u.pair.role!=PairRole::None||u.pair.partner_slot!=0xffffu||(u.flags&6u))return S::UnresolvedPair;
    for(unsigned i=0;i<250;++i)if(i!=slot&&s.units[i].occupied&&s.units[i].pair.partner_slot==slot)return S::UnresolvedPair;
    auto next=std::make_unique<NativeGameState>(s);
    auto& order=force==0?next->player_force_order:next->other_force_orders[force-1];
    if(RemoveForceOrderExact(order,slot)!=ForceOrderStatus::Ok)return S::MissingForceOrder;
    ClearNativeUnit(next->units[slot]);++next->unit_slot_generations[slot];
    next->free_unit_pool.cleared_constructor_keys[slot]=next->free_unit_pool.clear_constructor_key;
    if(JoinForceOrderExact(next->free_unit_pool.order,slot,0,last)!=ForceOrderStatus::Ok)return S::InvalidOrder;
    s=std::move(*next);return S::Ok;
}
}
