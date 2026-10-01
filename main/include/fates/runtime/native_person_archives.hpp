#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
namespace fates::runtime::native {
// Tokens identify loaded native archive instances, independently of Person IDs.
struct PersonArchiveDescriptor {
    std::uint32_t token{};
    std::optional<std::string> name{};
    std::uint16_t first_id{},count{};
    std::uint8_t resident{};
    bool has_support{},has_owner{};
    bool operator==(const PersonArchiveDescriptor&) const=default;
};
struct PersonArchiveLocation {
    std::uint32_t token{},record_index{};
    bool operator==(const PersonArchiveLocation&) const=default;
};
// Semantic identity of one original Person record. Archive tokens alone are
// local to a DefinitionStore and can collide after whole-store replacement.
// Copies retain the instance identity; resolution still requires that instance
// to be present in the particular store being read. No Person pointer survives
// vector relocation, and retaining a reference does not keep an archive loaded.
class PersonRecordReference final {
public:
    PersonRecordReference()=default;
    explicit operator bool() const noexcept {return bool(instance_);}
    PersonArchiveLocation location() const noexcept {return location_;}
    bool operator==(const PersonRecordReference&) const=default;
private:
    friend class DefinitionStore;
    struct Instance {};
    std::shared_ptr<const Instance> instance_;
    PersonArchiveLocation location_;
};
std::optional<std::size_t> FindPersonArchiveExact(std::span<const PersonArchiveDescriptor>,std::uint16_t) noexcept;
std::optional<PersonArchiveLocation> FindPersonRecordExact(std::span<const PersonArchiveDescriptor>,std::uint16_t,bool fallback_to_first=false) noexcept;
bool PersonIsResidentExact(std::span<const PersonArchiveDescriptor>,std::uint16_t) noexcept;
// A missing head is an invalid IsResidentFirst dereference shape.
std::optional<bool> PersonIsResidentFirstExact(std::span<const PersonArchiveDescriptor>,std::uint16_t) noexcept;
bool PersonIsDownloadExact(std::span<const PersonArchiveDescriptor>,std::uint16_t,std::uint64_t private_flags) noexcept;
enum class PersonArchiveOperation : std::uint8_t {Initialize,Append,Detach,Free,Prune,Finalize};
enum class PersonArchiveStatus : std::uint8_t {Ok,InvalidState,InvalidToken,UnresolvedName,InvalidCursor};
enum class PersonArchiveEventKind : std::uint8_t {InitializeSupport=1,LoadSupport,FreeSupport,ReleaseNode,DestroyOwner,FinalizeSupport};
struct PersonArchiveEvent {
    PersonArchiveEventKind kind{};
    std::uint32_t token{};
    std::vector<std::uint32_t> visible_archives{};
    bool operator==(const PersonArchiveEvent&) const=default;
};
struct PersonArchivePlan {
    PersonArchiveStatus status{PersonArchiveStatus::Ok};
    std::vector<PersonArchiveDescriptor> archives{};
    std::vector<PersonArchiveEvent> events{};
};
// Original ordered list transitions with successful allocation and resolved
// archive payloads. Effects record original call-time list visibility. Undefined
// null-name/cursor shapes refuse atomically. File IO/support internals external.
PersonArchivePlan PlanPersonArchivesExact(std::span<const PersonArchiveDescriptor>,
    PersonArchiveOperation,const PersonArchiveDescriptor& incoming={},std::string_view name={});
}
