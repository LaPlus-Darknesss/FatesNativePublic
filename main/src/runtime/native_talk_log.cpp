#include "fates/runtime/native_talk_log.hpp"
#include <limits>

namespace fates::runtime::native {
namespace {
using S=TalkLogStatus;
constexpr std::size_t ReadBudget=65536; // Explicit admission bound, not retail truncation.
void Write(TalkLogWords& text,std::size_t index,char16_t value) {
    text.words[index]=value;text.known.set(index);
}
TalkLogLine& Current(TalkLogSnapshot& state) {return state.lines[state.current&63u];}
}
TalkLogSource TalkLogSource::Text(std::u16string_view text) {
    std::vector<char16_t> words(text.begin(),text.end());words.push_back(0);return Words(words);
}
TalkLogSource TalkLogSource::Words(std::span<const char16_t> input) {
    TalkLogSource out;out.kind=Kind::Words;
    out.words=std::make_shared<const std::vector<char16_t>>(input.begin(),input.end());return out;
}
TalkLogSource TalkLogSource::Message(std::shared_ptr<NativeMessageLookup> owner,MessageLookupResult result) {
    TalkLogSource out;out.kind=Kind::Message;out.messages=std::move(owner);out.message=std::move(result);return out;
}
void NativeTalkLog::ResetLine(TalkLogLine& line) {
    Write(line.text,0,0);line.length=0;line.dirty=1;line.name_line=0;line.color={255,255,255,255};
}
TalkLogStatus NativeTalkLog::Initialize() {
    if(serial_==std::numeric_limits<std::uint64_t>::max())return S::IdentityExhausted;
    // Old views must become stale, including when host allocation throws.
    state_.reset();TalkLogSnapshot next;
    next.identity=std::make_shared<const TalkLogIdentity>(TalkLogIdentity{++serial_});
    for(auto& line:next.lines)ResetLine(line);
    Write(next.talker,0,0);state_=std::move(next);return S::Ready;
}
void NativeTalkLog::Finalize() noexcept {state_.reset();}
TalkLogStatus NativeTalkLog::InitializeEveryTalk() {
    // This is a global null shortcut; unlike instance methods absence is valid.
    if(!state_)return S::Ready;
    Write(state_->talker,0,0);state_->stream_flag=1;state_->recording=1;
    ResetLine(Current(*state_));return S::Ready;
}
void NativeTalkLog::NextLocal(TalkLogSnapshot& state) {
    if(!state.recording)return;
    ++state.current;state.auxiliary=0;auto& line=Current(state);ResetLine(line);
    // Original reset writes opaque white first, then NextLine changes alpha to0.
    line.color[3]=0;
}
TalkLogStatus NativeTalkLog::NextLine() {if(!state_)return S::Absent;NextLocal(*state_);return S::Ready;}
void NativeTalkLog::AppendLocal(TalkLogSnapshot& state,char16_t value) {
    if(!state.recording)return;
    if(value==u'\n'){NextLocal(state);return;}
    auto& line=Current(state);
    if(line.length<=62) {
        Write(line.text,line.length,value);
        ++line.length;Write(line.text,line.length,0);
    }
    // Full lines are not wrapped automatically. Even a dropped unit marks dirty.
    line.dirty=1;
}
TalkLogStatus NativeTalkLog::Append(char16_t value) {if(!state_)return S::Absent;AppendLocal(*state_,value);return S::Ready;}
TalkLogStatus NativeTalkLog::Read(const TalkLogSnapshot& state,const TalkLogSource& source,
    std::size_t index,char16_t& value) {
    if(index>std::numeric_limits<std::size_t>::max()-source.offset)return S::InvalidSource;
    const auto at=source.offset+index;
    if(source.kind==TalkLogSource::Kind::Words) {
        if(!source.words || at>=source.words->size())return S::InvalidSource;
        value=(*source.words)[at];return S::Ready;
    }
    if(source.kind==TalkLogSource::Kind::Message) {
        if(!source.messages)return S::InvalidSource;
        const auto word=source.messages->ReadSourceWord(source.message,at);
        if(word.status!=MessageLookupStatus::Ready)return S::StaleSource;
        value=word.value;return S::Ready;
    }
    if(source.kind==TalkLogSource::Kind::Reader) {
        const auto word=source.reader?source.reader(at):std::nullopt;
        if(!word)return S::UnknownWord;
        value=*word;return S::Ready;
    }
    if(source.kind!=TalkLogSource::Kind::Line && source.kind!=TalkLogSource::Kind::Talker)return S::InvalidSource;
    if(!source.owner || source.owner!=state.identity)return S::StaleSource;
    if(at>=64 || source.slot>=64)return S::InvalidSource;
    const auto& text=source.kind==TalkLogSource::Kind::Talker?state.talker:state.lines[source.slot].text;
    if(!text.known.test(at))return S::UnknownWord;
    value=text.words[at];return S::Ready;
}
TalkLogStatus NativeTalkLog::AppendNameLocal(TalkLogSnapshot& state,const TalkLogSource& source) {
    if(!state.recording)return S::Ready; // Do not inspect even a null/stale source.
    for(std::size_t index=0;index<ReadBudget;++index) {
        char16_t value{};if(const auto s=Read(state,source,index,value);s!=S::Ready)return s;
        if(!value) {
            auto& line=Current(state);line.dirty=0;line.color={200,200,0,0};line.name_line=1;
            NextLocal(state);return S::Ready;
        }
        AppendLocal(state,value);
    }
    return S::ReadLimit;
}
TalkLogStatus NativeTalkLog::AppendName(const TalkLogSource& source) {
    if(!state_)return S::Absent;
    auto next=*state_;const auto status=AppendNameLocal(next,source);
    if(status==S::Ready)state_=std::move(next);
    return status;
}
TalkLogStatus NativeTalkLog::SetTalker(const TalkLogSource& source) {
    if(!state_)return S::Absent;
    if(!state_->recording)return S::Ready;
    auto next=*state_;
    if(source.kind==TalkLogSource::Kind::Null) {
        // Original null uses a 128-byte zero-fill, not InitializeEveryTalk's
        // one-terminator reset and not an empty name-line insertion.
        next.talker.words.fill(0);next.talker.known.set();state_=std::move(next);return S::Ready;
    }
    bool equal=false;
    for(std::size_t index=0;index<64;++index) {
        if(!next.talker.known.test(index))return S::UnknownWord;
        char16_t value{};if(const auto s=Read(next,source,index,value);s!=S::Ready)return s;
        if(next.talker.words[index]!=value)break;
        if(!value){equal=true;break;}
        if(index==63)return S::InvalidSource; // Original compare would escape the admitted buffer.
    }
    if(equal)return S::Ready;
    // Original sut::wcsncpy(cap64) can write a 64th word and then overwrites that
    // same word with NUL. Preserve read/write order for a live overlapping input.
    std::size_t copied=0;
    while(copied<64) {
        char16_t value{};if(const auto s=Read(next,source,copied,value);s!=S::Ready)return s;
        if(!value)break;
        Write(next.talker,copied,value);++copied;
    }
    Write(next.talker,copied==64?63:copied,0);
    TalkLogSource retained;retained.kind=TalkLogSource::Kind::Talker;retained.owner=next.identity;
    const auto status=AppendNameLocal(next,retained);
    if(status==S::Ready)state_=std::move(next);
    return status;
}
std::optional<std::uint32_t> NativeTalkLog::GetCurrent() const noexcept {return state_?std::optional(state_->current):std::nullopt;}
std::optional<TalkLogSnapshot> NativeTalkLog::Observe() const {return state_;}
std::optional<TalkLogLineView> NativeTalkLog::GetLine(std::uint32_t index) const noexcept {
    if(!state_)return {};
    return TalkLogLineView{state_->identity,static_cast<std::uint8_t>(index&63u)};
}
std::optional<TalkLogLine> NativeTalkLog::ObserveLine(const TalkLogLineView& view) const {
    if(!state_ || !view.owner || view.owner!=state_->identity || view.slot>=64)return {};
    return state_->lines[view.slot];
}
TalkLogSource NativeTalkLog::TalkerSource(std::size_t offset) const {
    TalkLogSource out;out.kind=TalkLogSource::Kind::Talker;out.offset=offset;
    if(state_)out.owner=state_->identity;
    return out;
}
TalkLogSource NativeTalkLog::LineSource(const TalkLogLineView& view,std::size_t offset) const {
    TalkLogSource out;out.kind=TalkLogSource::Kind::Line;out.owner=view.owner;out.slot=view.slot;out.offset=offset;return out;
}
TalkLogStatus NativeTalkLog::RestoreCarried(const TalkLogSnapshot& value) {
    if(!state_)return S::Absent;
    if(!value.identity || value.identity!=state_->identity)return S::StaleSource;
    state_=value;return S::Ready;
}
}

namespace fates::runtime::native {
TalkLogStatus NativeTalkLog::DisableRecording(){if(!state_)return TalkLogStatus::Absent;state_->recording=0;return TalkLogStatus::Ready;}
TalkLogStatus NativeTalkLog::DisableStreamFlag(){if(!state_)return TalkLogStatus::Absent;state_->stream_flag=0;return TalkLogStatus::Ready;}
}
