#include "fates/runtime/native_unit_names.hpp"
#include "fates/runtime/native_runtime.hpp"
#include "fates/runtime/native_unit_pool.hpp"
#include <algorithm>
#include <limits>

namespace fates::runtime::native {
namespace {
using S=UnitEditNameStatus;
S CheckEdit(const NativeRuntime& r,std::uint16_t slot) {
    if(slot>=r.game.units.size()||!r.game.units[slot].occupied)return S::InvalidUnit;
    const auto& u=r.game.units[slot];
    if(!u.lineage.bound)return S::UnboundLineage;
    if(u.lineage.person_id!=u.person_id)return S::StaleLineage;
    if(!u.lineage.value.edit)return S::MissingEdit;
    return S::Ready;
}
}
UnitEditNameResult ReadUnitEditName(const NativeRuntime& r,std::uint16_t slot) {
    const auto status=CheckEdit(r,slot);if(status!=S::Ready)return {status};
    const auto& name=r.game.units[slot].lineage.value.edit_name;
    if(!name)return {S::UnboundName};
    const auto end=std::find(name->begin(),name->end(),char16_t{});
    if(end==name->end())return {S::InvalidName};
    return {S::Ready,MessageLookupStatus::Ready,std::u16string(name->begin(),end)};
}
UnitEditNameResult SetUnitEditName(NativeRuntime& r,std::uint16_t slot,
    std::optional<std::u16string_view> input,NativeMessageLookup& messages,
    const NativeMessageLookup::PlayerNameResolver& resolver) {
    const auto status=CheckEdit(r,slot);if(status!=S::Ready)return {status};
    auto& state=r.game.units[slot].lineage;
    if(input&&(input->size()>1024u*1024u||input->find(char16_t{})!=std::u16string_view::npos))
        return {S::InvalidName};
    // Refuse before a default-message lookup can mutate the shared buffer.
    if(state.revision==std::numeric_limits<std::uint64_t>::max())return {S::RevisionExhausted};
    MessageLookupResult default_name;
    if(!input) {
        default_name=messages.Get("MPID_\x83\x66\x83\x74\x83\x48\x83\x8b\x83\x67\x96\xbc",resolver);
        if(default_name.status!=MessageLookupStatus::Ready&&default_name.status!=MessageLookupStatus::Missing)
            return {S::MessageUnavailable,default_name.status};
        // Original Mess returns a nonnull empty literal for a missing label.
        input=std::u16string_view(default_name.text);
    }
    std::array<char16_t,13> next{};
    std::copy_n(input->begin(),std::min(input->size(),next.size()-1),next.begin());
    if(state.value.edit_name!=next) {state.value.edit_name=next;++state.revision;}
    // Name-only changes do not invalidate current combat capabilities. The
    // shared lineage revision still invalidates retained source observations.
    return ReadUnitEditName(r,slot);
}

namespace {
using N=UnitNameStatus;using M=MessageLookupStatus;using A=ArchiveIdentifierStatus;
UnitNameResult Fail(N status,M message=M::DependencyUnavailable) {return {status,{message}};}
M ArchiveMessageStatus(A status) {
    switch(status) {
    case A::Retired:return M::RegistryRetired;
    case A::StaleValue:return M::StaleValue;
    case A::InvalidIdentifier:return M::InvalidIdentifier;
    default:return M::InvalidArchive;
    }
}
struct NameDepth {
    unsigned& depth;
    explicit NameDepth(unsigned& value):depth(value) {++depth;}
    ~NameDepth() {--depth;}
};
bool ValidIdentifier(std::string_view text) {
    return text.size()<=65535&&text.find('\0')==std::string_view::npos;
}
}
UnitNameResult NativeUnitNames::GetMessage(std::optional<std::string_view> identifier) {
    if(depth_>=64)return Fail(N::RecursionLimit,M::RecursionLimit);
    NameDepth guard(depth_);N nested=N::Ready;
    auto result=messages_.Get(identifier,[&]() {
        auto name=GetPlayerName();
        if(name.status!=N::Ready)nested=name.status;
        return name.message;
    });
    if(nested!=N::Ready)return {nested,std::move(result)};
    const auto status=result.status==M::Ready||result.status==M::Missing?N::Ready:N::MessageUnavailable;
    return {status,std::move(result)};
}
UnitNameResult NativeUnitNames::GetPlayerName() {
    ++counters_.player_queries;
    const auto player=player_.Find(runtime_.definitions,runtime_.game);
    if(player.status!=PlayerUnitLookupStatus::Ready)return Fail(N::PlayerUnavailable);
    if(!player.slot)return {N::Ready,{M::Missing}};
    auto result=GetUnit(*player.slot);
    // Mess's missing-name result is a NONNULL empty literal. Only no player
    // above means a null name dependency eligible for MakeArgedMess's fallback.
    if(result.status==N::Ready&&result.message.status==M::Missing)result.message.status=M::Ready;
    return result;
}
UnitNameResult NativeUnitNames::GetUnit(std::uint16_t slot) {
    if(depth_>=64)return Fail(N::RecursionLimit,M::RecursionLimit);
    NameDepth guard(depth_);++counters_.unit_names;
    if(slot>=runtime_.game.units.size()||!runtime_.game.units[slot].occupied)return Fail(N::InvalidUnit);
    const auto& unit=runtime_.game.units[slot];
    // The replica flag precedes every Edit/Person/current-name dependency.
    if(unit.flags&0x40000u)return GetMessage("MPID_\x8e\xca\x82\xb5\x90\x67");
    if(!unit.lineage.bound)return Fail(N::UnknownEdit);
    if(unit.lineage.person_id!=unit.person_id)return Fail(N::StaleLineage);
    if(unit.lineage.value.edit) {
        const auto name=ReadUnitEditName(runtime_,slot);
        if(name.status==S::UnboundName)return Fail(N::UnknownEditName);
        if(name.status!=S::Ready)return Fail(N::InvalidEditName,M::InvalidText);
        return {N::Ready,{M::Ready,MessageLookupOrigin::Empty,name.text},UnitEditNameReference{this,slot,unit.person_id,runtime_.game.unit_slot_generations[slot]}};
    }
    if(!unit.capture_name_index)return Fail(N::UnknownCaptureName);
    if(!unit.person_record)return Fail(N::MissingPerson);
    const auto* person=runtime_.definitions.ResolvePerson(unit.person_record);
    if(!person||person->id!=unit.person_id)return Fail(N::StalePerson);
    return GetPerson(unit.person_record,*unit.capture_name_index);
}
UnitNameResult NativeUnitNames::GetPerson(const PersonRecordReference& reference,std::int32_t capture) {
    if(depth_>=64)return Fail(N::RecursionLimit,M::RecursionLimit);
    NameDepth guard(depth_);++counters_.person_names;
    const auto* person=runtime_.definitions.ResolvePerson(reference);
    if(!person)return Fail(N::StalePerson);
    const auto download=runtime_.definitions.IsPersonDownload(*person);
    if(!download)return Fail(N::UnknownPersonArchives);
    if(!person->name_message_present)return Fail(N::UnknownPersonStrings);
    if(!*person->name_message_present&&!person->name_message.empty())return Fail(N::InvalidPersonStrings);
    const auto check_fid=[&]() {
        if(!person->fid_present)return N::UnknownPersonStrings;
        return !*person->fid_present&&!person->fid.empty()?N::InvalidPersonStrings:N::Ready;
    };
    const auto fid=[&]()->std::optional<std::string_view> {
        return *person->fid_present?std::optional<std::string_view>{person->fid}:std::nullopt;
    };
    if(*download) {
        if(*person->name_message_present) {
            const auto found=identifiers_.Find(person->name_message);
            if(found.status==A::Missing)return GetMessage("MPID_\x95\x73\x96\xbe");
            if(found.status!=A::Ready)return Fail(N::ArchiveUnavailable,ArchiveMessageStatus(found.status));
        } else {
            const auto known=check_fid();if(known!=N::Ready)return Fail(known);
            if(*person->fid_present) {
                const auto face=FindFace(fid(),0);
                if(face.status!=N::Ready)return Fail(face.status,face.archive_status==A::Ready?M::DependencyUnavailable:ArchiveMessageStatus(face.archive_status));
                if(!face.value.registration)return GetMessage("MPID_\x95\x73\x96\xbe");
            }
        }
    }
    if(*person->name_message_present)return GetMessage(person->name_message);
    if(capture>0) {
        std::string key="MKID_\x95\xdf\x97\xb8_000";
        for(unsigned i=0;i<3;++i) {key[key.size()-1-i]=char('0'+capture%10);capture/=10;}
        return GetMessage(key);
    }
    const auto known=check_fid();if(known!=N::Ready)return Fail(known);
    const auto face=FindFace(fid(),0);
    if(face.status!=N::Ready)return Fail(face.status,face.archive_status==A::Ready?M::DependencyUnavailable:ArchiveMessageStatus(face.archive_status));
    if(!face.value.registration)return {N::Ready,{M::Ready,MessageLookupOrigin::Empty,u"Unknown"}};
    return GetFaceMessage(face.value);
}
FaceNameLookupResult NativeUnitNames::FindFace(std::optional<std::string_view> fid,std::int32_t type) {
    ++counters_.face_finds;
    if(fid&&!ValidIdentifier(*fid))return {N::InvalidPersonStrings,A::InvalidIdentifier};
    std::string name=fid?std::string(*fid).substr(0,63):std::string{};
    if(fid&&*fid=="FID_username") {
        name.clear();++counters_.player_queries;
        const auto player=player_.Find(runtime_.definitions,runtime_.game);
        if(player.status!=PlayerUnitLookupStatus::Ready)return {N::PlayerUnavailable};
        if(player.slot) {
            const auto& unit=runtime_.game.units[*player.slot];
            if(!unit.lineage.bound)return {N::UnknownEdit};
            if(unit.lineage.person_id!=unit.person_id)return {N::StaleLineage};
            if(unit.lineage.value.edit) {
                if(!unit.lineage.value.edit_face)return {N::UnknownAppearance};
                const auto& edit=*unit.lineage.value.edit_face;
                const auto index=unsigned(edit.gender)*2+edit.body_type;
                // Original pointer table6BDC00 has four observed nonnull
                // entries then nulls. Preserve aliases such as0/2 ->2; wider
                // table access is explicitly unavailable, never clamped.
                constexpr std::array<std::string_view,4> body{
                    "\x92\x6a""1","\x92\x6a""2","\x8f\x97""1","\x8f\x97""2"};
                if(index>=body.size())return {N::UnsupportedFaceTemplate};
                name="FID_\x83\x7d\x83\x43\x83\x86\x83\x6a_";
                name+=body[index];name+="_\x8a\xe7";
                const auto letter=static_cast<unsigned char>(unsigned(edit.face_index)+0x41);
                if(letter)name+=char(letter); // percent-c may itself write NUL.
            }
        }
        if(name.empty())name="FID_\x83\x7d\x83\x43\x83\x86\x83\x6a_\x93\xb6\x8a\xe7_\x8a\xe7\x97\xa7\x82\xbf""A";
    }
    // Original zeroed local buffer and null-tolerant strncpy make short/null
    // FIDs have an empty suffix. The first four bytes are skipped unconditionally.
    auto key=std::string("FSID_")+(type==1?"ST_":"BU_")+(name.size()>4?name.substr(4):std::string{});
    if(key.size()>63)key.resize(63);
    auto found=identifiers_.Find(key);
    if(found.status!=A::Ready&&found.status!=A::Missing)
        return {N::ArchiveUnavailable,found.status,{},std::move(key)};
    return {N::Ready,found.status,std::move(found.value),std::move(key)};
}
UnitNameResult NativeUnitNames::GetFaceMessage(const ArchiveIdentifierValue& face) {
    if(depth_>=64)return Fail(N::RecursionLimit,M::RecursionLimit);
    NameDepth guard(depth_);++counters_.face_names;
    const auto name=identifiers_.ReadStringField(face,4);
    if(name.status!=A::Ready)return Fail(N::InvalidFaceData,ArchiveMessageStatus(name.status));
    // Actual helper is strncmp2(name,literal), comparing strlen(literal).
    // A suffix after MPID_username still selects the player branch.
    if(name.text&&name.text->starts_with("MPID_username")) {
        ++counters_.player_queries;
        const auto first=player_.Find(runtime_.definitions,runtime_.game);
        if(first.status!=PlayerUnitLookupStatus::Ready)return Fail(N::PlayerUnavailable);
        if(first.slot) {
            ++counters_.player_queries;
            const auto second=player_.Find(runtime_.definitions,runtime_.game);
            if(second.status!=PlayerUnitLookupStatus::Ready||!second.slot)return Fail(N::PlayerUnavailable);
            return GetUnit(*second.slot);
        }
    }
    return GetMessage(name.text?std::optional<std::string_view>{*name.text}:std::nullopt);
}
}

namespace fates::runtime::native {
MessageSourceWord NativeUnitNames::ReadEditSourceWord(const UnitEditNameReference& source,std::size_t index) const {
    if(source.owner!=this || source.slot>=runtime_.game.units.size() || source.generation!=runtime_.game.unit_slot_generations[source.slot])return {MessageLookupStatus::StaleValue,0};
    const auto& unit=runtime_.game.units[source.slot];
    if(!unit.occupied || unit.person_id!=source.person_id || CheckEdit(runtime_,source.slot)!=UnitEditNameStatus::Ready)return {MessageLookupStatus::StaleValue,0};
    if(!unit.lineage.value.edit_name || index>=13)return {MessageLookupStatus::InvalidIndex,0};
    return {MessageLookupStatus::Ready,(*unit.lineage.value.edit_name)[index]};
}
}
