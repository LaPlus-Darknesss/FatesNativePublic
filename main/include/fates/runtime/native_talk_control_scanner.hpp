#pragma once
#include "fates/runtime/native_talk_text.hpp"

namespace fates::runtime::native {
enum class TalkCodeStatus:std::uint8_t {Ready,InvalidSource,InvalidCode,ReadLimit,Overflow};
enum class TalkCodeOperation:std::uint8_t {Dispose,Flash,Skip};
struct TalkCodeRoute {
    char16_t command{};
    std::uint32_t original_vtable{},original_target{};
    bool known_handler{};
};
struct TalkCodeResult {
    TalkCodeStatus status{TalkCodeStatus::InvalidSource};
    TalkCodeRoute route;
    // null is the real TcdiNone::Skip null result, not a zero-length success.
    std::optional<std::size_t> next;
    std::size_t words_read{};
};
// The exact read-only TalkCodeDisposer skip dispatch and its twenty handlers.
// Uses the SAME NativeTalkTokens object as the Talk expander; no second token
// buffer. Resolve identifies original actions but does not execute Dispose,
// Flash, input, audio, effects, window changes or any ProcTalkManager callback.
// Reader must be a serialized read-only view of an actual owner/known extent.
class NativeTalkControlScanner final {
public:
    explicit NativeTalkControlScanner(NativeTalkTokens& tokens):tokens_(tokens){}
    using Reader=NativeTalkTokens::Reader;
    static std::optional<TalkCodeRoute> Route(char16_t,TalkCodeOperation);
    TalkCodeResult Skip(const Reader&,std::size_t start=0);
    TalkCodeResult Skip(std::span<const char16_t>,std::size_t start=0);
    TalkCodeResult Skip(const NativeMessageLookup&,const MessageLookupResult&,std::size_t start=0);
    TalkCodeResult Skip(const NativeTalkText&,std::size_t start=0);
private:
    NativeTalkTokens& tokens_;
};
}
