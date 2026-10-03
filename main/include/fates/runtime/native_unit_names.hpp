#pragma once
#include "fates/runtime/native_message_lookup.hpp"
#include "fates/runtime/native_person_archives.hpp"

namespace fates::runtime::native {
struct NativeRuntime;
enum class UnitEditNameStatus : std::uint8_t {
    Ready,InvalidUnit,UnboundLineage,StaleLineage,MissingEdit,UnboundName,
    InvalidName,MessageUnavailable,RevisionExhausted
};
struct UnitEditNameResult {
    UnitEditNameStatus status{UnitEditNameStatus::Ready};
    MessageLookupStatus message_status{MessageLookupStatus::Ready};
    std::u16string text;
};
// Read a stable observation of the current carried Edit name. Presence is
// owned by UnitLineageSnapshot; this does not create a separate dialogue Edit.
UnitEditNameResult ReadUnitEditName(const NativeRuntime&,std::uint16_t slot);
// Original41967C SetName plus3A6ADC zero-fill. Null obtains the actual default
// message through the shared Mess owner; nonnull empty is an empty name.
// This edits an already present carried Edit. It does not claim CreateEdit,
// SetDefault, the remaining appearance fields, or serialized save import.
UnitEditNameResult SetUnitEditName(NativeRuntime&,std::uint16_t slot,
    std::optional<std::u16string_view>,NativeMessageLookup&,
    const NativeMessageLookup::PlayerNameResolver& ={});

class NativePlayerUnitSelector;
enum class UnitNameStatus : std::uint8_t {
    Ready,InvalidUnit,UnknownEdit,StaleLineage,UnknownEditName,InvalidEditName,
    UnknownCaptureName,MissingPerson,StalePerson,UnknownPersonArchives,
    UnknownPersonStrings,InvalidPersonStrings,UnknownAppearance,UnsupportedFaceTemplate,
    ArchiveUnavailable,InvalidFaceData,PlayerUnavailable,MessageUnavailable,RecursionLimit
};
class NativeUnitNames;
struct UnitEditNameReference {
    const NativeUnitNames* owner{};std::uint16_t slot{},person_id{};std::uint64_t generation{};
};
struct UnitNameResult {
    UnitNameStatus status{UnitNameStatus::Ready};
    MessageLookupResult message;
    std::optional<UnitEditNameReference> edit_source{};
};
struct FaceNameLookupResult {
    UnitNameStatus status{UnitNameStatus::Ready};
    ArchiveIdentifierStatus archive_status{ArchiveIdentifierStatus::Ready};
    ArchiveIdentifierValue value; // Ready with no registration means known absent.
    std::string key;
};
struct UnitNameCounters {
    std::uint64_t unit_names{},person_names{},face_finds{},face_names{},player_queries{};
};
// Serialized original Unit/Person/FaceData name policy. All dependencies are
// shared current owners; no roster/name fixture or renderer authority is used.
// References must outlive this owner. Returned strings are stable observations;
// shared Mess-buffer identity is preserved for nested expansion aliasing.
class NativeUnitNames final {
public:
    const NativeRuntime& runtime() const noexcept {return runtime_;}
    NativeUnitNames(const NativeRuntime& runtime,const NativeArchiveIdentifiers& identifiers,
        NativeMessageLookup& messages,NativePlayerUnitSelector& player)
        :runtime_(runtime),identifiers_(identifiers),messages_(messages),player_(player) {}
    UnitNameResult GetUnit(std::uint16_t slot);
    UnitNameResult GetPerson(const PersonRecordReference&,std::int32_t capture_index);
    FaceNameLookupResult FindFace(std::optional<std::string_view> fid,std::int32_t type);
    UnitNameResult GetFaceMessage(const ArchiveIdentifierValue&);
    UnitNameResult GetPlayerName();
    UnitNameResult GetMessage(std::optional<std::string_view> identifier);
    MessageSourceWord ReadEditSourceWord(const UnitEditNameReference&,std::size_t) const;
    UnitNameCounters counters() const noexcept {return counters_;}
    bool UsesMessages(const NativeMessageLookup& owner) const noexcept {return &messages_==&owner;}
private:
    const NativeRuntime& runtime_;
    const NativeArchiveIdentifiers& identifiers_;
    NativeMessageLookup& messages_;
    NativePlayerUnitSelector& player_;
    unsigned depth_{};
    UnitNameCounters counters_{};
};
}
