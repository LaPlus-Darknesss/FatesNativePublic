#pragma once
#include "fates/runtime/native_archive_identifiers.hpp"
#include "fates/runtime/native_message_archive.hpp"
#include <array>
#include <functional>

namespace fates::runtime::native {
enum class MessageLookupStatus : std::uint8_t {
    Ready, Missing, InvalidIdentifier, RegistryRetired, StaleValue,
    InvalidArchive, InvalidBuffer, BufferTooSmall, DependencyUnavailable,
    InvalidText, InvalidIndex, RecursionLimit, Busy
};
enum class MessageLookupOrigin : std::uint8_t { Empty, Archive, Buffer };
// Owner identity for results that originally point into the shared Mess buffer.
// The text field below remains an immutable observation. Expansion alone uses
// this identity to follow live aliasing after a nested player-name lookup.
class MessageBufferIdentity final {};
struct MessageLookupResult {
    MessageLookupStatus status{MessageLookupStatus::Missing};
    MessageLookupOrigin origin{MessageLookupOrigin::Empty};
    // A stable value observation, never a borrowed view into the mutable buffer.
    std::u16string text;
    std::shared_ptr<const ArchiveRegistration> registration;
    std::size_t payload_offset{};
    std::shared_ptr<const MessageBufferIdentity> buffer_identity;
};
struct MessageSourceWord {
    MessageLookupStatus status{MessageLookupStatus::InvalidText};
    char16_t value{};
};
// Serialized Mess text ownership over the shared original identifier namespace.
// The player-name resolver is the still-explicit UnitPool::GetPlayer / GetName
// dependency, not a save-format or renderer input. Missing means known null and
// takes the original default-name lookup; no resolver means unknown. A resolver
// may call Get on this same owner, preserving the original shared-buffer alias.
// It must not mutate argument/link state or external gameplay state.
class NativeMessageLookup final {
public:
    using PlayerNameResolver=std::function<MessageLookupResult()>;
    NativeMessageLookup(const NativeArchiveIdentifiers&,std::size_t buffer_capacity);
    NativeMessageLookup(const NativeMessageLookup&)=delete;
    NativeMessageLookup& operator=(const NativeMessageLookup&)=delete;
    MessageLookupResult Get(std::optional<std::string_view> identifier,
        const PlayerNameResolver& player_name={});
    MessageLookupResult GetNoReplace(std::optional<std::string_view> identifier);
    // Follow the actual result source after subsequent Mess calls. Archive
    // reads validate current registration/lifetime; buffer reads require this
    // owner's identity and observe live words, not the result's text snapshot.
    MessageSourceWord ReadSourceWord(const MessageLookupResult&,std::size_t index) const;
    MessageLookupStatus SetArgument(std::int32_t index,std::optional<std::u16string_view> text);
    MessageLookupStatus SetArgument(std::int32_t index,std::int32_t value);
    MessageLookupStatus SetLinkName(std::int32_t index,std::optional<std::u16string_view> text);
    std::span<const char16_t> argument_words(std::int32_t index) const noexcept;
    std::optional<std::size_t> argument_buffer_size(std::int32_t index) const noexcept;
    std::span<const char16_t> link_words(std::int32_t index) const noexcept;
    std::span<const char16_t> buffer_words() const noexcept {return buffer_;}
private:
    MessageLookupResult GetInternal(std::optional<std::string_view>,const PlayerNameResolver&);
    MessageLookupStatus Expand(std::u16string_view,std::size_t,const PlayerNameResolver&);
    MessageLookupResult BufferResult() const;
    const NativeArchiveIdentifiers& identifiers_;
    std::vector<char16_t> buffer_;
    std::array<std::array<char16_t,64>,4> arguments_{};
    std::array<std::array<char16_t,13>,2> links_{};
    std::shared_ptr<const MessageBufferIdentity> buffer_identity_{std::make_shared<MessageBufferIdentity>()};
    unsigned expansion_depth_{};
};
}
