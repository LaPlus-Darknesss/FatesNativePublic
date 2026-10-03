#include "fates/runtime/native_talk_mode_fade.hpp"
#include <algorithm>
#include <bit>
#include <limits>
#include <map>
namespace fates::runtime::native {
using namespace presentation::native;using S=TalkControlStatus;
namespace {
constexpr std::uint32_t TypeDispose=0x1c0420,TypeFlash=0x1c040c,FadeDispose=0x4f31b4,FadeFlash=0x4f3188,
    SetType=0x1e4b60,FadeIn=0x1e5c5c,BlackOut=0x1e6120,WhiteOut=0x1e61f4,IsFading=0x1e61dc;
constexpr std::array Targets{TypeDispose,TypeFlash,FadeDispose,FadeFlash,SetType,FadeIn,BlackOut,WhiteOut,IsFading};
ProcessCall Service(ProcessHandle parent,std::uint32_t target,std::initializer_list<std::uint32_t> args={}){
    ProcessCall call;call.process=std::move(parent);call.kind=ProcessCallKind::Service;call.target=target;
    call.argument_count=static_cast<std::uint8_t>(args.size());std::copy(args.begin(),args.end(),call.arguments.begin());return call;
}
}
struct NativeTalkModeFade::State {
    struct Request{TalkModeFadeObservation view;TalkWindowSource source;std::size_t start{};TalkCodeOperation operation{};char16_t handler{};};
    std::weak_ptr<NativeProcessScheduler> scheduler;std::shared_ptr<NativeTalkControlContext> managers;
    std::shared_ptr<NativeTalkWindow> windows;std::shared_ptr<NativeFadeSystem> fade;std::shared_ptr<NativeGameSkip> skip;
    TalkPriorityState priority;std::map<std::uint32_t,std::shared_ptr<Request>> requests;std::uint32_t serial{};
    S Mutable(ProcessAccess* access)const {
        const auto owner=scheduler.lock();if(!owner || !owner->root(2))return S::Retired;
        if(access)return access->BelongsTo(*owner)?S::Ready:S::MismatchedDomain;
        return owner->busy()?S::Busy:S::Ready;
    }
    std::shared_ptr<Request> Get(TalkModeFadeRequest handle)const {
        if(!handle)return {};
        const auto it=requests.find(handle->serial);return it!=requests.end() && it->second->view.identity==handle?it->second:nullptr;
    }
};
struct NativeTalkModeFade::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;std::shared_ptr<State::Request> request;ProcessCall call;
    unsigned stage{},index{};char16_t command{},subtype{};std::uint32_t duration{};bool flash{};
    TalkWindowHandle target;FadeColor color{};
    ProcessCallbackStep Block(S why=S::Unavailable){if(request)request->view.status=why;return ProcessCallbackStep::Blocked();}
    ProcessCallbackStep Done(std::uint32_t value){
        if(request){request->view.completed=true;request->view.status=S::Ready;request->view.result=flash?std::nullopt:std::optional{value};}
        return ProcessCallbackStep::Return(flash?0u:value);
    }
    ProcessCallbackStep Type(ProcessAccess& access){
        if(stage==0){
            const auto row=state->managers->Observe(call.process);if(!row || !row->free_window_mode)return Block();
            const auto old=std::int32_t(std::bit_cast<std::int8_t>(*row->free_window_mode));
            stage=old==std::bit_cast<std::int32_t>(call.arguments[0])?2u:1u;
        }
        if(stage==1){
            for(;index<3;++index){
                const auto row=state->managers->Observe(call.process);if(!row || !row->window_slots[index])return Block();
                if(state->windows->Reset(*row->window_slots[index],&access)!=TalkWindowStatus::Ready)return Block();
            }
            stage=2;
        }
        if(stage==2){
            if(state->managers->WriteTalkMode(call.process,static_cast<std::uint8_t>(call.arguments[0]),access)!=S::Ready)return Block();
            stage=3;
        }
        if(stage==3){
            if(call.arguments[0]>2)return ProcessCallbackStep::Return(); // Full-width branch, stored byte alone is not the selector.
            if(!state->priority.base)return Block();
            const auto offset=call.arguments[0]==0?256u:call.arguments[0]==1?128u:std::uint32_t{0}-128u;
            state->priority.window=static_cast<std::uint16_t>(std::uint32_t(*state->priority.base)+offset);stage=4;
        }
        if(call.arguments[0]==2){
            if(stage==4){
                const auto row=state->managers->Observe(call.process);if(!row || !row->system_window || !*row->system_window)return Block();
                if(state->windows->ResetSystem(*row->system_window,&access)!=TalkWindowStatus::Ready)return Block();
                stage=5;
            }
            const auto row=state->managers->Observe(call.process);if(!row || !row->system_window || !*row->system_window)return Block();
            if(state->managers->SelectCurrentWindow(call.process,*row->system_window,access)!=S::Ready)return Block();
        }
        return ProcessCallbackStep::Return();
    }
    ProcessCallbackStep Fade(ProcessAccess& access){
        if(stage==0){
            const auto current=state->skip->Current();if(!current)return Block();
            if(*current){const auto row=state->skip->Observe(*current);if(!row)return Block();stage=row->state?1u:3u;}
            else stage=3;
        }
        if(stage==1){
            // The original dereferences current again after IsSkip, not the captured instance.
            const auto current=state->skip->Current();if(!current)return Block();
            if(!*current)return ProcessCallbackStep::Return();
            if(!state->skip->Observe(*current))return Block();
            if(call.target==FadeIn){const auto fade=state->fade->ObserveChannel(0);if(!fade)return Block();color=fade->color;color[3]=0;}
            else color=call.target==WhiteOut?FadeColor{255,255,255,255}:FadeColor{0,0,0,255};
            if(state->skip->SetSavedFadeGoal(access,*current,0,color)!=GameSkipStatus::Ready)return Block();
            return ProcessCallbackStep::Return();
        }
        const auto tone=call.target==WhiteOut?FadeTone::White:FadeTone::Black;
        const auto direction=call.target==FadeIn?FadeDirection::In:FadeDirection::Out;
        const auto ms=std::bit_cast<std::int32_t>(call.arguments[0]);
        if(stage==3){stage=4;return ProcessCallbackStep::Call(NativeFadeSystem::FadeCall(call.process,ms,0,tone,direction));}
        if(stage==4){
            const auto row=state->managers->Observe(call.process);if(!row || !row->fade_second_target)return Block();
            stage=5;if(*row->fade_second_target)return ProcessCallbackStep::Call(NativeFadeSystem::FadeCall(call.process,ms,1,tone,direction));
        }
        if(stage==5){stage=6;return ProcessCallbackStep::Call(NativeFadeSystem::WaitCall(call.process,0));}
        return ProcessCallbackStep::Return();
    }
    ProcessCallbackStep Step(ProcessAccess& access)override {
        if(state->Mutable(&access)!=S::Ready)return Block(S::Retired);
        if(call.target==SetType)return Type(access);
        if(call.target==FadeIn || call.target==BlackOut || call.target==WhiteOut)return Fade(access);
        if(call.target==IsFading){const auto row=state->fade->ObserveChannel(0);return row?ProcessCallbackStep::Return(row->color[3]?1u:0u):Block();}
        if(!request || request->view.completed)return Block(S::InvalidHandle);
        request->view.status=S::Ready;
        const auto reader=[this](std::size_t at)->std::optional<char16_t>{const auto value=state->windows->Read(request->source,at);return value.status==TalkWindowStatus::Ready?std::optional{value.value}:std::nullopt;};
        if(stage==0){
            if(request->start>std::numeric_limits<std::size_t>::max()-3)return Block(S::InvalidSource);
            const auto sub=reader(request->start+2);if(!sub)return Block(S::InvalidSource);
            command=request->handler;subtype=*sub;flash=request->operation==TalkCodeOperation::Flash;stage=1;
        }
        if(command==u'F'){
            if(stage==1){
                const auto value=TalkDecimalize(reader,request->start+3);if(value.status!=TalkArgumentStatus::Ready)return Block(S::InvalidSource);
                duration=value.value;
                if(std::bit_cast<std::int32_t>(duration)>=0 && state->managers->WriteFadeDuration(call.process,duration,access)!=S::Ready)return Block();
                stage=2;
            }
            if(stage==2){
                if(subtype!=u'i' && subtype!=u'o' && subtype!=u'w')return Done(2);
                const auto row=state->managers->Observe(call.process);if(!row || !row->screen_fade_duration)return Block();
                stage=3;return ProcessCallbackStep::Call(Service(call.process,subtype==u'i'?FadeIn:subtype==u'o'?BlackOut:WhiteOut,{*row->screen_fade_duration}));
            }
            return Done(2);
        }
        if(stage==1){
            stage=2;if(subtype>=u'0' && subtype<=u'2')return ProcessCallbackStep::Call(Service(call.process,SetType,{std::uint32_t(subtype-u'0')}));
        }
        if(stage==2){stage=3;return ProcessCallbackStep::Call(Service(call.process,IsFading));}
        if(stage==3){stage=4;if(access.call_result())return ProcessCallbackStep::Call(Service(call.process,FadeIn,{500}));stage=5;}
        if(stage==4){
            const auto row=state->managers->Observe(call.process);if(!row || !row->free_window_mode)return Block();
            if(*row->free_window_mode==1 && state->managers->WriteRevealCounter(call.process,30,access)!=S::Ready)return Block();
            // 1C04BC branches to 1C04CC (return4), not the label3 branch.
            return Done(4);
        }
        if(stage==5){
            const auto row=state->managers->Observe(call.process);if(!row || !row->free_window_mode)return Block();
            if(*row->free_window_mode!=1)return Done(4);
            if(access.Jump(call.process,3)!=ProcessStatus::Ready)return Block();
            stage=6;
        }
        return Done(2);
    }
};
NativeTalkModeFade::NativeTalkModeFade(std::shared_ptr<State> state):state_(std::move(state)){}
NativeTalkModeFade::~NativeTalkModeFade()=default;
S NativeTalkModeFade::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<NativeTalkControlContext> managers,std::shared_ptr<NativeTalkWindow> windows,
    std::shared_ptr<NativeFadeSystem> fade,std::shared_ptr<NativeGameSkip> skip,std::shared_ptr<NativeTalkModeFade>& out){
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !managers || !managers->UsesScheduler(*scheduler)
        || !windows || !windows->UsesScheduler(*scheduler) || !managers->UsesWindows(*windows) || !fade || !fade->UsesScheduler(*scheduler)
        || !skip || !skip->UsesScheduler(*scheduler) || !skip->UsesFadeSystem(*fade))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->managers=std::move(managers);state->windows=std::move(windows);state->fade=std::move(fade);state->skip=std::move(skip);
    auto next=std::shared_ptr<NativeTalkModeFade>(new NativeTalkModeFade(state));if(!registry->Register(Targets,next))return S::DuplicateBinding;
    out=std::move(next);return S::Ready;
}
S NativeTalkModeFade::RestorePriority(TalkPriorityState value,ProcessAccess* access){if(const auto status=state_->Mutable(access);status!=S::Ready)return status;state_->priority=std::move(value);return S::Ready;}
std::optional<TalkPriorityState> NativeTalkModeFade::Priority()const {return state_->Mutable(nullptr)==S::Retired?std::nullopt:std::optional{state_->priority};}
S NativeTalkModeFade::Prepare(ProcessHandle manager,TalkWindowSource source,std::size_t start,TalkCodeOperation op,char16_t handler,TalkModeFadeRequest& out,ProcessAccess* access){
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    if(!state_->managers->Observe(manager))return S::InvalidParent;
    if((op!=TalkCodeOperation::Dispose && op!=TalkCodeOperation::Flash) || (handler!=u't' && handler!=u'F'))return S::Unsupported;
    if(state_->serial==std::numeric_limits<std::uint32_t>::max())return S::IdentityExhausted;
    auto row=std::make_shared<State::Request>();row->view.identity=std::make_shared<TalkModeFadeIdentity>(++state_->serial);row->view.manager=manager;
    row->source=std::move(source);row->start=start;row->operation=op;row->handler=handler;state_->requests.emplace(row->view.identity->serial,row);out=row->view.identity;return S::Ready;
}
std::optional<ProcessCall> NativeTalkModeFade::Call(TalkModeFadeRequest h)const {
    const auto row=state_->Get(h);if(!row || row->view.completed || state_->Mutable(nullptr)==S::Retired)return {};
    // The dispatcher already read the handler code. Do not reread that live word.
    const bool flash=row->operation==TalkCodeOperation::Flash;
    return Service(row->view.manager,row->handler==u't'?(flash?TypeFlash:TypeDispose):(flash?FadeFlash:FadeDispose),{h->serial});
}
std::optional<TalkModeFadeObservation> NativeTalkModeFade::Observe(TalkModeFadeRequest h)const {
    const auto row=state_->Get(h);return row && state_->Mutable(nullptr)!=S::Retired && state_->managers->Observe(row->view.manager)?std::optional{row->view}:std::nullopt;
}
S NativeTalkModeFade::Release(TalkModeFadeRequest h,ProcessAccess* access){if(const auto status=state_->Mutable(access);status!=S::Ready)return status;if(!state_->Get(h))return S::InvalidHandle;state_->requests.erase(h->serial);return S::Ready;}
void NativeTalkModeFade::Forget(TalkModeFadeRequest h)noexcept {if(state_->Get(h))state_->requests.erase(h->serial);}
ProcessCall NativeTalkModeFade::SetTalkTypeCall(ProcessHandle p,std::uint32_t type){return Service(std::move(p),SetType,{type});}
ProcessCall NativeTalkModeFade::FadeInCall(ProcessHandle p,std::int32_t ms){return Service(std::move(p),FadeIn,{std::bit_cast<std::uint32_t>(ms)});}
ProcessCall NativeTalkModeFade::BlackOutCall(ProcessHandle p,std::int32_t ms){return Service(std::move(p),BlackOut,{std::bit_cast<std::uint32_t>(ms)});}
ProcessCall NativeTalkModeFade::WhiteOutCall(ProcessHandle p,std::int32_t ms){return Service(std::move(p),WhiteOut,{std::bit_cast<std::uint32_t>(ms)});}
ProcessCall NativeTalkModeFade::IsFadingCall(ProcessHandle p){return Service(std::move(p),IsFading);}
bool NativeTalkModeFade::UsesOwners(const NativeTalkControlContext& m,const NativeTalkWindow& w,const NativeGameSkip& s)const noexcept {return state_->managers.get()==&m && state_->windows.get()==&w && state_->skip.get()==&s;}
std::unique_ptr<ProcessContinuation> NativeTalkModeFade::Begin(const ProcessCall& call){
    if(state_->Mutable(nullptr)==S::Retired || !call.process || call.kind!=ProcessCallKind::Service || call.has_self || call.this_adjustment || std::find(Targets.begin(),Targets.end(),call.target)==Targets.end())return {};
    if(call.argument_count!=(call.target==IsFading?0u:1u))return {};
    const auto owner=state_->scheduler.lock();const auto process=owner->Observe(call.process);if(!process || !process->linked || (process->flags&1u))return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->call=call;
    if(call.target==TypeDispose || call.target==TypeFlash || call.target==FadeDispose || call.target==FadeFlash){
        const auto it=state_->requests.find(call.arguments[0]);if(it==state_->requests.end() || it->second->view.manager!=call.process || it->second->view.completed)return {};
        const bool flashing=call.target==TypeFlash || call.target==FadeFlash;
        if(flashing!=(it->second->operation==TalkCodeOperation::Flash) || ((call.target==TypeDispose || call.target==TypeFlash)!=(it->second->handler==u't')))return {};
        next->request=it->second;
    }
    return next;
}
}
