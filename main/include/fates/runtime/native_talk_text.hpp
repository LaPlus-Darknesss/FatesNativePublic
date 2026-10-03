#pragma once
#include "fates/runtime/native_unit_names.hpp"
#include "fates/runtime/native_unit_pool.hpp"
#include <bitset>

namespace fates::runtime::native {
enum class TalkTextStatus : std::uint8_t {
    Ready,InvalidUnit,StalePlayer,PlayerUnavailable,UnknownEdit,StaleEdit,MissingEdit,
    UnknownName,UnknownAppearance,UnsupportedTone,MessageUnavailable,ArchiveUnavailable,
    InvalidIdentifier,InvalidSource,UnknownWord,BufferOverflow,UnboundCursor,InvalidBuffer,Retired
};
struct TalkPlayerResult {
    TalkTextStatus status{TalkTextStatus::Ready};
    std::optional<std::uint16_t> slot;
};
struct TalkEditResult {
    TalkTextStatus status{TalkTextStatus::Ready};
    bool present{};
    std::optional<std::array<char16_t,13>> name;
    std::optional<UnitEditFaceState> face;
};
// One fresh TalkUtil global scope. Override null is the original initial/default
// state, not a known absent player. A retained override guards Unit slot reuse.
// This does not change the shared UnitPool selector or ordinary Mess name policy.
class NativeTalkPlayer final {
public:
    NativeTalkPlayer(const NativeRuntime& runtime,NativePlayerUnitSelector& player)
        :runtime_(runtime),player_(player) {}
    TalkTextStatus SetPlayer(std::optional<std::uint16_t>);
    void SetDefault() noexcept {override_.reset();}
    TalkPlayerResult Find();
    TalkEditResult Edit(std::uint16_t) const;
    TalkEditResult CurrentEdit();
    std::uint64_t pool_queries() const noexcept {return pool_queries_;}
private:
    struct Override {std::uint16_t slot;std::uint64_t generation;};
    const NativeRuntime& runtime_;
    NativePlayerUnitSelector& player_;
    std::optional<Override> override_;
    std::uint64_t pool_queries_{};
};
struct TalkTokenResult {
    TalkTextStatus status{TalkTextStatus::Ready};
    std::u16string text;
    bool shared_buffer{};
};
// One shared original GetToken buffer per fresh Talk scope. Empty/delimiter
// input returns a separate empty literal and leaves all65 static words intact.
class NativeTalkTokens final {
public:
    TalkTokenResult Get(std::optional<std::span<const char16_t>>);
    struct SkipResult {TalkTextStatus status;std::size_t words;};
    SkipResult Skip(std::span<const char16_t>);
    std::array<char16_t,65> snapshot() const noexcept {return words_;}
    // GetFidForWindow's inlined scanner uses a DISTINCT static array7531F4,
    // not GetToken's752FE0 buffer. Both remain in this one process-scoped owner.
    std::array<char16_t,65> window_snapshot() const noexcept {return window_words_;}
    // Existing lazy reader overloads are shared with the control scanner. Their
    // implementation and static-buffer semantics remain unchanged.
    using Reader=std::function<std::optional<char16_t>(std::size_t)>;
    TalkTokenResult Get(const Reader&,std::size_t);
    SkipResult Skip(const Reader&,std::size_t);
private:
    friend class NativeTalkText;
    friend struct TalkWindowArgumentState;
    std::array<char16_t,65> words_{};
    std::array<char16_t,65> window_words_{};
};
struct TalkTextBuffer {
    std::array<char16_t,8192> words{};
    // The constructor allocates but does not initialize original words.
    std::bitset<8192> known;
    std::optional<std::size_t> cursor;
};
struct TalkMessageSelection {
    TalkTextStatus status{TalkTextStatus::Ready};
    MessageLookupResult source;
    std::string identifier;
};
struct TalkTextObservation {
    TalkTextStatus status{TalkTextStatus::Ready};
    std::u16string text;
};
// ProcTalkManager's message-selection prefix and its retained TalkExpander.
// This prepares text only; process/window/log/input completion is not supplied.
// All referenced owners must share one runtime scope and outlive this owner.
class NativeTalkText final {
public:
    NativeTalkText(const NativeArchiveIdentifiers& identifiers,NativeMessageLookup& messages,
        NativeUnitNames& names,NativeTalkPlayer& player,NativeTalkTokens& tokens)
        :identifiers_(identifiers),messages_(messages),names_(names),player_(player),tokens_(tokens) {}
    TalkMessageSelection SelectMessage(std::string_view identifier);
    TalkTextStatus PrepareMessage(std::string_view identifier);
    TalkTextStatus ExpandMessage(const MessageLookupResult&);
    TalkTextStatus ExpandIdentifier(std::optional<std::string_view>);
    // A separate immutable supplied source, with its complete admitted extent.
    // NULs/trailing words are retained for original bounded token reads.
    TalkTextStatus ExpandWords(std::optional<std::span<const char16_t>>);
    TalkTextStatus ExpandText(std::optional<std::u16string_view>);
    // Explicit original alias into this expander's own live output allocation.
    TalkTextStatus ExpandCurrent(std::size_t offset=0);
    // Same admitted expander, with an external live reader supplied by an owner.
    // Own-output aliases MUST use ExpandCurrent so writes see the working output.
    TalkTextStatus ExpandReader(const NativeTalkTokens::Reader&);
    bool UsesTokens(const NativeTalkTokens& value) const noexcept {return &tokens_==&value;}

    TalkTextStatus RestoreBuffer(std::span<const char16_t>,std::optional<std::size_t> cursor={});
    bool live() const noexcept {return !retired_;}
    // Original embedded expander destruction retires its allocation even while
    // a host shared_ptr to the source remains. No retained view revives it.
    void Retire() noexcept {retired_=true;buffer_.known.reset();buffer_.cursor.reset();}
    const TalkTextBuffer& buffer() const noexcept {return buffer_;}
    TalkTextObservation Observe() const;
private:
    using Reader=NativeTalkTokens::Reader;
    TalkTextStatus Expand(const std::function<Reader(TalkTextBuffer&)>&,bool null_source);
    const NativeArchiveIdentifiers& identifiers_;
    NativeMessageLookup& messages_;
    NativeUnitNames& names_;
    NativeTalkPlayer& player_;
    NativeTalkTokens& tokens_;
    TalkTextBuffer buffer_;
    bool retired_{};
};
}
