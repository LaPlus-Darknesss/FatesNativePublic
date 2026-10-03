#include "fates/runtime/native_talk_text.hpp"
#include "fates/runtime/native_runtime.hpp"
#include <algorithm>

namespace fates::runtime::native {
namespace {
using S=TalkTextStatus;using M=MessageLookupStatus;using A=ArchiveIdentifierStatus;
bool Delimiter(char16_t word) {return word==u'|'||word==u',';}
bool MessageReady(const MessageLookupResult& r) {return r.status==M::Ready||r.status==M::Missing;}
constexpr std::string_view DefaultName="MPID_\x83\x66\x83\x74\x83\x48\x83\x8b\x83\x67\x96\xbc";
}
TalkTextStatus NativeTalkPlayer::SetPlayer(std::optional<std::uint16_t> slot) {
    if(!slot) {SetDefault();return S::Ready;}
    if(*slot>=runtime_.game.units.size()||!runtime_.game.units[*slot].occupied)return S::InvalidUnit;
    override_=Override{*slot,runtime_.game.unit_slot_generations[*slot]};return S::Ready;
}
TalkPlayerResult NativeTalkPlayer::Find() {
    if(override_) {
        const auto slot=override_->slot;
        if(!runtime_.game.units[slot].occupied||runtime_.game.unit_slot_generations[slot]!=override_->generation)
            return {S::StalePlayer};
        return {S::Ready,slot};
    }
    ++pool_queries_;const auto found=player_.Find(runtime_.definitions,runtime_.game);
    return found.status==PlayerUnitLookupStatus::Ready?TalkPlayerResult{S::Ready,found.slot}:TalkPlayerResult{S::PlayerUnavailable};
}
TalkEditResult NativeTalkPlayer::Edit(std::uint16_t slot) const {
    if(slot>=runtime_.game.units.size()||!runtime_.game.units[slot].occupied)return {S::InvalidUnit};
    const auto& unit=runtime_.game.units[slot];
    if(!unit.lineage.bound)return {S::UnknownEdit};
    if(unit.lineage.person_id!=unit.person_id)return {S::StaleEdit};
    if(!unit.lineage.value.edit)return {};
    return {S::Ready,true,unit.lineage.value.edit_name,unit.lineage.value.edit_face};
}
TalkEditResult NativeTalkPlayer::CurrentEdit() {
    const auto player=Find();if(player.status!=S::Ready)return {player.status};
    return player.slot?Edit(*player.slot):TalkEditResult{};
}
TalkTokenResult NativeTalkTokens::Get(const Reader& read,std::size_t at) {
    auto word=read(at);if(!word)return {S::InvalidSource};
    if(!*word||Delimiter(*word))return {};
    auto next=words_;std::size_t length{};
    do {
        next[length++]=*word;++at;
        // Original reads the following word even after its64th copy, before
        // testing the length bound. An embedded NUL is not a delimiter here.
        word=read(at);if(!word)return {S::InvalidSource};
    } while(!Delimiter(*word)&&length<64);
    next[length]=0;words_=next;
    const auto end=std::find(words_.begin(),words_.end(),char16_t{});
    return {S::Ready,std::u16string(words_.begin(),end),true};
}
NativeTalkTokens::SkipResult NativeTalkTokens::Skip(const Reader& read,std::size_t at) {
    const auto token=Get(read,at);return {token.status,token.text.size()+1};
}
TalkTokenResult NativeTalkTokens::Get(std::optional<std::span<const char16_t>> words) {
    if(!words)return {};
    const std::vector<char16_t> source(words->begin(),words->end());
    return Get([&](std::size_t i)->std::optional<char16_t> {
        return i<source.size()?std::optional<char16_t>{source[i]}:std::nullopt;
    },0);
}
NativeTalkTokens::SkipResult NativeTalkTokens::Skip(std::span<const char16_t> words) {
    const std::vector<char16_t> source(words.begin(),words.end());
    return Skip([&](std::size_t i)->std::optional<char16_t> {
        return i<source.size()?std::optional<char16_t>{source[i]}:std::nullopt;
    },0);
}
TalkMessageSelection NativeTalkText::SelectMessage(std::string_view identifier) {
    if(retired_)return {S::Retired};
    if(identifier.size()>65535||identifier.find('\0')!=std::string_view::npos)return {S::InvalidIdentifier};
    const auto exists=[&](std::string_view id)->std::optional<bool> {
        const auto found=identifiers_.Find(id);
        if(found.status==A::Ready)return true;
        if(found.status==A::Missing)return false;
        return std::nullopt;
    };
    // Original6DCEC4 initialized default tone. This is unrelated to the Edit
    // voice index; GetTalkTone always selects column zero of its3-column table.
    std::string tone="_PCM1";
    const auto player=player_.Find();if(player.status!=S::Ready)return {player.status};
    if(player.slot) {
        const auto edit=player_.Edit(*player.slot);if(edit.status!=S::Ready)return {edit.status};
        if(!edit.present)return {S::MissingEdit};
        if(!edit.face)return {S::UnknownAppearance};
        if(edit.face->gender>1)return {S::UnsupportedTone};
        tone=edit.face->gender==0?"_PCM1":"_PCF1";
        auto probe=std::string(identifier.substr(0,127))+tone;probe.resize(std::min<std::size_t>(probe.size(),127));
        const auto found=exists(probe);if(!found)return {S::ArchiveUnavailable};
        if(!*found)tone=edit.face->gender==0?"_PCM1":"_PCF1"; // distinct original fallback call
    }
    auto candidate=std::string(identifier.substr(0,127))+tone;candidate.resize(std::min<std::size_t>(candidate.size(),127));
    const auto found=exists(candidate);if(!found)return {S::ArchiveUnavailable};
    if(!*found) {
        const auto base=exists(identifier);if(!base)return {S::ArchiveUnavailable};
        if(!*base)return {};
        candidate=identifier;
    }
    auto source=messages_.GetNoReplace(candidate);
    return {MessageReady(source)?S::Ready:S::MessageUnavailable,std::move(source),std::move(candidate)};
}
TalkTextStatus NativeTalkText::PrepareMessage(std::string_view identifier) {
    const auto selected=SelectMessage(identifier);
    return selected.status==S::Ready?ExpandMessage(selected.source):selected.status;
}
TalkTextStatus NativeTalkText::ExpandIdentifier(std::optional<std::string_view> identifier) {
    if(retired_)return S::Retired;
    return ExpandMessage(messages_.GetNoReplace(identifier));
}
TalkTextStatus NativeTalkText::ExpandMessage(const MessageLookupResult& source) {
    if(retired_)return S::Retired;
    if(!MessageReady(source))return S::MessageUnavailable;
    return Expand([&](TalkTextBuffer&)->Reader {
        return [&](std::size_t i)->std::optional<char16_t> {
            const auto word=messages_.ReadSourceWord(source,i);
            return word.status==M::Ready?std::optional<char16_t>{word.value}:std::nullopt;
        };
    },false);
}
TalkTextStatus NativeTalkText::ExpandWords(std::optional<std::span<const char16_t>> input) {
    if(retired_)return S::Retired;
    if(input&&input->size()>1024u*1024u)return S::InvalidSource;
    const std::vector<char16_t> source=input?std::vector<char16_t>(input->begin(),input->end()):std::vector<char16_t>{};
    return Expand([&](TalkTextBuffer&)->Reader {
        return [&](std::size_t i)->std::optional<char16_t> {return i<source.size()?std::optional<char16_t>{source[i]}:std::nullopt;};
    },!input);
}
TalkTextStatus NativeTalkText::ExpandText(std::optional<std::u16string_view> input) {
    if(retired_)return S::Retired;
    if(!input)return ExpandWords(std::nullopt);
    if(input->size()>1024u*1024u)return S::InvalidSource;
    std::vector<char16_t> words(input->begin(),input->end());words.push_back(0);
    return ExpandWords(std::span<const char16_t>(words));
}
TalkTextStatus NativeTalkText::ExpandCurrent(std::size_t offset) {
    if(retired_)return S::Retired;
    if(offset>=buffer_.words.size())return S::InvalidSource;
    return Expand([offset](TalkTextBuffer& next)->Reader {
        return [&next,offset](std::size_t i)->std::optional<char16_t> {
            if(i>=next.words.size()-offset||!next.known[offset+i])return std::nullopt;
            return next.words[offset+i];
        };
    },false);
}
TalkTextStatus NativeTalkText::ExpandReader(const NativeTalkTokens::Reader& input) {
    if(retired_)return S::Retired;
    if(!input)return TalkTextStatus::InvalidSource;
    return Expand([&](TalkTextBuffer&)->Reader {return input;},false);
}
TalkTextStatus NativeTalkText::RestoreBuffer(std::span<const char16_t> words,std::optional<std::size_t> cursor) {
    if(retired_)return S::Retired;
    if(words.size()!=buffer_.words.size()||(cursor&&*cursor>=words.size()))return S::InvalidBuffer;
    std::copy(words.begin(),words.end(),buffer_.words.begin());buffer_.known.set();buffer_.cursor=cursor;return S::Ready;
}
TalkTextObservation NativeTalkText::Observe() const {
    if(retired_)return {S::Retired};
    if(!buffer_.cursor)return {S::UnboundCursor};
    std::u16string text;
    for(auto i=*buffer_.cursor;i<buffer_.words.size();++i) {
        if(!buffer_.known[i])return {S::UnknownWord};
        if(!buffer_.words[i])return {S::Ready,std::move(text)};
        text+=buffer_.words[i];
    }
    return {S::BufferOverflow};
}
TalkTextStatus NativeTalkText::Expand(const std::function<Reader(TalkTextBuffer&)>& make_read,bool null_source) {
    if(retired_)return S::Retired;
    auto next=buffer_;auto tokens=tokens_;const auto read=make_read(next);
    const auto write=[&](std::size_t at,char16_t value) {
        if(at>=next.words.size())return false;
        next.words[at]=value;next.known.set(at);return true;
    };
    const auto finish=[&](std::size_t output) {
        if(!write(output,0))return S::BufferOverflow;
        next.cursor=0;buffer_=std::move(next);tokens_=std::move(tokens);return S::Ready;
    };
    if(null_source)return finish(0);
    // This Get occurs even for empty text and before reading any source word.
    // A source in the same Mess buffer therefore observes this mutation.
    const auto default_name=names_.GetMessage(DefaultName);
    if(default_name.status!=UnitNameStatus::Ready)return S::MessageUnavailable;
    const auto edit=player_.CurrentEdit();if(edit.status!=S::Ready)return edit.status;
    const Reader name=[&](std::size_t i)->std::optional<char16_t> {
        if(edit.present) {
            if(!edit.name||i>=edit.name->size())return std::nullopt;
            return (*edit.name)[i];
        }
        const auto word=messages_.ReadSourceWord(default_name.message,i);
        return word.status==M::Ready?std::optional<char16_t>{word.value}:std::nullopt;
    };
    const auto length=[](const Reader& source)->std::optional<std::size_t> {
        for(std::size_t i=0;i<1024u*1024u;++i) {const auto word=source(i);if(!word)return std::nullopt;if(!*word)return i;}
        return std::nullopt;
    };
    const auto copy=[&](std::size_t at,const Reader& source,std::size_t count) {
        // Original wcsncpy writes through the first NUL, preserving all tail.
        for(std::size_t i=0;i<count;++i) {
            const auto word=source(i);if(!word)return S::InvalidSource;
            if(!write(at+i,*word))return S::BufferOverflow;
            if(!*word)return S::Ready;
        }
        return write(at+count-1,0)?S::Ready:S::BufferOverflow;
    };
    std::size_t at{},output{};
    for(;;) {
        const auto word=read(at);if(!word)return S::InvalidSource;
        if(!*word)return finish(output);
        if(*word==u'$') {
            const auto code=read(at+1);if(!code)return S::InvalidSource;
            if(*code==u'a') {at+=3;output+=3;continue;}
            if(*code==u'N') {
                const auto n=length(name);if(!n)return S::UnknownName;
                const auto kind=read(at+2);if(!kind)return S::InvalidSource;
                const auto end=output+*n;if(end>=8192)return finish(output);
                const Reader source=(*kind==u'p'||*kind==u'u')?name:Reader{[](std::size_t i)->std::optional<char16_t> {
                    return i==0?std::optional<char16_t>{char16_t{}}:std::nullopt;
                }};
                const auto status=copy(output,source,*n+1);if(status!=S::Ready)return status;
                at+=3;output=end;continue;
            }
            if(*code==u'G'||*code==u'e') {
                at+=2;
                if(*code==u'G') {
                    if(edit.present&&!edit.face)return S::UnknownAppearance;
                    if(!edit.present||edit.face->gender!=0) {
                        const auto skip=tokens.Skip(read,at);if(skip.status!=S::Ready)return skip.status;at+=skip.words;
                    }
                }
                auto token=tokens.Get(read,at);if(token.status!=S::Ready)return token.status;
                if(*code==u'e'||(edit.present&&edit.face->gender==0)) {
                    const auto skip=tokens.Skip(read,at);if(skip.status!=S::Ready)return skip.status;at+=skip.words;
                }
                if(*code==u'e') {
                    const auto dollar=read(at);if(!dollar)return S::InvalidSource;
                    if(*dollar==u'$') {
                        const auto n=read(at+1);if(!n)return S::InvalidSource;
                        if(*n==u'N') {
                            const auto first=name(0);if(!first)return S::UnknownName;
                            if(std::u16string_view(u"AEHIOUYaehiouy").find(*first)!=std::u16string_view::npos)token.text=u"'";
                        }
                    }
                }
                const auto end=output+token.text.size();if(end>=8192)return finish(output);
                const Reader source=[&](std::size_t i)->std::optional<char16_t> {
                    if(i>token.text.size())return std::nullopt;
                    return i==token.text.size()?char16_t{}:token.text[i];
                };
                const auto status=copy(output,source,token.text.size()+1);if(status!=S::Ready)return status;
                if(*code==u'G') {const auto skip=tokens.Skip(read,at);if(skip.status!=S::Ready)return skip.status;at+=skip.words;}
                output=end;continue;
            }
        }
        if(output+1>=8192)return finish(output);
        if(!write(output++,*word))return S::BufferOverflow;
        ++at;
    }
}
}
