#pragma once
#include "fates/runtime/native_message_lookup.hpp"
#include <bitset>
#include <functional>

namespace fates::runtime::native {
// Original LogLine has room for 63 UTF-16 units plus a terminator. Allocation
// does not initialize unused text. Knownness is separate from physical zeroes.
struct TalkLogWords {
    std::array<char16_t,64> words{};
    std::bitset<64> known;
};
struct TalkLogLine {
    TalkLogWords text;
    std::uint8_t length{},dirty{1},name_line{};
    std::array<std::uint8_t,4> color{255,255,255,255};
};
struct TalkLogIdentity final {const std::uint64_t serial;};
using TalkLogHandle=std::shared_ptr<const TalkLogIdentity>;
struct TalkLogLineView {TalkLogHandle owner;std::uint8_t slot{};};
struct TalkLogSnapshot {
    TalkLogHandle identity;
    // Bytes +4/+5. Only recording is consulted by Append/NextLine/SetTalker;
    // do not infer viewer or completion policy from the other carried byte.
    std::uint8_t stream_flag{1},recording{1};
    std::array<TalkLogLine,64> lines;
    TalkLogWords talker;
    std::uint32_t current{},auxiliary{};
};
enum class TalkLogStatus:std::uint8_t {
    Ready,Absent,StaleSource,InvalidSource,UnknownWord,ReadLimit,IdentityExhausted
};
// Sources retain live identity, not a string snapshot masquerading as a pointer.
// A line view follows ring-slot reuse while the log lives; replacement/finalize
// retires it. Mess results follow the shared original buffer/archive owner.
struct TalkLogSource {
    enum class Kind:std::uint8_t {Null,Words,Talker,Line,Message,Reader};
    Kind kind{Kind::Null};
    std::shared_ptr<const std::vector<char16_t>> words;
    TalkLogHandle owner;
    std::uint8_t slot{};
    std::size_t offset{};
    std::shared_ptr<NativeMessageLookup> messages;
    MessageLookupResult message;
    // A borrowed source adapter keeps the concrete caller's live identity. It
    // does not own copied speaker text or acknowledge unavailable words as NUL.
    std::function<std::optional<char16_t>(std::size_t)> reader{};
    static TalkLogSource Text(std::u16string_view);
    static TalkLogSource Words(std::span<const char16_t>);
    static TalkLogSource Message(std::shared_ptr<NativeMessageLookup>,MessageLookupResult);
};
// Shared TalkLog/ILogStream data owner, independent of rendering. This does not
// create TalkLogViewer, a Talk process, or a successful dialogue/input wait.
class NativeTalkLog final {
public:
    NativeTalkLog()=default;
    NativeTalkLog(const NativeTalkLog&)=delete;
    NativeTalkLog& operator=(const NativeTalkLog&)=delete;
    // Initialize replaces an earlier log, not merely clears its current line.
    TalkLogStatus Initialize();
    void Finalize() noexcept;
    TalkLogStatus InitializeEveryTalk();
    TalkLogStatus Append(char16_t);
    TalkLogStatus AppendName(const TalkLogSource&);
    TalkLogStatus SetTalker(const TalkLogSource&);
    TalkLogStatus NextLine();
    TalkLogStatus DisableRecording();
    TalkLogStatus DisableStreamFlag();
    static constexpr std::uint32_t GetMaxLines() noexcept {return 64;}
    std::optional<std::uint32_t> GetCurrent() const noexcept;
    std::optional<TalkLogSnapshot> Observe() const;
    std::optional<TalkLogLineView> GetLine(std::uint32_t original_index) const noexcept;
    std::optional<TalkLogLine> ObserveLine(const TalkLogLineView&) const;
    TalkLogSource TalkerSource(std::size_t offset=0) const;
    TalkLogSource LineSource(const TalkLogLineView&,std::size_t offset=0) const;
    // Explicit carried state for already-owned allocation. Never changes its
    // identity or creates a singleton. Unknown words remain unknown; values
    // outside native safe storage are refused when the original read reaches them.
    TalkLogStatus RestoreCarried(const TalkLogSnapshot&);
private:
    std::optional<TalkLogSnapshot> state_;
    std::uint64_t serial_{};
    static void ResetLine(TalkLogLine&);
    static void AppendLocal(TalkLogSnapshot&,char16_t);
    static void NextLocal(TalkLogSnapshot&);
    static TalkLogStatus Read(const TalkLogSnapshot&,const TalkLogSource&,std::size_t,char16_t&);
    static TalkLogStatus AppendNameLocal(TalkLogSnapshot&,const TalkLogSource&);
};
}
