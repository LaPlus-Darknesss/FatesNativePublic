#include "fates/runtime/native_talk_reveal.hpp"
#include <bit>
#include <limits>

namespace fates::runtime::native {
using namespace presentation::native;
using S=TalkControlStatus;
namespace {
constexpr std::uint32_t Append=0x1e4e88,Tick=0x1e5a88;
ProcessCall Service(ProcessHandle owner,std::uint32_t target) {
    ProcessCall call;call.process=std::move(owner);call.target=target;call.kind=ProcessCallKind::Service;return call;
}
}
struct NativeTalkReveal::State {
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<NativeTalkControlContext> managers;std::shared_ptr<NativeTalkWindow> windows;
    std::shared_ptr<NativeTalkWindowEffects> effects;std::shared_ptr<NativeTalkMotion> motion;std::shared_ptr<NativeTalkMessageWidth> widths;
    std::shared_ptr<NativeTalkSpeaker> speaker;std::shared_ptr<NativeTalkControlEffects> controls;
    std::shared_ptr<NativeTalkLog> log;std::shared_ptr<NativeTalkTokens> tokens;
    std::shared_ptr<NativeGameSkip> skip;std::shared_ptr<GameSkipInputSource> input;
};
struct NativeTalkReveal::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;ProcessCall call;unsigned stage{};
    TalkWidthRequest width;TalkControlRequest control;
    TalkWindowHandle window;TalkWindowSource source;TalkLogSource talker;
    std::uint32_t measured{},result{},burst{1024};bool accelerated{},ordinary{};
    ~Continuation() override {if(width)state->widths->Forget(width);if(control)state->controls->Forget(control);}
    ProcessCallbackStep Block() const {return ProcessCallbackStep::Blocked();}
    std::optional<TalkControlManagerView> Manager() const {return state->managers->Observe(call.process);}
    bool Write(ProcessAccess& access,const TalkCharacterUpdate& value) {return state->managers->WriteCharacterState(call.process,value,access)==S::Ready;}
    bool Cursor() {const auto manager=Manager();if(!manager || !manager->message_cursor)return false;source=*manager->message_cursor;return true;}
    bool Window() {const auto manager=Manager();if(!manager || !manager->window || !*manager->window)return false;window=*manager->window;return bool(state->windows->Observe(window));}
    std::optional<char16_t> Read(const TalkWindowSource& value,std::size_t index=0) const {
        const auto row=state->windows->Read(value,index);return row.status==TalkWindowStatus::Ready?std::optional{row.value}:std::nullopt;
    }
    bool Advance(ProcessAccess& access,TalkWindowSource value,std::size_t amount=1) {
        if(amount>std::numeric_limits<std::size_t>::max()-value.offset)return false;
        value.offset+=amount;TalkCharacterUpdate change;change.cursor=std::move(value);return Write(access,change);
    }
    ProcessCallbackStep Measure(ProcessAccess& access,unsigned after) {
        if(!width && state->widths->Prepare(call.process,window,source,0,width,&access)!=TalkMessageWidthStatus::Ready)return Block();
        const auto nested=state->widths->Call(width);if(!nested)return Block();
        stage=after;return ProcessCallbackStep::Call(*nested);
    }
    bool FinishMeasure(ProcessAccess& access) {
        const auto row=state->widths->Observe(width);if(!row || !row->completed || !row->result)return false;
        measured=*row->result;if(state->widths->Release(width,&access)!=TalkMessageWidthStatus::Ready)return false;
        width.reset();return true;
    }
    ProcessCallbackStep Effect(TalkWindowEffect effect,std::int32_t value,unsigned after) {
        const auto nested=state->effects->Call(call.process,window,effect,value);if(!nested)return Block();
        stage=after;return ProcessCallbackStep::Call(*nested);
    }
    ProcessCallbackStep AppendStep(ProcessAccess& access) {
        if(stage==0) {
            const auto manager=Manager();if(!manager || !manager->character_delay)return Block();
            if(std::bit_cast<std::int32_t>(*manager->character_delay)>0)stage=1;
            else {TalkCharacterUpdate change;change.delay=std::uint32_t{1};if(!Write(access,change))return Block();stage=2;}
        }
        if(stage==1) {
            if(!Cursor())return Block();
            const auto word=Read(source);if(!word)return Block();
            if(*word==u'$') {
                // Original rewinds the second '$' when a doubled escape waits.
                // Before the retained source extent is a native admission barrier.
                if(!source.offset)return Block();
                --source.offset;TalkCharacterUpdate change;change.cursor=source;if(!Write(access,change))return Block();
            }
            return ProcessCallbackStep::Return(2);
        }
        if(stage==2) {
            if(!Window())return Block();
            const auto row=state->windows->Observe(window);
            stage=std::bit_cast<std::int32_t>(row->line)>=2?3u:5u;
        }
        if(stage==3) {
            const auto row=Manager();if(!row || !row->skip)return Block();
            if(*row->skip){if(state->windows->ResetStrings(window,&access)!=TalkWindowStatus::Ready)return Block();
            return ProcessCallbackStep::Return(3);}
            const auto child=state->motion->NextPageCall(call.process,window);
            if(!child)return Block();
            stage=99;return ProcessCallbackStep::Call(*child);
        }
        if(stage==5) {
            const auto row=state->windows->Observe(window);if(!row)return Block();stage=row->active?10u:6u;
        }
        if(stage==6) {if(!Cursor())return Block();
        stage=7;}
        if(stage==7)return Measure(access,8);
        if(stage==8){if(!FinishMeasure(access))return Block();
        stage=9;}
        if(stage==9){if(!Window())return Block();
        return Effect(TalkWindowEffect::StartOpen,std::bit_cast<std::int32_t>(measured),99);}
        if(stage==10) {
            const auto row=Manager();if(!row || !row->selection_pending)return Block();
            if(*row->selection_pending){TalkCharacterUpdate change;change.selection=std::uint8_t{0};if(!Write(access,change))return Block();
            stage=11;}
            else stage=20;
        }
        if(stage==11) {
            const auto name=state->speaker->Get(window,&access);if(name.status!=TalkSpeakerStatus::Ready)return Block();
            talker={};
            if(name.source.kind!=TalkWindowSource::Kind::Null){talker.kind=TalkLogSource::Kind::Reader;
                const auto windows=state->windows;const auto name_source=name.source;
                talker.reader=[windows,name_source](std::size_t at)->std::optional<char16_t>{const auto word=windows->Read(name_source,at);return word.status==TalkWindowStatus::Ready?std::optional{word.value}:std::nullopt;};}
            stage=12;
        }
        if(stage==12){if(state->log->SetTalker(talker)!=TalkLogStatus::Ready)return Block();
        stage=13;}
        if(stage==13){if(!Window() || !Cursor())return Block();
        stage=14;}
        if(stage==14)return Measure(access,15);
        if(stage==15){if(!FinishMeasure(access))return Block();
        stage=16;}
        if(stage==16){if(!Window())return Block();
        return Effect(TalkWindowEffect::WaitNextMessage,0,98);}
        if(stage==20) {
            if(!Cursor())return Block();
            const auto word=Read(source);if(!word)return Block();
            if(state->log->Append(*word)!=TalkLogStatus::Ready)return Block();
            stage=21;
        }
        if(stage==21) {
            if(!Cursor() || !Window())return Block();
            if(state->windows->AddLetter(window,source,&access)!=TalkWindowStatus::Ready)return Block();
            stage=22;
        }
        if(stage==22){if(!Advance(access,source))return Block();
        stage=23;}
        if(stage==23) {
            const auto row=Manager();if(!row || !row->character_parity)return Block();
            const auto old=std::int32_t(std::bit_cast<std::int8_t>(*row->character_parity));
            const auto parity=static_cast<std::uint8_t>((old+1)%2);TalkCharacterUpdate change;change.parity=parity;
            if(!parity)change.pulse=std::uint8_t{1};
            if(!Write(access,change))return Block();
            stage=24;
        }
        if(stage==24){const auto row=Manager();if(!row || !row->pending_voice_known.test(0))return Block();
        if(!row->pending_voice[0])return ProcessCallbackStep::Return(3);
        stage=25;}
        if(stage==25){const auto row=Manager();if(!row || !row->skip)return Block();
        stage=*row->skip?28u:26u;}
        if(stage==26){auto nested=Service(call.process,0x41ffe4);nested.argument_count=1;nested.arguments[0]=0x129e;stage=27;return ProcessCallbackStep::Call(nested);}
        if(stage==27){auto nested=Service(call.process,0x41fad4);nested.argument_count=1;nested.arguments[0]=0x129e;stage=28;return ProcessCallbackStep::Call(nested);}
        if(stage==28){TalkCharacterUpdate change;change.voice_first=std::uint8_t{0};if(!Write(access,change))return Block();
        return ProcessCallbackStep::Return(3);}
        return ProcessCallbackStep::Return(stage==98?2u:3u);
    }
    ProcessCallbackStep TickStep(ProcessAccess& access) {
        if(stage==0) {
            const auto current=state->skip->Current();if(!current)return Block();
            if(*current){const auto row=state->skip->Observe(*current);if(!row)return Block();
            if(row->state){if(access.Jump(call.process,5)!=ProcessStatus::Ready)return Block();
            return ProcessCallbackStep::Return();}}
            stage=1;
        }
        if(stage==1) {
            const auto held=state->input->HeldButtons();if(!held)return Block();accelerated=(*held&0x81u)==0x81u;
            TalkCharacterUpdate change;change.skip=static_cast<std::uint8_t>(accelerated);if(!Write(access,change))return Block();stage=2;
        }
        if(stage==2){const auto trigger=state->input->TriggerButtons();if(!trigger)return Block();
        accelerated=accelerated || bool(*trigger&0x800u);stage=3;}
        if(stage==3) {
            const auto manager=Manager();if(!manager || !manager->character_delay)return Block();
            TalkCharacterUpdate change;change.delay=accelerated?std::uint32_t{0}:*manager->character_delay-access.frame_delta();
            if(!Write(access,change))return Block();
            stage=4;
        }
        if(stage==4) {
            if(!Cursor())return Block();
            const auto word=Read(source);if(!word)return Block();
            ordinary=false;
            if(!*word)stage=20;else if(*word==u'\n')stage=23;else if(*word==u'$')stage=5;
            else {ordinary=true;stage=9;return ProcessCallbackStep::Call(AppendCharacterCall(call.process));}
        }
        if(stage==5) {
            const auto next=Read(source,1);if(!next)return Block();
            if(*next==u'$'){if(!Advance(access,source))return Block();
            stage=9;return ProcessCallbackStep::Call(AppendCharacterCall(call.process));}
            stage=6;
        }
        if(stage==6) {
            if(!control && state->controls->Prepare(call.process,source,0,TalkCodeOperation::Dispose,control,&access)!=S::Ready)return Block();
            const auto nested=state->controls->Call(control);if(!nested)return Block();stage=7;return ProcessCallbackStep::Call(*nested);
        }
        if(stage==7) {
            const auto value=state->controls->Observe(control);if(!value || !value->completed || !value->result)return Block();result=*value->result;
            if(state->controls->Release(control,&access)!=S::Ready)return Block();
            control.reset();stage=result==1?26u:8u;
        }
        if(stage==8) {
            // Dispose can change current manager text; Skip reads it again.
            if(!Cursor())return Block();
            NativeTalkControlScanner scanner(*state->tokens);
            const auto skipped=scanner.Skip([this](std::size_t at){return Read(source,at);},0);
            if(skipped.status!=TalkCodeStatus::Ready)return Block();
            if(!skipped.next){TalkCharacterUpdate change;change.cursor=TalkWindowSource{};if(!Write(access,change))return Block();
            }
            else if(!Advance(access,source,*skipped.next))return Block();
            stage=26;
        }
        if(stage==9){result=access.call_result();if(ordinary && burst && result==3){--burst;result=4;}stage=26;}
        if(stage==20){if(state->log->NextLine()!=TalkLogStatus::Ready)return Block();
        stage=21;}
        if(stage==21){if(state->log->NextLine()!=TalkLogStatus::Ready)return Block();
        stage=22;}
        if(stage==22){if(access.Next(call.process)!=ProcessStatus::Ready)return Block();
        result=0;stage=26;}
        if(stage==23){if(!Advance(access,source))return Block();
        stage=24;}
        if(stage==24){if(!Window() || state->windows->NextLine(window,&access)!=TalkWindowStatus::Ready)return Block();
        stage=25;}
        if(stage==25){if(state->log->NextLine()!=TalkLogStatus::Ready)return Block();
        result=4;stage=26;}
        if(stage==26) {
            if(result==4 || (accelerated && result==3)){stage=3;return ProcessCallbackStep::Continue();}
            stage=27;
        }
        if(stage==27){const auto manager=Manager();if(!manager || !manager->letter_pulse)return Block();
        if(*manager->letter_pulse){TalkCharacterUpdate change;change.pulse=std::uint8_t{0};if(!Write(access,change))return Block();
        }return ProcessCallbackStep::Return();}
        return Block();
    }
    ProcessCallbackStep Step(ProcessAccess& access) override {
        const auto owner=state->scheduler.lock();if(!owner || !access.BelongsTo(*owner) || !Manager())return Block();
        return call.target==Append?AppendStep(access):TickStep(access);
    }
};
NativeTalkReveal::NativeTalkReveal(std::shared_ptr<State> state):state_(std::move(state)){}
NativeTalkReveal::~NativeTalkReveal()=default;
S NativeTalkReveal::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<NativeTalkControlContext> managers,std::shared_ptr<NativeTalkWindow> windows,
    std::shared_ptr<NativeTalkWindowEffects> effects,std::shared_ptr<NativeTalkMotion> motion,std::shared_ptr<NativeTalkMessageWidth> widths,
    std::shared_ptr<NativeTalkSpeaker> speaker,std::shared_ptr<NativeTalkControlEffects> controls,
    std::shared_ptr<NativeTalkLog> log,std::shared_ptr<NativeTalkTokens> tokens,std::shared_ptr<NativeGameSkip> skip,
    std::shared_ptr<GameSkipInputSource> input,std::shared_ptr<NativeTalkReveal>& output) {
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !managers || !managers->UsesScheduler(*scheduler)
        || !windows || !windows->UsesScheduler(*scheduler) || !managers->UsesWindows(*windows)
        || !motion || !motion->UsesWindow(*windows) || !motion->UsesScheduler(*scheduler)
        || !effects || !effects->UsesOwners(*windows,*managers) || !widths || !widths->UsesOwners(*managers,*windows)
        || !speaker || !speaker->UsesWindows(*windows) || !controls || !log || !tokens || !skip || !input
        || !skip->UsesScheduler(*scheduler) || !skip->UsesInput(*input) || !widths->UsesTokens(*tokens)
        || !controls->UsesTokens(*tokens) || !controls->UsesOwners(*managers,*windows,*log,*skip))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->managers=std::move(managers);state->windows=std::move(windows);
    state->effects=std::move(effects);state->motion=std::move(motion);state->widths=std::move(widths);state->speaker=std::move(speaker);state->controls=std::move(controls);
    state->log=std::move(log);state->tokens=std::move(tokens);state->skip=std::move(skip);state->input=std::move(input);
    auto result=std::shared_ptr<NativeTalkReveal>(new NativeTalkReveal(state));
    if(!registry->Register(std::array{Append,Tick},result))return S::DuplicateBinding;
    output=std::move(result);return S::Ready;
}
ProcessCall NativeTalkReveal::AppendCharacterCall(ProcessHandle manager){return Service(std::move(manager),Append);}
ProcessCall NativeTalkReveal::TickCall(ProcessHandle manager){return Service(std::move(manager),Tick);}
bool NativeTalkReveal::UsesScheduler(const NativeProcessScheduler& owner) const noexcept{return state_->scheduler.lock().get()==&owner;}
std::unique_ptr<ProcessContinuation> NativeTalkReveal::Begin(const ProcessCall& call) {
    const auto scheduler=state_->scheduler.lock();if(!scheduler || !scheduler->root(2) || !state_->managers->Observe(call.process)
        || call.argument_count || call.this_adjustment || (call.target!=Append && call.target!=Tick))return {};
    const bool service=call.kind==ProcessCallKind::Service && !call.has_self;
    const bool tick=call.target==Tick && call.kind==ProcessCallKind::Descriptor && call.command==13 && call.has_self;
    if(!service && !tick)return {};
    const auto owner=scheduler->Observe(call.process);if(!owner || !owner->linked || (owner->flags&1u))return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->call=call;return next;
}
}
