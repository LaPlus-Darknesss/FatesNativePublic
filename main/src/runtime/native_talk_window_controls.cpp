#include "fates/runtime/native_talk_window_controls.hpp"
#include <algorithm>
#include <bit>
#include <limits>
#include <map>
namespace fates::runtime::native {
using namespace presentation::native;
using S=TalkControlStatus;
namespace {
constexpr std::uint32_t Dispose=0x19049c,Flash=0x19028c,SetActive=0x18fdd0,Activate=0x1e57b8;
constexpr std::array Targets{Dispose,Flash,SetActive,Activate};
ProcessCall Service(ProcessHandle parent,std::uint32_t target,std::initializer_list<std::uint32_t> args={}) {
    ProcessCall call;call.process=std::move(parent);call.kind=ProcessCallKind::Service;call.target=target;
    call.argument_count=static_cast<std::uint8_t>(args.size());std::copy(args.begin(),args.end(),call.arguments.begin());return call;
}
}
struct NativeTalkWindowControls::State {
    struct Request{TalkWindowControlObservation view;TalkWindowSource source;std::size_t start{};TalkCodeOperation operation{};};
    std::weak_ptr<NativeProcessScheduler> scheduler;std::shared_ptr<NativeTalkControlContext> managers;
    std::shared_ptr<NativeTalkWindow> windows;std::shared_ptr<NativeTalkWindowEffects> effects;
    std::shared_ptr<NativeTalkMessageWidth> widths;std::shared_ptr<NativeGameSkip> skip;std::shared_ptr<NativeTalkLog> log;
    std::shared_ptr<NativeTalkTokens> tokens;std::shared_ptr<const NativeShiftJis> encoding;
    std::map<std::uint32_t,std::shared_ptr<Request>> requests;std::uint32_t serial{};
    S Mutable(ProcessAccess* access)const {
        const auto owner=scheduler.lock();if(!owner || !owner->root(2))return S::Retired;
        if(access)return access->BelongsTo(*owner)?S::Ready:S::MismatchedDomain;
        return owner->busy()?S::Busy:S::Ready;
    }
    std::shared_ptr<Request> Get(TalkWindowControlRequest handle)const {
        if(!handle)return {};
        const auto it=requests.find(handle->serial);return it!=requests.end() && it->second->view.identity==handle?it->second:nullptr;
    }
    bool ObserveSkipForMiss() const {
        const auto current=skip->Current();if(!current)return false;
        if(!*current)return true;
        const auto row=skip->Observe(*current);if(!row)return false;
        // Original GameSkip::IsSkip reads state!=0 here; caller ignores its
        // returned bool. Retain owner/lifetime observation without inventing
        // a diagnostic callback or a second global skip state.
        [[maybe_unused]] const bool ignored_result=row->state!=0;
        return true;
    }
    TalkWindowHandle BySerial(std::uint32_t id)const {for(const auto& h:windows->Handles())if(h->serial==id)return h;return {};}
};
struct NativeTalkWindowControls::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;std::shared_ptr<State::Request> request;ProcessCall call;
    unsigned stage{},index{};char16_t subtype{};bool flash{};TalkWindowHandle target;
    TalkWindowArgumentState argument;TalkWidthRequest measurement;
    ~Continuation()override {if(measurement)state->widths->Forget(measurement);}
    ProcessCallbackStep Block(S why=S::Unavailable){if(request)request->view.status=why;return ProcessCallbackStep::Blocked();}
    ProcessCallbackStep Done() {
        request->view.completed=true;request->view.status=S::Ready;
        request->view.result=flash?std::nullopt:std::optional{std::uint32_t{4}};
        return ProcessCallbackStep::Return(flash?0u:4u);
    }
    std::optional<TalkWindowHandle> Current()const {
        const auto row=state->managers->CurrentWindow(call.process);
        return row && *row && state->windows->Observe(*row)?row:std::nullopt;
    }
    ProcessCallbackStep Effect(TalkWindowEffect effect,unsigned next,std::uint32_t parameter=0) {
        const auto nested=state->effects->Call(call.process,target,effect,std::bit_cast<std::int32_t>(parameter));
        if(!nested)return Block();
        stage=next;return ProcessCallbackStep::Call(*nested);
    }
    ProcessCallbackStep Speaker(ProcessAccess& access) {
        const auto row=state->windows->Observe(target);if(!row)return Block(S::InvalidHandle);
        const bool enable=call.arguments[1]!=0;
        if(stage==0){
            // Old+5D5 affects only an enabling transition. A disable has no
            // log branch regardless of its unknown old byte.
            if(enable){
                if(!row->speaker_active)return Block();
                if(!*row->speaker_active && state->log->Observe()
                   && state->log->SetTalker({})!=TalkLogStatus::Ready)return Block();
            }
            stage=1;
        }
        if(state->windows->RestoreSpeakerActive(target,static_cast<std::uint8_t>(call.arguments[1]),&access)!=TalkWindowStatus::Ready)return Block();
        return ProcessCallbackStep::Return();
    }
    ProcessCallbackStep ActivateWindows(ProcessAccess&) {
        for(;;){
            if(stage==0){
                if(index==3){const auto window=Current();if(!window)return Block();target=*window;stage=4;
                    return ProcessCallbackStep::Call(Service(call.process,SetActive,{target->serial,1}));}
                const auto context=state->managers->Observe(call.process);
                if(!context || !context->window || !context->window_slots[index])return Block();
                target=*context->window_slots[index];
                if(target==*context->window){++index;continue;}
                if(!context->free_window_mode)return Block();
                if(*context->free_window_mode==1)return Effect(TalkWindowEffect::StartClose,2);
                stage=2;return ProcessCallbackStep::Call(Service(call.process,0x41f6c4,{100}));
            }
            if(stage==2){stage=3;return ProcessCallbackStep::Call(Service(call.process,SetActive,{target->serial,0}));}
            if(stage==3){++index;stage=0;continue;}
            return ProcessCallbackStep::Return();
        }
    }
    ProcessCallbackStep Step(ProcessAccess& access)override {
        if(state->Mutable(&access)!=S::Ready)return Block(S::Retired);
        if(call.target==SetActive)return Speaker(access);
        if(call.target==Activate)return ActivateWindows(access);
        if(!request || request->view.completed)return Block(S::InvalidHandle);
        request->view.status=S::Ready;
        const auto reader=[this](std::size_t at)->std::optional<char16_t>{
            const auto value=state->windows->Read(request->source,at);
            return value.status==TalkWindowStatus::Ready?std::optional{value.value}:std::nullopt;
        };
        if(stage==0){
            if(request->start>std::numeric_limits<std::size_t>::max()-2)return Block(S::InvalidSource);
            const auto word=reader(request->start+2);if(!word)return Block(S::InvalidSource);
            subtype=*word;flash=request->operation==TalkCodeOperation::Flash;stage=1;
        }
        if(stage==1){
            const auto status=argument.Step(reader,request->start+2,*state->tokens,*state->encoding);
            if(status!=TalkMessageWidthStatus::Ready)return Block(status==TalkMessageWidthStatus::Unavailable?S::Unavailable:S::InvalidSource);
            stage=2;
        }
        if(flash && subtype!=u'm' && subtype!=u's' && subtype!=u'd' && subtype!=u'f')return Done();
        if(subtype==u'm'){
            if(flash)return Block(S::Unsupported); // Always creates a face in this branch.
            if(stage==2){
                const auto found=state->managers->WindowForWidth(call.process,argument.value.fid);
                if(!found)return Block();
                if(!*found){if(!state->ObserveSkipForMiss())return Block();return Block(S::Unsupported);} // Actual factory remains unowned.
                const auto row=state->windows->Observe(*found);if(!row)return Block();
                if(row->active)return Block(S::Unsupported);
                if(!row->location)return Block();
                if(*row->location!=argument.value.location)return Block(S::Unsupported);
                stage=3; // Already-present inactive same-location shortcut reaches selection.
            }
        }
        if(subtype==u'm' || subtype==u's'){
            if(stage==2 || stage==3){
                if(state->managers->BeginWindowSelection(call.process,access)!=S::Ready)return Block();
                stage=4;
            }
            if(stage==4){
                const auto found=state->managers->WindowForWidth(call.process,argument.value.fid);
                if(!found)return Block();
                auto chosen=*found;
                if(!chosen){
                    if(!state->ObserveSkipForMiss())return Block();
                    const auto context=state->managers->Observe(call.process);
                    if(!context || !context->window_slots[0] || !*context->window_slots[0])return Block();
                    chosen=*context->window_slots[0];
                }
                if(state->managers->SelectCurrentWindow(call.process,chosen,access)!=S::Ready)return Block();
                stage=5;
            }
            return Done();
        }
        if(subtype==u'a'){
            if(stage==2){stage=3;return ProcessCallbackStep::Call(Service(call.process,Activate));}
            return Done();
        }
        if(subtype==u'v'){
            const auto current=Current();if(!current)return Block();
            if(state->windows->ClearNextIcon(*current,access)!=TalkWindowStatus::Ready)return Block();
            return Done();
        }
        if(subtype==u'f'){
            if(!Current())return Block(); // Actual null attached face: no lower flip call.
            return Done();
        }
        if(subtype==u'c'){
            if(stage==2){const auto current=Current();if(!current)return Block();target=*current;return Effect(TalkWindowEffect::StartClose,3);}
            return Done();
        }
        if(subtype==u'd'){
            if(stage==2){
                const auto context=state->managers->Observe(call.process);if(!context || !context->delete_in_skip)return Block();
                const auto current=Current();if(!current)return Block();target=*current;
                if(*context->delete_in_skip){
                    if(state->windows->ClearFaceState(target,true,access)!=TalkWindowStatus::Ready)return Block();
                    return Done();
                }
                const auto row=state->windows->Observe(target);if(!row)return Block();
                if(row->active)return Effect(TalkWindowEffect::StartClose,3);
                if(state->windows->ClearFaceState(target,false,access)!=TalkWindowStatus::Ready)return Block();
            }
            return Done();
        }
        if(subtype==u'o'){
            if(stage==2){
                const auto context=state->managers->Observe(call.process);const auto current=Current();
                if(!context || !context->message_cursor || !current)return Block();
                if(state->widths->Prepare(call.process,*current,*context->message_cursor,0,measurement,&access)!=TalkMessageWidthStatus::Ready)return Block();
                const auto nested=state->widths->Call(measurement);if(!nested)return Block();
                stage=3;return ProcessCallbackStep::Call(*nested);
            }
            if(stage==3){
                request->view.measured_width=access.call_result();
                if(state->widths->Release(measurement,&access)!=TalkMessageWidthStatus::Ready)return Block();
                measurement.reset();stage=4;
            }
            if(stage==4){const auto current=Current();if(!current)return Block();target=*current;return Effect(TalkWindowEffect::StartOpen,5,*request->view.measured_width);}
            return Done();
        }
        return Done(); // Genuine unrecognized subtype returns4; not another specialized handler.
    }
};
NativeTalkWindowControls::NativeTalkWindowControls(std::shared_ptr<State> state):state_(std::move(state)){}
NativeTalkWindowControls::~NativeTalkWindowControls()=default;
S NativeTalkWindowControls::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<NativeTalkControlContext> managers,std::shared_ptr<NativeTalkWindow> windows,
    std::shared_ptr<NativeTalkWindowEffects> effects,std::shared_ptr<NativeTalkMessageWidth> widths,std::shared_ptr<NativeGameSkip> skip,std::shared_ptr<NativeTalkLog> log,
    std::shared_ptr<NativeTalkTokens> tokens,std::shared_ptr<const NativeShiftJis> encoding,std::shared_ptr<NativeTalkWindowControls>& output) {
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !managers || !windows || !effects || !widths || !skip || !log || !tokens || !encoding
       || !managers->UsesScheduler(*scheduler) || !managers->UsesWindows(*windows)
       || !effects->UsesOwners(*windows,*managers) || !widths->UsesOwners(*managers,*windows)
       || !skip->UsesScheduler(*scheduler) || !widths->UsesTokens(*tokens) || !widths->UsesScheduler(*scheduler))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->managers=std::move(managers);state->windows=std::move(windows);
    state->effects=std::move(effects);state->widths=std::move(widths);state->skip=std::move(skip);state->log=std::move(log);state->tokens=std::move(tokens);state->encoding=std::move(encoding);
    auto module=std::shared_ptr<NativeTalkWindowControls>(new NativeTalkWindowControls(state));
    if(!registry->Register(Targets,module))return S::DuplicateBinding;
    output=std::move(module);return S::Ready;
}
S NativeTalkWindowControls::Prepare(ProcessHandle manager,TalkWindowSource source,std::size_t start,TalkCodeOperation op,TalkWindowControlRequest& out,ProcessAccess* access){
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    const auto owner=state_->scheduler.lock();const auto row=owner->Observe(manager);
    if(!row || !row->linked || (row->flags&1u))return S::InvalidParent;
    if(op!=TalkCodeOperation::Dispose && op!=TalkCodeOperation::Flash)return S::Unsupported;
    if(state_->serial==std::numeric_limits<std::uint32_t>::max())return S::IdentityExhausted;
    auto request=std::make_shared<State::Request>();request->view.identity=std::make_shared<TalkWindowControlIdentity>(++state_->serial);
    request->view.manager=std::move(manager);request->source=std::move(source);request->start=start;request->operation=op;
    state_->requests.emplace(request->view.identity->serial,request);out=request->view.identity;return S::Ready;
}
std::optional<ProcessCall> NativeTalkWindowControls::Call(TalkWindowControlRequest handle)const {
    const auto owner=state_->scheduler.lock();const auto row=state_->Get(handle);
    if(!owner || !owner->root(2) || !row || row->view.completed || !owner->Observe(row->view.manager))return {};
    return Service(row->view.manager,row->operation==TalkCodeOperation::Dispose?Dispose:Flash,{handle->serial});
}
std::optional<TalkWindowControlObservation> NativeTalkWindowControls::Observe(TalkWindowControlRequest handle)const {
    const auto owner=state_->scheduler.lock();const auto row=state_->Get(handle);
    return owner && owner->root(2) && row && owner->Observe(row->view.manager)?std::optional{row->view}:std::nullopt;
}
S NativeTalkWindowControls::Release(TalkWindowControlRequest handle,ProcessAccess* access){
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    if(!state_->Get(handle))return S::InvalidHandle;
    state_->requests.erase(handle->serial);return S::Ready;
}
void NativeTalkWindowControls::Forget(TalkWindowControlRequest handle)noexcept{if(state_->Get(handle))state_->requests.erase(handle->serial);}
bool NativeTalkWindowControls::UsesOwners(const NativeTalkControlContext& managers,const NativeTalkWindow& windows,const NativeTalkTokens& tokens,const NativeTalkLog& log,const NativeGameSkip& skip)const noexcept {
    return state_->managers.get()==&managers && state_->windows.get()==&windows && state_->tokens.get()==&tokens && state_->log.get()==&log && state_->skip.get()==&skip;
}
std::unique_ptr<ProcessContinuation> NativeTalkWindowControls::Begin(const ProcessCall& call){
    const auto owner=state_->scheduler.lock();if(!owner || !owner->root(2) || !call.process || call.kind!=ProcessCallKind::Service || call.has_self || call.this_adjustment)return {};
    const auto parent=owner->Observe(call.process);if(!parent || !parent->linked || (parent->flags&1u))return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->call=call;
    if(call.target==SetActive){
        if(call.argument_count!=2 || call.arguments[1]>1)return {};
        next->target=state_->BySerial(call.arguments[0]);if(!next->target)return {};
    }else if(call.target==Activate){if(call.argument_count)return {};}
    else if(call.target==Dispose || call.target==Flash){
        if(call.argument_count!=1)return {};
        const auto it=state_->requests.find(call.arguments[0]);if(it==state_->requests.end() || it->second->view.manager!=call.process || it->second->view.completed)return {};
        if(call.target!=(it->second->operation==TalkCodeOperation::Dispose?Dispose:Flash))return {};
        next->request=it->second;
    }else return {};
    return next;
}
}
