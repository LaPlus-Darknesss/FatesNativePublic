#include "fates/runtime/native_talk_control_effects.hpp"
#include "fates/runtime/native_talk_window_controls.hpp"
#include "fates/runtime/native_talk_mode_fade.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
namespace fates::runtime::native {
using namespace presentation::native;
using S=TalkControlStatus;
namespace {
constexpr std::uint32_t Dispose=0x1efec4,Flash=0x1efda0,FaceTick=0x1ef16c,FaceDestroy=0x1ef190;
constexpr std::array Targets{Dispose,Flash,FaceTick,FaceDestroy};
ProcessCall Service(ProcessHandle process,std::uint32_t target,std::initializer_list<std::uint32_t> args={}) {
    ProcessCall call;call.process=std::move(process);call.kind=ProcessCallKind::Service;call.target=target;
    call.argument_count=static_cast<std::uint8_t>(args.size());std::copy(args.begin(),args.end(),call.arguments.begin());return call;
}
float Add(float a,float b){volatile float result=a+b;return result;}
}
NativeTalkControlContext::NativeTalkControlContext(std::shared_ptr<NativeProcessScheduler> scheduler,
    std::shared_ptr<NativeTalkWindow> windows,std::shared_ptr<NativeTalkBackground> backgrounds)
    :scheduler_(std::move(scheduler)),windows_(std::move(windows)),backgrounds_(std::move(backgrounds)){}
S NativeTalkControlContext::RestoreCarried(const TalkControlManagerView& value,ProcessAccess* access) {
    const auto scheduler=scheduler_.lock();if(!scheduler || !scheduler->root(2))return S::Retired;
    if(!windows_ || !backgrounds_ || !windows_->UsesScheduler(*scheduler) || !backgrounds_->UsesScheduler(*scheduler))return S::MismatchedDomain;
    if(access){if(!access->BelongsTo(*scheduler))return S::MismatchedDomain;}
    else if(scheduler->busy())return S::Busy;
    const auto manager=scheduler->Observe(value.manager);if(!manager || !manager->linked || (manager->flags&1u))return S::InvalidParent;
    if(value.window && *value.window && !windows_->Observe(*value.window))return S::InvalidHandle;
    if(value.system_window && *value.system_window && !windows_->Observe(*value.system_window))return S::InvalidHandle;
    if(value.background && *value.background && !backgrounds_->Observe(*value.background))return S::InvalidHandle;
    if(value.auxiliary && *value.auxiliary && !scheduler->Observe(*value.auxiliary))return S::InvalidHandle;
    for(const auto& slot:value.window_slots)if(slot && (!*slot || !windows_->Observe(*slot)))return S::InvalidHandle;
    rows_[value.manager->serial]=value;return S::Ready;
}
std::optional<TalkControlManagerView> NativeTalkControlContext::Observe(ProcessHandle manager) const {
    const auto scheduler=scheduler_.lock();if(!scheduler || !scheduler->root(2) || !scheduler->Observe(manager))return {};
    const auto row=rows_.find(manager->serial);if(row==rows_.end() || row->second.manager!=manager)return {};
    return row->second;
}
std::optional<TalkWindowHandle> NativeTalkControlContext::FreeWindow(ProcessHandle manager,std::uint32_t location) const {
    const auto view=Observe(manager);if(!view)return {};
    for(const auto& slot:view->window_slots) {
        if(!slot || !*slot)return {};
        const auto row=windows_->Observe(*slot);if(!row || !row->location)return {};
        if(*row->location==location)return *slot;
    }
    for(const auto& slot:view->window_slots) {
        const auto row=windows_->Observe(*slot);if(!row)return {};
        // The window owner currently admits genuine fresh face-null windows.
        if(!row->active)return *slot;
    }
    if(!view->free_window_mode)return {};
    if(*view->free_window_mode)return *view->window_slots[location>5?1u:0u];
    if(location>=9)return TalkWindowHandle{};
    return *view->window_slots[location%3u==2u?1u:0u];
}
std::optional<TalkWindowHandle> NativeTalkControlContext::WindowForWidth(ProcessHandle manager,std::span<const std::uint8_t> fid) const {
    const auto view=Observe(manager);if(!view)return {};
    for(const auto& slot:view->window_slots) {
        if(!slot || !*slot)return {};
        const auto row=windows_->Observe(*slot);if(!row)return {};
        bool matched=false;
        for(std::size_t index=0;;++index) {
            if(index>=fid.size() || index>32)return {}; // Beyond owned identifier/adjacent byte is unavailable.
            const auto byte=index==0?row->name_effect_enabled:index==32?std::optional{row->name_override}:
                row->face_identifier_known[index-1]?std::optional{row->face_identifier_tail[index-1]}:std::nullopt;
            if(!byte)return {};
            if(fid[index]!=*byte)break;
            if(!fid[index]){matched=true;break;}
        }
        if(matched)return *slot;
    }
    return TalkWindowHandle{};
}
S NativeTalkControlContext::BeginWindowSelection(ProcessHandle manager,ProcessAccess& access) {
    const auto scheduler=scheduler_.lock();if(!scheduler || !access.BelongsTo(*scheduler))return S::MismatchedDomain;
    if(!Observe(manager))return S::InvalidParent;
    rows_.at(manager->serial).selection_pending=std::uint8_t{1};return S::Ready;
}
S NativeTalkControlContext::SelectCurrentWindow(ProcessHandle manager,TalkWindowHandle window,ProcessAccess& access) {
    const auto scheduler=scheduler_.lock();if(!scheduler || !access.BelongsTo(*scheduler))return S::MismatchedDomain;
    if(!Observe(manager))return S::InvalidParent;
    if(!window || !windows_->Observe(window))return S::InvalidHandle;
    rows_.at(manager->serial).window=std::move(window);return S::Ready;
}
S NativeTalkControlContext::ClearAuxiliary(ProcessHandle manager,ProcessAccess& access) {
    const auto scheduler=scheduler_.lock();if(!scheduler || !access.BelongsTo(*scheduler))return S::MismatchedDomain;
    if(!Observe(manager))return S::InvalidParent;
    rows_.at(manager->serial).auxiliary=ProcessHandle{};return S::Ready;
}
S NativeTalkControlContext::WriteShadowEnabled(ProcessHandle manager,std::uint8_t value,ProcessAccess& access) {
    const auto scheduler=scheduler_.lock();if(!scheduler || !access.BelongsTo(*scheduler))return S::MismatchedDomain;
    if(!Observe(manager))return S::InvalidParent;
    rows_.at(manager->serial).shadow_enabled=value;return S::Ready;
}
S NativeTalkControlContext::WriteTalkMode(ProcessHandle manager,std::uint8_t value,ProcessAccess& access) {
    const auto scheduler=scheduler_.lock();if(!scheduler || !access.BelongsTo(*scheduler))return S::MismatchedDomain;
    if(!Observe(manager))return S::InvalidParent;
    rows_.at(manager->serial).free_window_mode=value;return S::Ready;
}
S NativeTalkControlContext::WriteFadeDuration(ProcessHandle manager,std::uint32_t value,ProcessAccess& access) {
    const auto scheduler=scheduler_.lock();if(!scheduler || !access.BelongsTo(*scheduler))return S::MismatchedDomain;
    if(!Observe(manager))return S::InvalidParent;
    rows_.at(manager->serial).screen_fade_duration=value;return S::Ready;
}
S NativeTalkControlContext::WriteRevealCounter(ProcessHandle manager,std::uint32_t value,ProcessAccess& access) {
    const auto scheduler=scheduler_.lock();if(!scheduler || !access.BelongsTo(*scheduler))return S::MismatchedDomain;
    if(!Observe(manager))return S::InvalidParent;
    rows_.at(manager->serial).reveal_counter=value;return S::Ready;
}
S NativeTalkControlContext::CommitInitializedText(ProcessHandle manager,std::shared_ptr<const NativeTalkText> expander,ProcessAccess& access) {
    const auto scheduler=scheduler_.lock();if(!scheduler || !access.BelongsTo(*scheduler))return S::MismatchedDomain;
    if(!Observe(manager))return S::InvalidParent;
    if(!expander || !expander->buffer().cursor)return S::Unavailable;
    auto& row=rows_.at(manager->serial);
    TalkWindowSource source;source.kind=TalkWindowSource::Kind::Expanded;source.expanded=std::move(expander);source.offset=*source.expanded->buffer().cursor;
    row.message_cursor=std::move(source);
    row.character_delay=std::uint32_t{0};row.skip=std::uint8_t{0};row.delete_in_skip=std::uint8_t{0};
    row.character_parity=std::uint8_t{255};row.letter_pulse=std::uint8_t{0};row.pending_voice[0]=0;row.pending_voice_known.set(0);
    row.initialize_face_color=std::uint8_t{1};row.initialize_face_motion=std::uint8_t{1};return S::Ready;
}
std::optional<std::uint8_t> NativeTalkControlContext::Skip(ProcessHandle manager) const {const auto row=Observe(manager);return row?row->skip:std::nullopt;}
std::optional<TalkWindowHandle> NativeTalkControlContext::CurrentWindow(ProcessHandle manager) const {const auto row=Observe(manager);return row?row->window:std::nullopt;}
bool NativeTalkControlContext::UsesScheduler(const NativeProcessScheduler& s) const noexcept{return scheduler_.lock().get()==&s;}
bool NativeTalkControlContext::UsesOwners(const NativeTalkWindow& w,const NativeTalkBackground& b) const noexcept{return windows_.get()==&w && backgrounds_.get()==&b;}
struct NativeTalkControlEffects::State {
    struct Request {TalkControlObservation value;TalkWindowSource source;std::size_t start{};TalkCodeOperation operation{};};
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<NativeTalkControlContext> managers;std::shared_ptr<NativeTalkWindow> windows;std::shared_ptr<NativeTalkBackground> backgrounds;
    std::shared_ptr<NativeTalkMotion> motion;std::shared_ptr<NativeTalkColorFader> colors;std::shared_ptr<NativeGameSkip> skip;
    std::shared_ptr<NativeTalkLog> log;std::shared_ptr<NativeTalkWait> waits;std::shared_ptr<NativeTalkKeyWait> keys;std::shared_ptr<NativeTalkTokens> tokens;
    std::shared_ptr<NativeTalkFontEffects> font_effects;
    std::shared_ptr<NativeTalkWindowControls> window_controls;
    std::shared_ptr<NativeTalkModeFade> mode_fade;
    std::shared_ptr<const NativeShiftJis> encoding=NativeShiftJis::AsciiSubset();
    std::map<std::uint32_t,std::shared_ptr<Request>> requests;std::map<std::uint64_t,TalkFaceWaitObservation> faces;std::uint32_t serial{};
    S Mutable(ProcessAccess* access) const {
        const auto s=scheduler.lock();if(!s || !s->root(2))return S::Retired;
        if(access)return access->BelongsTo(*s)?S::Ready:S::MismatchedDomain;
        return s->busy()?S::Busy:S::Ready;
    }
    std::shared_ptr<Request> Find(TalkControlRequest handle) const {
        if(!handle)return {};
        const auto it=requests.find(handle->serial);return it!=requests.end() && it->second->value.identity==handle?it->second:nullptr;
    }
};
struct NativeTalkControlEffects::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;ProcessCall call;std::shared_ptr<State::Request> request;
    unsigned stage{},index{};char16_t command{},sub{};std::size_t argument{},cursor{};
    TalkWindowHandle window;TalkBackgroundHandle background;TalkBackgroundName name;
    TalkArgumentColor color;TalkVectorBits position{},delta{};std::uint32_t number{};ProcessHandle auxiliary;
    TalkWindowControlRequest window_request;
    TalkModeFadeRequest mode_request;
    ~Continuation()override {if(mode_request && state->mode_fade)state->mode_fade->Forget(mode_request);if(window_request && state->window_controls)state->window_controls->Forget(window_request);}
    bool flash{};std::u16string pending_token;
    ProcessCallbackStep Block(S why=S::Unavailable){if(request)request->value.status=why;return ProcessCallbackStep::Blocked();}
    ProcessCallbackStep Done(std::uint32_t value=0) {
        request->value.completed=true;request->value.status=S::Ready;
        request->value.result=flash?std::nullopt:std::optional{value};return ProcessCallbackStep::Return(flash?0u:value);
    }
    NativeTalkTokens::Reader Reader() const {
        return [this](std::size_t offset)->std::optional<char16_t>{const auto value=state->windows->Read(request->source,offset);return value.status==TalkWindowStatus::Ready?std::optional{value.value}:std::nullopt;};
    }
    std::optional<TalkWindowHandle> CurrentWindow() const {
        const auto current=state->managers->CurrentWindow(call.process);
        return current && *current && state->windows->Observe(*current)?current:std::nullopt;
    }
    std::optional<TalkBackgroundHandle> CurrentBackground() const {
        const auto row=state->managers->Observe(call.process);
        return row && row->background && *row->background && state->backgrounds->Observe(*row->background)?row->background:std::nullopt;
    }
    ProcessCallbackStep Face(ProcessAccess& access) {
        const auto row=state->faces.find(call.process->serial);
        if(row==state->faces.end() || row->second.process!=call.process)return Block();
        if(call.target==FaceDestroy){state->faces.erase(row);return ProcessCallbackStep::Return();}
        if(stage==0){stage=1;return ProcessCallbackStep::Call(Service(call.process,0x196588));}
        if(stage==1){stage=2;if(access.call_result()){++row->second.deletion_requests;return ProcessCallbackStep::Delete(call.process);}}
        return ProcessCallbackStep::Return();
    }
    ProcessCallbackStep Page(ProcessAccess& access) {
        if(stage==1){if(state->log->NextLine()!=TalkLogStatus::Ready)return Block();stage=2;}
        if(flash)return Done();
        if(stage==2) {
            const auto skip=state->managers->Skip(call.process);if(!skip)return Block();
            const auto current=CurrentWindow();if(!current)return Block();window=*current;
            if(*skip){if(state->windows->ResetStrings(window,&access)!=TalkWindowStatus::Ready)return Block();return Done(4);}
            const auto child=state->motion->NextPageCall(call.process,window);if(!child)return Block();stage=3;return ProcessCallbackStep::Call(*child);
        }
        return Done(2);
    }
    ProcessCallbackStep Cont(ProcessAccess& access) {
        if(stage==1) {
            const auto manager=state->managers->Observe(call.process);if(!manager || !manager->auxiliary)return Block();
            auxiliary=*manager->auxiliary;
            if(auxiliary){if(!access.Observe(auxiliary))return Block();stage=2;return ProcessCallbackStep::Delete(auxiliary);}
            stage=3;
        }
        if(stage==2){if(state->managers->ClearAuxiliary(call.process,access)!=S::Ready)return Block();stage=3;}
        if(stage==3){if(access.Jump(call.process,4)!=ProcessStatus::Ready)return Block();stage=4;}
        return Done(2);
    }
    ProcessCallbackStep Wait(ProcessAccess& access) {
        if(stage==1){const auto value=TalkDecimalize(Reader(),argument);if(value.status!=TalkArgumentStatus::Ready)return Block(S::InvalidSource);number=value.value;stage=2;}
        if(stage==2) {
            const auto manager=state->managers->Observe(call.process);if(!manager || !manager->alternate_wait_clock)return Block();
            ProcessHandle child;if(state->waits->Bind(access,call.process,std::bit_cast<std::int32_t>(number),*manager->alternate_wait_clock?1u:0u,child)!=TalkWaitStatus::Ready)return Block();
            stage=3;
        }
        if(stage==3) {
            auto type=ProcessType::Base();type.methods[0].target=FaceDestroy;type.methods[1].target=FaceTick;
            ProcessHandle child;if(access.Create(call.process,ProcessProgram::Default(),"ProcWaitFaceLoad",true,type,child)!=ProcessStatus::Ready)return Block();
            state->faces.emplace(child->serial,TalkFaceWaitObservation{child,call.process,0});stage=4;
        }
        return Done(2);
    }
    ProcessCallbackStep Log(ProcessAccess& access) {
        if(stage==1){const auto value=Reader()(argument);if(!value)return Block(S::InvalidSource);sub=*value;stage=2;}
        if(sub==u'r'){if(state->log->DisableRecording()!=TalkLogStatus::Ready)return Block();}
        else if(sub==u'v'){if(state->log->DisableStreamFlag()!=TalkLogStatus::Ready)return Block();}
        else if(sub==u'l'){if(state->log->NextLine()!=TalkLogStatus::Ready)return Block();}
        else if(sub==u's'){if(state->skip->DisableCurrent(access)!=GameSkipStatus::Ready)return Block();}
        else return Done(5);
        return Done(4);
    }
    ProcessCallbackStep FontColor(ProcessAccess&) {
        if(!state->font_effects)return Block(S::Unsupported);
        if(stage==1){const auto parsed=TalkGetColor8(Reader(),argument,{});if(parsed.status!=TalkArgumentStatus::Ready)return Block(S::InvalidSource);color=parsed.color;if(!color.known.all())return Block(S::Unavailable);stage=2;}
        if(stage==2){const auto current=CurrentWindow();if(!current)return Block();window=*current;stage=3;return ProcessCallbackStep::Call(NativeTalkFontEffects::ChangeColorCall(call.process,window,color.bytes));}
        return Done(4);
    }
    ProcessCallbackStep Background(ProcessAccess& access) {
        if(stage==1) {
            const auto value=Reader()(argument);if(!value || argument==std::numeric_limits<std::size_t>::max())return Block(S::InvalidSource);
            sub=*value;cursor=argument+1;stage=2;
        }
        if(stage==2) {
            // Unknown subcommands compute no owned read and return4 in retail.
            if(sub!=u'm' && sub!=u'0' && sub!=u'1' && sub!=u'f' && sub!=u'c' && sub!=u's')return Done(4);
            if(sub==u'm') {
                const auto token=state->tokens->Get(Reader(),cursor);
                if(token.status!=TalkTextStatus::Ready)return Block(S::InvalidSource);
                pending_token=token.text;stage=20;
            }else stage=21;
        }
        if(stage==20) {
            std::array<std::uint8_t,32> encoded{};
            const auto converted=state->encoding->TerminatedSjis([this](std::size_t offset)->std::optional<char16_t>{
                if(offset<pending_token.size())return pending_token[offset];
                return offset==pending_token.size()?std::optional{char16_t{0}}:std::nullopt;
            },encoded);
            if(converted.status==ShiftJisStatus::TablesUnavailable)return Block(S::Unsupported);
            if(converted.status!=ShiftJisStatus::Ready)return Block(S::InvalidSource);
            // sut::UTF16ToSJIS deliberately returns the terminated prefix even
            // when the lower STD converter stopped on an unconvertible word.
            const std::string path(reinterpret_cast<const char*>(encoded.data()),converted.produced);
            if(state->backgrounds->RegisterName(path,name,&access)!=TalkBackgroundStatus::Ready)return Block();
            stage=21;
        }
        if(stage==21) {
            const auto current=CurrentBackground();if(!current)return Block();background=*current;stage=3;
        }
        if(sub==u'0'||sub==u'1') {
            if(state->backgrounds->Select(background,sub==u'0'?0u:1u,&access)!=TalkBackgroundStatus::Ready)return Block();
            return Done(4);
        }
        if(sub==u'f'){if(state->backgrounds->FloatSelected(background,&access)!=TalkBackgroundStatus::Ready)return Block();return Done(4);}
        if(sub==u'm') {
            if(stage==3){const auto child=state->backgrounds->LoadCall(call.process,background,name);if(!child)return Block();stage=4;return ProcessCallbackStep::Call(*child);}
            return Done(4);
        }
        if(sub==u'c') {
            if(stage==3){const auto row=state->backgrounds->Observe(background);if(!row)return Block();color={row->colors[row->selected],std::bitset<4>{15}};stage=4;}
            if(stage==4){const auto value=TalkGetColor8(Reader(),cursor,color);if(value.status!=TalkArgumentStatus::Ready)return Block(S::InvalidSource);color=value.color;cursor=value.next;stage=5;}
            if(stage==5){const auto value=TalkDecimalize(Reader(),cursor);if(value.status!=TalkArgumentStatus::Ready)return Block(S::InvalidSource);number=value.value;stage=6;}
            const auto row=state->backgrounds->Observe(background);if(!row || !color.known.all())return Block();
            ProcessHandle child;
            if(state->colors->Bind(call.process,state->backgrounds,row->color_handles[row->selected],color.bytes,std::bit_cast<std::int32_t>(number),2,15,true,child,&access)!=TalkColorFaderStatus::Ready)return Block();
            return Done(4);
        }
        if(sub==u's') {
            if(stage==3){const auto value=state->backgrounds->Position(background);if(!value)return Block();position=*value;stage=4;}
            if(stage==4) {
                for(;index<3;++index){const auto value=TalkDecimalize(Reader(),cursor);if(value.status!=TalkArgumentStatus::Ready || value.next==std::numeric_limits<std::size_t>::max())return Block(S::InvalidSource);
                    delta[index]=std::bit_cast<std::uint32_t>(static_cast<float>(std::bit_cast<std::int32_t>(value.value)));cursor=value.next+1;}
                stage=5;
            }
            if(stage==5){const auto value=TalkDecimalize(Reader(),cursor);if(value.status!=TalkArgumentStatus::Ready)return Block(S::InvalidSource);number=value.value;
                TalkVectorBits result{};for(std::size_t i=0;i<3;++i){const auto sum=Add(std::bit_cast<float>(position[i]),std::bit_cast<float>(delta[i]));if(!std::isfinite(sum))return Block(S::Unsupported);result[i]=std::bit_cast<std::uint32_t>(sum);}position=result;stage=6;}
            const auto row=state->backgrounds->Observe(background);if(!row)return Block();ProcessHandle child;
            if(state->motion->BindCarrier(call.process,state->backgrounds,row->movable_handle,position,std::bit_cast<std::int32_t>(number),2,true,child,&access)!=TalkMotionStatus::Ready)return Block();
            return Done(4);
        }
        return Done(4); // Original unrecognized background subtype has no effect.
    }
    ProcessCallbackStep Step(ProcessAccess& access) override {
        if(state->Mutable(&access)!=S::Ready)return Block();
        if(call.target==FaceTick || call.target==FaceDestroy)return Face(access);
        if(!request || request->value.completed)return Block(S::InvalidHandle);
        request->value.status=S::Ready;
        if(stage==0) {
            if(request->start>std::numeric_limits<std::size_t>::max()-2)return Block(S::InvalidSource);
            const auto value=Reader()(request->start+1);if(!value)return Block(S::InvalidSource);
            const auto route=NativeTalkControlScanner::Route(*value,request->operation);if(!route)return Block(S::Unsupported);
            request->value.route=route;command=*value;argument=request->start+2;flash=request->operation==TalkCodeOperation::Flash;stage=1;
        }
        const auto& route=*request->value.route;
        if(flash) {
            // Flash is void: neutral ProcessCall completion is NOT a fabricated
            // original pointer return. Only declared scalar Dispose results escape.
            if(route.original_target==0x21934c || route.original_target==0x4e0964)return Done();
            if(command!=u'C' && command!=u'L' && command!=u'p' && command!=u'W' && command!=u't' && command!=u'F')return Block(S::Unsupported);
        }
        if(!route.known_handler)return Done(5);
        if(command==u'e'||command==u'm')return Done(4); // Genuine original no-op implementations.
        if(command==u'C')return Cont(access);
        if(command==u'p')return Page(access);
        if(command==u'w')return Wait(access);
        if(command==u'L')return Log(access);
        if(command==u'B')return Background(access);
        if(command==u'c')return FontColor(access);
        if(command==u't' || command==u'F'){
            if(!state->mode_fade)return Block(S::Unsupported);
            if(stage==1){
                if(!mode_request && state->mode_fade->Prepare(call.process,request->source,request->start,request->operation,command,mode_request,&access)!=S::Ready)return Block();
                const auto child=state->mode_fade->Call(mode_request);if(!child)return Block();
                stage=2;return ProcessCallbackStep::Call(*child);
            }
            const auto value=access.call_result();
            if(state->mode_fade->Release(mode_request,&access)!=S::Ready)return Block();
            mode_request.reset();return Done(value);
        }
        if(command==u'W'){
            if(!state->window_controls)return Block(S::Unsupported);
            if(stage==1){
                if(!window_request && state->window_controls->Prepare(call.process,request->source,request->start,request->operation,window_request,&access)!=S::Ready)return Block();
                const auto child=state->window_controls->Call(window_request);if(!child)return Block();
                stage=2;return ProcessCallbackStep::Call(*child);
            }
            const auto value=access.call_result();
            if(state->window_controls->Release(window_request,&access)!=S::Ready)return Block();
            window_request.reset();return Done(value);
        }
        if(command==u'i'){const auto value=TalkGetColor8(Reader(),argument,{});if(value.status!=TalkArgumentStatus::Ready)return Block(S::InvalidSource);return Done(2);}
        if(command==u'k') {
            if(stage==1){const auto child=state->keys->DisposeCall(call.process);if(!child)return Block();stage=2;return ProcessCallbackStep::Call(*child);}
            return Done(access.call_result());
        }
        return Block(S::Unsupported);
    }
};
NativeTalkControlEffects::NativeTalkControlEffects(std::shared_ptr<State> state):state_(std::move(state)){}
NativeTalkControlEffects::~NativeTalkControlEffects()=default;
S NativeTalkControlEffects::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<ObjectHandleRegistry> objects,std::shared_ptr<NativeTalkControlContext> managers,
    std::shared_ptr<NativeTalkWindow> windows,std::shared_ptr<NativeTalkBackground> backgrounds,
    std::shared_ptr<NativeTalkMotion> motion,std::shared_ptr<NativeTalkColorFader> colors,std::shared_ptr<NativeGameSkip> skip,
    std::shared_ptr<NativeTalkLog> log,std::shared_ptr<NativeTalkWait> waits,std::shared_ptr<NativeTalkKeyWait> keys,
    std::shared_ptr<NativeTalkTokens> tokens,std::shared_ptr<NativeTalkControlEffects>& output) {
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !objects || !managers || !managers->UsesScheduler(*scheduler)
        || !windows || !windows->UsesScheduler(*scheduler) || !windows->UsesObjectRegistry(*objects)
        || !backgrounds || !backgrounds->UsesScheduler(*scheduler) || !backgrounds->UsesObjectRegistry(*objects)
        || !managers->UsesOwners(*windows,*backgrounds) || !motion || !motion->UsesWindow(*windows) || !motion->UsesScheduler(*scheduler)
        || !colors || !colors->UsesScheduler(*scheduler) || !colors->UsesObjectRegistry(*objects) || !skip || !skip->UsesScheduler(*scheduler)
        || !log || !waits || !waits->UsesScheduler(*scheduler) || !waits->UsesGameSkip(*skip)
        || !keys || !keys->UsesScheduler(*scheduler) || !keys->UsesOwners(*managers,*log,*windows) || !tokens)return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->managers=std::move(managers);state->windows=std::move(windows);
    state->backgrounds=std::move(backgrounds);state->motion=std::move(motion);state->colors=std::move(colors);state->skip=std::move(skip);
    state->log=std::move(log);state->waits=std::move(waits);state->keys=std::move(keys);state->tokens=std::move(tokens);
    auto next=std::shared_ptr<NativeTalkControlEffects>(new NativeTalkControlEffects(state));
    if(!registry->Register(Targets,next))return S::DuplicateBinding;
    output=std::move(next);return S::Ready;
}
S NativeTalkControlEffects::PublishModeFade(std::shared_ptr<NativeTalkModeFade> controls,ProcessAccess* access){
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    if(!controls || !controls->UsesOwners(*state_->managers,*state_->windows,*state_->skip))return S::MismatchedDomain;
    state_->mode_fade=std::move(controls);return S::Ready;
}
S NativeTalkControlEffects::PublishWindowControls(std::shared_ptr<NativeTalkWindowControls> controls,ProcessAccess* access){
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    if(!controls || !controls->UsesOwners(*state_->managers,*state_->windows,*state_->tokens,*state_->log,*state_->skip))return S::MismatchedDomain;
    state_->window_controls=std::move(controls);return S::Ready;
}
S NativeTalkControlEffects::PublishFontEffects(std::shared_ptr<NativeTalkFontEffects> effects,ProcessAccess* access){
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    const auto scheduler=state_->scheduler.lock();if(!effects || !effects->UsesScheduler(*scheduler) || !effects->UsesWindow(*state_->windows))return S::MismatchedDomain;
    state_->font_effects=std::move(effects);return S::Ready;
}
S NativeTalkControlEffects::PublishEncoding(std::shared_ptr<const NativeShiftJis> encoding,ProcessAccess* access) {
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    if(!encoding)return S::Unavailable;
    state_->encoding=std::move(encoding);return S::Ready;
}
S NativeTalkControlEffects::Prepare(ProcessHandle manager,TalkWindowSource source,std::size_t start,TalkCodeOperation operation,TalkControlRequest& output,ProcessAccess* access) {
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    const auto s=state_->scheduler.lock();const auto view=s->Observe(manager);if(!view || !view->linked || (view->flags&1u))return S::InvalidParent;
    if(operation!=TalkCodeOperation::Dispose && operation!=TalkCodeOperation::Flash)return S::Unsupported;
    if(state_->serial==std::numeric_limits<std::uint32_t>::max())return S::IdentityExhausted;
    auto row=std::make_shared<State::Request>();row->value.identity=std::make_shared<TalkControlRequestIdentity>(++state_->serial);row->value.manager=manager;
    row->source=std::move(source);row->start=start;row->operation=operation;state_->requests.emplace(row->value.identity->serial,row);
    output=row->value.identity;return S::Ready;
}
std::optional<ProcessCall> NativeTalkControlEffects::Call(TalkControlRequest handle) const {
    const auto row=state_->Find(handle);const auto s=state_->scheduler.lock();
    if(!row || !s || !s->root(2) || !s->Observe(row->value.manager) || row->value.completed)return {};
    return Service(row->value.manager,row->operation==TalkCodeOperation::Dispose?Dispose:Flash,{handle->serial});
}
std::optional<TalkControlObservation> NativeTalkControlEffects::Observe(TalkControlRequest handle) const {
    const auto s=state_->scheduler.lock();const auto row=state_->Find(handle);
    if(!s || !s->root(2) || !row || !s->Observe(row->value.manager))return {};
    return row->value;
}
S NativeTalkControlEffects::Release(TalkControlRequest handle,ProcessAccess* access) {
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    if(!state_->Find(handle))return S::InvalidHandle;
    state_->requests.erase(handle->serial);return S::Ready;
}
void NativeTalkControlEffects::Forget(TalkControlRequest handle) noexcept {if(state_->Find(handle))state_->requests.erase(handle->serial);}
bool NativeTalkControlEffects::UsesOwners(const NativeTalkControlContext& context,const NativeTalkWindow& windows,
    const NativeTalkLog& log,const NativeGameSkip& skip) const noexcept {
    return state_->managers.get()==&context && state_->windows.get()==&windows && state_->log.get()==&log && state_->skip.get()==&skip;
}
std::vector<TalkFaceWaitObservation> NativeTalkControlEffects::FaceWaits() const {
    std::vector<TalkFaceWaitObservation> out;const auto s=state_->scheduler.lock();if(!s || !s->root(2))return out;
    for(const auto& [id,row]:state_->faces){(void)id;if(s->Observe(row.process))out.push_back(row);}return out;
}
bool NativeTalkControlEffects::UsesTokens(const NativeTalkTokens& tokens) const noexcept{return state_->tokens.get()==&tokens;}
std::unique_ptr<ProcessContinuation> NativeTalkControlEffects::Begin(const ProcessCall& call) {
    if(!call.process || call.this_adjustment)return {};
    const auto owner=state_->scheduler.lock();if(!owner || !owner->root(2))return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->call=call;
    if(call.target==Dispose || call.target==Flash) {
        if(call.kind!=ProcessCallKind::Service || call.has_self || call.argument_count!=1)return {};
        const auto live=owner->Observe(call.process);if(!live || !live->linked || (live->flags&1u))return {};
        const auto row=state_->requests.find(call.arguments[0]);if(row==state_->requests.end() || row->second->value.manager!=call.process || row->second->value.completed)return {};
        if(call.target!=(row->second->operation==TalkCodeOperation::Dispose?Dispose:Flash))return {};
        next->request=row->second;
    }else {
        const auto row=state_->faces.find(call.process->serial);if(row==state_->faces.end() || row->second.process!=call.process || !call.has_self || call.argument_count)return {};
        if(call.target==FaceDestroy){if(call.kind!=ProcessCallKind::Destroy)return {};}
        else if(call.target!=FaceTick || call.kind!=ProcessCallKind::Descriptor || call.command!=13)return {};
    }
    return next;
}
}

namespace fates::runtime::native {
TalkControlStatus NativeTalkControlContext::WriteCharacterState(ProcessHandle manager,const TalkCharacterUpdate& value,ProcessAccess& access) {
    const auto scheduler=scheduler_.lock();
    if(!scheduler || !access.BelongsTo(*scheduler))return TalkControlStatus::MismatchedDomain;
    if(!Observe(manager))return TalkControlStatus::InvalidParent;
    auto& row=rows_.at(manager->serial);
    if(value.cursor)row.message_cursor=*value.cursor;
    if(value.delay)row.character_delay=*value.delay;
    if(value.selection)row.selection_pending=*value.selection;
    if(value.parity)row.character_parity=*value.parity;
    if(value.pulse)row.letter_pulse=*value.pulse;
    if(value.skip)row.skip=*value.skip;
    if(value.voice_first){row.pending_voice[0]=*value.voice_first;row.pending_voice_known.set(0);}
    return TalkControlStatus::Ready;
}
}

namespace fates::runtime::native {
TalkControlStatus NativeTalkControlContext::WriteLifecycleState(ProcessHandle manager,
    const TalkLifecycleUpdate& change,ProcessAccess& access) {
    const auto owner=scheduler_.lock();
    if(!owner || !access.BelongsTo(*owner))return TalkControlStatus::MismatchedDomain;
    if(!Observe(manager))return TalkControlStatus::InvalidParent;
    if(change.auxiliary && *change.auxiliary && !owner->Observe(*change.auxiliary))return TalkControlStatus::InvalidHandle;
    auto& row=rows_.at(manager->serial);
    if(change.render)row.render_enabled=*change.render;
    if(change.drawer)row.drawer_acquired=*change.drawer;
    if(change.delete_in_skip)row.delete_in_skip=*change.delete_in_skip;
    if(change.ending_fade)row.ending_fade=*change.ending_fade;
    if(change.auxiliary)row.auxiliary=*change.auxiliary;
    return TalkControlStatus::Ready;
}
}

namespace fates::runtime::native {
TalkControlStatus NativeTalkControlContext::ConstructMembers(ProcessHandle manager,
    const std::array<presentation::native::TalkWindowHandle,3>& slots,
    presentation::native::TalkBackgroundHandle background,ProcessAccess& access) {
    if(Observe(manager))return TalkControlStatus::DuplicateBinding;
    TalkControlManagerView row;row.manager=manager;row.window=slots[0];row.system_window=slots[2];row.background=background;
    for(unsigned i=0;i<3;++i)row.window_slots[i]=slots[i];
    row.message_cursor=presentation::native::TalkWindowSource{};row.auxiliary=ProcessHandle{};
    row.free_window_mode=std::uint8_t{2};row.screen_fade_duration=std::uint32_t{500};
    row.render_enabled=std::uint8_t{0};row.skip=std::uint8_t{0};row.delete_in_skip=std::uint8_t{0};
    row.suppress_face_fade=std::uint8_t{0};row.drawer_acquired=std::uint8_t{0};row.saved_skip_flags=std::uint32_t{0};
    row.shadow_enabled=std::uint8_t{1};row.reveal_counter=std::uint32_t{0};
    row.initialize_face_color=std::uint8_t{1};row.initialize_face_motion=std::uint8_t{1};
    row.fade_second_target=std::uint8_t{0};row.alternate_wait_clock=std::uint8_t{0};
    const auto owner=scheduler_.lock();
    if(!owner || !access.BelongsTo(*owner))return TalkControlStatus::MismatchedDomain;
    const auto process=access.Observe(manager);
    if(process && process->constructing && !process->linked && !(process->flags&1u)) {
        if(!windows_ || !backgrounds_ || !windows_->UsesScheduler(*owner) || !backgrounds_->UsesScheduler(*owner))return TalkControlStatus::MismatchedDomain;
        for(const auto& slot:slots)if(!slot || !windows_->Observe(slot))return TalkControlStatus::InvalidHandle;
        if(!background || !backgrounds_->Observe(background))return TalkControlStatus::InvalidHandle;
        rows_.emplace(manager->serial,std::move(row));return TalkControlStatus::Ready;
    }
    return RestoreCarried(row,&access);
}
TalkControlStatus NativeTalkControlContext::WriteSavedSkipFlags(ProcessHandle manager,std::uint32_t bits,ProcessAccess& access) {
    const auto owner=scheduler_.lock();if(!owner || !access.BelongsTo(*owner))return TalkControlStatus::MismatchedDomain;
    const auto process=access.Observe(manager);auto found=manager?rows_.find(manager->serial):rows_.end();
    if(!process || (!process->linked && !process->constructing) || (process->flags&1u) || found==rows_.end() || found->second.manager!=manager)return TalkControlStatus::InvalidParent;
    found->second.saved_skip_flags=bits;return TalkControlStatus::Ready;
}
TalkControlStatus NativeTalkControlContext::ClearOwnedAuxiliary(ProcessHandle manager,ProcessAccess& access) {
    const auto owner=scheduler_.lock();if(!owner || !access.BelongsTo(*owner))return TalkControlStatus::MismatchedDomain;
    const auto process=access.Observe(manager);auto found=manager?rows_.find(manager->serial):rows_.end();
    if(!process || process->linked || !(process->flags&1u) || found==rows_.end() || found->second.manager!=manager)return TalkControlStatus::InvalidParent;
    found->second.auxiliary=ProcessHandle{};return TalkControlStatus::Ready;
}
TalkControlStatus NativeTalkControlContext::RetireOwnedMembers(ProcessHandle manager,ProcessAccess& access) {
    const auto owner=scheduler_.lock();if(!owner || !access.BelongsTo(*owner))return TalkControlStatus::MismatchedDomain;
    const auto process=access.Observe(manager);auto found=manager?rows_.find(manager->serial):rows_.end();
    if(!process || process->linked || !(process->flags&1u) || found==rows_.end() || found->second.manager!=manager)return TalkControlStatus::InvalidParent;
    rows_.erase(found);return TalkControlStatus::Ready;
}
}
