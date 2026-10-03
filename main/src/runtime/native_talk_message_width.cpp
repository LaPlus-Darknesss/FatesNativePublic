#include "fates/runtime/native_talk_message_width.hpp"
#include <algorithm>
#include <bit>
#include <limits>

namespace fates::runtime::native {
using namespace presentation::native;
using S=TalkMessageWidthStatus;
namespace {
constexpr std::uint32_t Calculate=0x1efdbc;
constexpr std::size_t ReadBound=1024*1024;
}
NativeShiftJis::WordReader TalkWindowArgumentState::TokenReader(NativeTalkTokens& tokens) const {
    if(!shared)return [](std::size_t at)->std::optional<char16_t>{return at==0?std::optional{char16_t{0}}:std::nullopt;};
    return [&tokens](std::size_t at)->std::optional<char16_t>{return at<tokens.window_words_.size()?std::optional{tokens.window_words_[at]}:std::nullopt;};
}
S TalkWindowArgumentState::Step(const NativeTalkTokens::Reader& read,std::size_t begin,NativeTalkTokens& tokens,const NativeShiftJis& encoding) {
    if(value.completed)return S::InvalidRequest;
    auto block=[&](S why=S::InvalidSource){value.status=why;return why;};
    if(begin>std::numeric_limits<std::size_t>::max()-67)return block();
    if(stage==0){value.fid={};value.fid[0]='F';value.fid[1]='I';value.fid[2]='D';value.fid[3]='_';value.location=0;stage=1;}
    if(stage==1){const auto sub=read?read(begin):std::nullopt;if(!sub)return block();if(*sub!=u'm' && *sub!=u's'){value.completed=true;return value.status=S::Ready;}stage=2;source_index=begin+1;}
    if(stage==2){const auto first=read(source_index);if(!first)return block();if(!*first || *first==u'|' || *first==u','){shared=false;stage=6;}else {shared=true;stage=3;}}
    for(;;) {
        if(stage==3){const auto word=read(source_index);if(!word)return block();tokens.window_words_[copied++]=*word;++source_index;stage=4;}
        if(stage==4){const auto next=read(source_index);if(!next)return block();if(*next==u'|' || *next==u',' || copied==64){tokens.window_words_[copied]=0;stage=6;}else {stage=3;continue;}}
        if(stage==6){
            // Read strlen from the live dedicated window token, not from a copied return.
            length=0;if(shared){while(length<tokens.window_words_.size() && tokens.window_words_[length])++length;if(length==tokens.window_words_.size())return block();}
            stage=7;
        }
        if(stage==7){const auto word=read(begin+length+2);if(!word)return block();if(*word==u'g'){value.location=static_cast<std::uint8_t>(*word);stage=10;}else stage=8;}
        if(stage==8){length=0;if(shared){while(length<tokens.window_words_.size() && tokens.window_words_[length])++length;if(length==tokens.window_words_.size())return block();}stage=9;}
        if(stage==9){const auto word=read(begin+length+2);if(!word)return block();value.location=static_cast<std::uint8_t>(std::uint32_t(*word)-48u);stage=10;}
        if(stage==10){
            std::array<std::uint8_t,32> encoded{};
            const auto result=encoding.TerminatedSjis(TokenReader(tokens),encoded);
            if(result.status!=ShiftJisStatus::Ready)return block(result.status==ShiftJisStatus::TablesUnavailable?S::Unavailable:S::InvalidSource);
            // Existing bounded strcat has64bytes here and a <=31byte source.
            // The maximum4+31byte result cannot truncate this admitted buffer.
            std::copy_n(encoded.begin(),result.produced+1u,value.fid.begin()+4);
            value.completed=true;return value.status=S::Ready;
        }
    }
}
struct NativeTalkMessageWidth::State {
    struct Request {TalkWidthObservation value;ProcessHandle manager;TalkWindowHandle window;Reader reader;};
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<NativeTalkControlContext> managers;std::shared_ptr<NativeTalkWindow> windows;
    std::shared_ptr<NativeFontMetrics> fonts;std::shared_ptr<NativeTalkTokens> tokens;std::shared_ptr<const NativeShiftJis> encoding;
    std::map<std::uint32_t,std::shared_ptr<Request>> requests;std::uint32_t serial{};
    S Mutable(ProcessAccess* access) const {
        const auto owner=scheduler.lock();if(!owner || !owner->root(2))return S::Retired;
        if(access)return access->BelongsTo(*owner)?S::Ready:S::MismatchedDomain;
        return owner->busy()?S::Busy:S::Ready;
    }
    std::shared_ptr<Request> Get(TalkWidthRequest handle)const {
        if(!handle)return {};
        const auto it=requests.find(handle->serial);return it!=requests.end() && it->second->value.identity==handle?it->second:nullptr;
    }
};
struct NativeTalkMessageWidth::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;std::shared_ptr<State::Request> request;ProcessCall call;
    unsigned stage{};std::uint32_t saved{};char16_t word{},command{},sub{};
    bool keep_scanning{true};FontTextRequest glyph;TalkWindowArgumentState argument;
    ~Continuation()override {if(glyph)state->fonts->ForgetText(glyph);}
    ProcessCallbackStep Block(S status=S::Unavailable){request->value.status=status;return ProcessCallbackStep::Blocked();}
    std::optional<char16_t> Read(std::size_t at){if(request->value.reads>=ReadBound){request->value.status=S::ReadLimit;return {};}++request->value.reads;return request->reader?request->reader(at):std::nullopt;}
    bool Offset(std::size_t amount){if(!request->value.cursor || amount>std::numeric_limits<std::size_t>::max()-*request->value.cursor)return false;*request->value.cursor+=amount;return true;}
    ProcessCallbackStep Step(ProcessAccess& access)override {
        if(state->Mutable(&access)!=S::Ready)return Block(S::Retired);
        request->value.status=S::Ready;
        if(stage==0){stage=1;return ProcessCallbackStep::Call(NativeFontMetrics::GetCurrentCall(call.process));}
        if(stage==1){saved=access.call_result();stage=2;}
        if(stage==2){const auto window=state->windows->Observe(request->window);if(!window)return Block(S::InvalidWindow);stage=3;return ProcessCallbackStep::Call(NativeFontMetrics::SetCurrentCall(call.process,window->font));}
        for(;;){
            if(stage==3){if(!request->value.cursor)return Block(S::InvalidSource);const auto value=Read(*request->value.cursor);if(!value)return Block(request->value.status==S::ReadLimit?S::ReadLimit:S::InvalidSource);word=*value;
                if(!word)stage=20;else if(word==u'\n'){request->value.line_width=0;if(!Offset(1))return Block(S::InvalidSource);}else stage=word==u'$'?8u:4u;
                if(word==u'\n')continue;}
            if(stage==4){const auto value=Read(*request->value.cursor);if(!value)return Block(S::InvalidSource);word=*value;if(!Offset(1))return Block(S::InvalidSource);stage=5;}
            if(stage==5){
                // Original copies just this one source word and a terminator to
                // stack before calling Font::GetWidth. That scratch is not live
                // message storage: later input changes must not change the glyph.
                const auto copied=word;
                const auto reader=[copied](std::size_t at)->std::optional<char16_t>{if(at==0)return copied;if(at==1)return char16_t{0};return {};};
                if(state->fonts->PrepareText(reader,FontTextOperation::Width,glyph,&access)!=FontMetricStatus::Ready)return Block();
                stage=6;const auto nested=state->fonts->TextCall(call.process,glyph);if(!nested)return Block();return ProcessCallbackStep::Call(*nested);
            }
            if(stage==6){const auto value=access.call_result();if(state->fonts->Release(glyph,&access)!=FontMetricStatus::Ready)return Block();glyph.reset();request->value.line_width+=value;
                if(std::bit_cast<std::int32_t>(request->value.maximum)<std::bit_cast<std::int32_t>(request->value.line_width))request->value.maximum=request->value.line_width;
                stage=3;continue;}
            if(stage==8){
                if(*request->value.cursor>std::numeric_limits<std::size_t>::max()-2)return Block(S::InvalidSource);
                const auto value=Read(*request->value.cursor+1);if(!value)return Block(S::InvalidSource);command=*value;
                if(!NativeTalkControlScanner::Route(command,TalkCodeOperation::Skip))return Block(S::InvalidControl);
                keep_scanning=true;stage=command==u'W'?9u:12u;
            }
            if(stage==9){const auto value=Read(*request->value.cursor+2);if(!value)return Block(S::InvalidSource);sub=*value;argument=TalkWindowArgumentState{};stage=10;}
            if(stage==10){
                const auto status=argument.Step([this](std::size_t at){return Read(at);},*request->value.cursor+2,*state->tokens,*state->encoding);
                if(status!=S::Ready)return Block(status);
                stage=11;
            }
            if(stage==11){
                std::optional<TalkWindowHandle> selected;
                if(sub==u'm')selected=state->managers->FreeWindow(call.process,argument.value.location);
                else if(sub==u's')selected=state->managers->WindowForWidth(call.process,argument.value.fid);
                else if(sub==u'a'||sub==u'c'||sub==u'd')selected=state->managers->CurrentWindow(call.process);
                else {keep_scanning=true;stage=12;}
                if(stage==11){if(!selected)return Block();const bool equal=*selected==request->window;keep_scanning=(sub==u's'||sub==u'a')?equal:!equal;stage=12;}
            }
            if(stage==12){
                // Page reset depends on the FIRST dispatch byte, but Skip reads
                // the command again. An ending handler still executes Skip.
                if(command==u'p')request->value.line_width=0;
                stage=13;
            }
            if(stage==13){
                NativeTalkControlScanner scan(*state->tokens);
                const auto result=scan.Skip([this](std::size_t at){return Read(at);},*request->value.cursor);
                if(result.status!=TalkCodeStatus::Ready)return Block(result.status==TalkCodeStatus::InvalidCode?S::InvalidControl:S::InvalidSource);
                request->value.cursor=result.next;++request->value.controls;
                stage=keep_scanning?3u:20u;if(stage==3)continue;
            }
            if(stage==20){stage=21;return ProcessCallbackStep::Call(NativeFontMetrics::SetCurrentCall(call.process,saved));}
            if(stage==21){request->value.completed=true;request->value.result=request->value.maximum+16u;return ProcessCallbackStep::Return(*request->value.result);}
        }
    }
};
NativeTalkMessageWidth::NativeTalkMessageWidth(std::shared_ptr<State> state):state_(std::move(state)){}
NativeTalkMessageWidth::~NativeTalkMessageWidth()=default;
S NativeTalkMessageWidth::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<NativeTalkControlContext> managers,std::shared_ptr<NativeTalkWindow> windows,
    std::shared_ptr<NativeFontMetrics> fonts,std::shared_ptr<NativeTalkTokens> tokens,std::shared_ptr<const NativeShiftJis> encoding,
    std::shared_ptr<NativeTalkMessageWidth>& output){
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !managers || !managers->UsesScheduler(*scheduler) || !windows || !windows->UsesScheduler(*scheduler) || !managers->UsesWindows(*windows) || !fonts || !fonts->UsesScheduler(*scheduler) || !tokens || !encoding)return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->managers=std::move(managers);state->windows=std::move(windows);state->fonts=std::move(fonts);state->tokens=std::move(tokens);state->encoding=std::move(encoding);
    auto owner=std::shared_ptr<NativeTalkMessageWidth>(new NativeTalkMessageWidth(state));if(!registry->Register(std::array{Calculate},owner))return S::DuplicateBinding;
    output=std::move(owner);return S::Ready;
}
S NativeTalkMessageWidth::Prepare(ProcessHandle manager,TalkWindowHandle window,TalkWindowSource source,std::size_t start,TalkWidthRequest& out,ProcessAccess* access){
    const auto windows=state_->windows;return PrepareReader(std::move(manager),std::move(window),[windows,source=std::move(source)](std::size_t at)->std::optional<char16_t>{const auto value=windows->Read(source,at);return value.status==TalkWindowStatus::Ready?std::optional{value.value}:std::nullopt;},start,out,access);
}
S NativeTalkMessageWidth::PrepareReader(ProcessHandle manager,TalkWindowHandle window,Reader reader,std::size_t start,TalkWidthRequest& out,ProcessAccess* access){
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    const auto scheduler=state_->scheduler.lock();const auto parent=scheduler->Observe(manager);
    if(!parent || !parent->linked || (parent->flags&1u))return S::InvalidRequest;
    if(!state_->windows->Observe(window))return S::InvalidWindow;
    if(!reader)return S::InvalidSource;
    if(state_->serial==std::numeric_limits<std::uint32_t>::max())return S::IdentityExhausted;
    auto row=std::make_shared<State::Request>();row->value.identity=std::make_shared<TalkWidthIdentity>(++state_->serial);row->value.cursor=start;
    row->manager=std::move(manager);row->window=std::move(window);row->reader=std::move(reader);state_->requests.emplace(row->value.identity->serial,row);out=row->value.identity;return S::Ready;
}
std::optional<ProcessCall> NativeTalkMessageWidth::Call(TalkWidthRequest handle)const {
    const auto owner=state_->scheduler.lock();const auto row=state_->Get(handle);if(!owner || !owner->root(2) || !row || row->value.completed || !owner->Observe(row->manager))return {};
    ProcessCall call;call.process=row->manager;call.target=Calculate;call.kind=ProcessCallKind::Service;call.argument_count=1;call.arguments[0]=handle->serial;return call;
}
std::optional<TalkWidthObservation> NativeTalkMessageWidth::Observe(TalkWidthRequest handle)const {const auto owner=state_->scheduler.lock();const auto row=state_->Get(handle);return owner && owner->root(2) && row && owner->Observe(row->manager)?std::optional{row->value}:std::nullopt;}
S NativeTalkMessageWidth::Release(TalkWidthRequest handle,ProcessAccess* access){if(const auto status=state_->Mutable(access);status!=S::Ready)return status;if(!state_->Get(handle))return S::InvalidRequest;state_->requests.erase(handle->serial);return S::Ready;}
void NativeTalkMessageWidth::Forget(TalkWidthRequest handle) noexcept {if(state_->Get(handle))state_->requests.erase(handle->serial);}
bool NativeTalkMessageWidth::UsesOwners(const NativeTalkControlContext& managers,const NativeTalkWindow& windows)const noexcept{return state_->managers.get()==&managers && state_->windows.get()==&windows;}
bool NativeTalkMessageWidth::UsesTokens(const NativeTalkTokens& tokens)const noexcept{return state_->tokens.get()==&tokens;}
bool NativeTalkMessageWidth::UsesScheduler(const NativeProcessScheduler& scheduler)const noexcept{return state_->scheduler.lock().get()==&scheduler;}
std::unique_ptr<ProcessContinuation> NativeTalkMessageWidth::Begin(const ProcessCall& call){
    const auto owner=state_->scheduler.lock();if(!owner || !owner->root(2) || !call.process || call.kind!=ProcessCallKind::Service || call.has_self || call.this_adjustment || call.target!=Calculate || call.argument_count!=1)return {};
    const auto it=state_->requests.find(call.arguments[0]);if(it==state_->requests.end() || it->second->manager!=call.process || it->second->value.completed)return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->request=it->second;next->call=call;return next;
}
}
