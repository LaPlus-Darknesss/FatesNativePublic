#include "fates/runtime/native_talk_initialize.hpp"
#include <bit>
#include <limits>
#include <map>
namespace fates::runtime::native {
using namespace presentation::native;
using S=TalkControlStatus;
namespace {
constexpr std::uint32_t Initialize=0x1e4830,Direct=0x1e5474,ShadowIn=0x1e4c04,ShadowOut=0x1e4dc0;
constexpr std::array Targets{Initialize,Direct,ShadowIn,ShadowOut};
ProcessCall Service(ProcessHandle manager,std::uint32_t target){ProcessCall call;call.process=std::move(manager);call.kind=ProcessCallKind::Service;call.target=target;return call;}
}
struct NativeTalkInitialize::State {
    struct Binding {ProcessHandle manager;std::shared_ptr<NativeTalkText> text;};
    struct Request {TalkInitializeObservation value;TalkWindowSource source;std::string identifier;std::shared_ptr<NativeTalkText> text;};
    std::weak_ptr<NativeProcessScheduler> scheduler;std::shared_ptr<NativeTalkControlContext> managers;
    std::shared_ptr<NativeTalkWindow> windows;std::shared_ptr<NativeTalkLog> log;std::shared_ptr<NativeTalkTokens> tokens;
    std::shared_ptr<NativeGameSkip> skip;std::shared_ptr<NativeTalkLifecycle> lifecycle;
    std::map<std::uint64_t,Binding> bindings;std::map<std::uint32_t,std::shared_ptr<Request>> requests;std::uint32_t serial{};
    S Mutable(ProcessAccess* access)const {
        const auto owner=scheduler.lock();if(!owner || !owner->root(2))return S::Retired;
        if(access)return access->BelongsTo(*owner)?S::Ready:S::MismatchedDomain;
        return owner->busy()?S::Busy:S::Ready;
    }
    std::shared_ptr<NativeTalkText> Text(ProcessHandle manager)const {
        const auto owner=scheduler.lock();const auto row=owner?owner->Observe(manager):std::nullopt;
        if(!row || !row->linked || (row->flags&1u))return {};
        const auto it=bindings.find(manager->serial);return it!=bindings.end() && it->second.manager==manager?it->second.text:nullptr;
    }
    std::shared_ptr<Request> Get(TalkInitializeRequest id)const {
        if(!id)return {};
        const auto it=requests.find(id->serial);return it!=requests.end() && it->second->value.identity==id?it->second:nullptr;
    }
};
struct NativeTalkInitialize::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;std::shared_ptr<State::Request> request;ProcessCall call;
    unsigned stage{},index{};bool shadow_advance{};TalkWindowSource source;TalkWindowHandle window;MessageLookupResult selected;
    std::optional<TalkControlManagerView> Manager()const{return state->managers->Observe(call.process);}
    ProcessCallbackStep Block(S why=S::Unavailable){if(request)request->value.status=why;return ProcessCallbackStep::Blocked();}
    std::optional<bool> Skipping()const {
        const auto current=state->skip->Current();if(!current)return {};if(!*current)return false;
        const auto row=state->skip->Observe(*current);if(!row)return {};return row->state!=0;
    }
    ProcessCallbackStep Shadow(ProcessAccess& access) {
        if(stage==0){
        const auto row=Manager();if(!row || !row->free_window_mode)return Block();
        // Conditional reads stay conditional: disabled/non-one mode does not
        // demand a known counter, ending byte, or shadow-enable byte.
        bool advance=*row->free_window_mode!=1;
        if(!advance){if(!row->shadow_enabled)return Block();advance=*row->shadow_enabled==0;}
        if(!advance && call.target==ShadowOut){if(!row->ending_fade)return Block();advance=*row->ending_fade!=0;}
        if(!advance){
            if(!row->reveal_counter)return Block();
            const auto value=call.target==ShadowIn?*row->reveal_counter+access.frame_delta():*row->reveal_counter-access.frame_delta();
            const bool finished=call.target==ShadowIn?std::bit_cast<std::int32_t>(value)>=30:(value&0x80000000u)!=0;
            const auto stored=finished?(call.target==ShadowIn?30u:0u):value;
            if(state->managers->WriteRevealCounter(call.process,stored,access)!=S::Ready)return Block();
            advance=finished;
        }
        shadow_advance=advance;stage=1;
        }
        if(shadow_advance && access.Next(call.process)!=ProcessStatus::Ready)return Block();
        return ProcessCallbackStep::Return();
    }
    ProcessCallbackStep Step(ProcessAccess& access) override {
        if(state->Mutable(&access)!=S::Ready)return Block(S::Retired);
        const auto owner=state->scheduler.lock();const auto process=access.Observe(call.process);
        if(!owner || !process || !process->linked || (process->flags&1u) || !Manager())return Block(S::InvalidParent);
        if(!request)return Shadow(access);
        if(state->Text(call.process)!=request->text)return Block(S::Retired);
        request->value.status=S::Ready;
        for(;;){
            if(stage==0){
                if(request->value.message_identifier){
                    const auto selection=request->text->SelectMessage(request->identifier);
                    request->value.text_status=selection.status;
                    if(selection.status!=TalkTextStatus::Ready)return Block();
                    request->value.selected_identifier=selection.identifier;
                    selected=selection.source;stage=3;
                } else {
                    source=request->source;TalkTextStatus status;
                    if(source.kind==TalkWindowSource::Kind::Null)status=request->text->ExpandWords(std::nullopt);
                    else if(source.kind==TalkWindowSource::Kind::Expanded && source.expanded.get()==request->text.get())status=request->text->ExpandCurrent(source.offset);
                    else status=request->text->ExpandReader([this](std::size_t at)->std::optional<char16_t>{
                        const auto word=state->windows->Read(source,at);return word.status==TalkWindowStatus::Ready?std::optional{word.value}:std::nullopt;});
                    request->value.text_status=status;if(status!=TalkTextStatus::Ready)return Block();
                }
                if(stage==0)stage=1;
            }
            if(stage==3){
                const auto status=request->text->ExpandMessage(selected);
                request->value.text_status=status;
                if(status!=TalkTextStatus::Ready)return Block();
                stage=1;
            }
            if(stage==1){if(state->managers->CommitInitializedText(call.process,request->text,access)!=S::Ready)return Block();stage=2;}
            if(stage==2){const auto row=Manager();if(!row || !row->auxiliary)return Block();
                if(*row->auxiliary)stage=4;else {stage=12;const auto nested=NativeTalkLifecycle::Call(call.process,TalkLifecycleOperation::Resume);if(!nested)return Block();return ProcessCallbackStep::Call(*nested);}}
            if(stage==4){TalkCharacterUpdate change;change.selection=std::uint8_t{0};if(state->managers->WriteCharacterState(call.process,change,access)!=S::Ready)return Block();stage=5;}
            if(stage==5){if(state->managers->WriteTalkMode(call.process,std::uint8_t{2},access)!=S::Ready)return Block();stage=6;}
            if(stage==6){const auto row=Manager();if(!row || !row->window_slots[0])return Block();if(state->managers->SelectCurrentWindow(call.process,*row->window_slots[0],access)!=S::Ready)return Block();stage=7;}
            if(stage==7){if(state->log->InitializeEveryTalk()!=TalkLogStatus::Ready)return Block();index=0;stage=8;}
            if(stage==8){
                if(index==3){stage=12;continue;}
                const auto row=Manager();if(!row || !row->window_slots[index])return Block();window=*row->window_slots[index];
                if(state->windows->Reset(window,&access)!=TalkWindowStatus::Ready)return Block();
                stage=9;
            }
            if(stage==9){const auto skipping=Skipping();if(!skipping)return Block();const auto row=Manager();if(!row || !row->free_window_mode)return Block();
                const auto type=static_cast<std::int32_t>(std::bit_cast<std::int8_t>(*row->free_window_mode));
                if(state->windows->InitializeWithoutFace(window,type,4,*skipping,access)!=TalkWindowStatus::Ready)return Block();
                ++index;stage=8;continue;
            }
            if(stage==12){const auto skipping=Skipping();if(!skipping)return Block();if(*skipping && access.Jump(call.process,5)!=ProcessStatus::Ready)return Block();
                request->value.completed=true;return ProcessCallbackStep::Return();}
        }
    }
};
NativeTalkInitialize::NativeTalkInitialize(std::shared_ptr<State> state):state_(std::move(state)){}
NativeTalkInitialize::~NativeTalkInitialize()=default;
S NativeTalkInitialize::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<NativeTalkControlContext> managers,std::shared_ptr<NativeTalkWindow> windows,std::shared_ptr<NativeTalkLog> log,
    std::shared_ptr<NativeTalkTokens> tokens,std::shared_ptr<NativeGameSkip> skip,std::shared_ptr<NativeTalkLifecycle> lifecycle,
    std::shared_ptr<NativeTalkInitialize>& out){
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !managers || !windows || !log || !tokens || !skip || !lifecycle ||
       !managers->UsesScheduler(*scheduler) || !managers->UsesWindows(*windows) || !windows->UsesScheduler(*scheduler) ||
       !skip->UsesScheduler(*scheduler) || !lifecycle->UsesOwners(*managers,*windows,*log,*tokens,*skip))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->managers=std::move(managers);state->windows=std::move(windows);state->log=std::move(log);
    state->tokens=std::move(tokens);state->skip=std::move(skip);state->lifecycle=std::move(lifecycle);
    auto next=std::shared_ptr<NativeTalkInitialize>(new NativeTalkInitialize(state));
    if(!registry->Register(Targets,next))return S::DuplicateBinding;
    out=std::move(next);return S::Ready;
}
S NativeTalkInitialize::AttachExpander(ProcessHandle manager,std::shared_ptr<NativeTalkText> text,ProcessAccess* access){
    if(auto status=state_->Mutable(access);status!=S::Ready)return status;
    const auto owner=state_->scheduler.lock();const auto row=owner->Observe(manager);
    if(!row || (!row->linked && !(access && row->constructing)) || (row->flags&1u) || !state_->managers->Observe(manager))return S::InvalidParent;
    if(!text || !text->UsesTokens(*state_->tokens))return S::MismatchedDomain;
    if(state_->bindings.contains(manager->serial))return S::DuplicateBinding;
    for(const auto& [id,binding]:state_->bindings){(void)id;if(binding.text==text && owner->Observe(binding.manager))return S::DuplicateBinding;}
    state_->bindings[manager->serial]={manager,std::move(text)};return S::Ready;
}
S NativeTalkInitialize::DetachExpander(ProcessHandle manager,ProcessAccess* access){
    if(auto status=state_->Mutable(access);status!=S::Ready)return status;
    if(!manager)return S::InvalidParent;
    const auto it=state_->bindings.find(manager->serial);
    if(it==state_->bindings.end() || it->second.manager!=manager)return S::InvalidParent;
    state_->bindings.erase(it);return S::Ready;
}
S NativeTalkInitialize::PrepareDirect(ProcessHandle manager,TalkWindowSource source,TalkInitializeRequest& out,ProcessAccess* access){
    if(auto status=state_->Mutable(access);status!=S::Ready)return status;
    auto text=state_->Text(manager);if(!text)return S::InvalidParent;
    if(state_->serial==std::numeric_limits<std::uint32_t>::max())return S::IdentityExhausted;
    auto row=std::make_shared<State::Request>();row->value.identity=std::make_shared<const TalkInitializeIdentity>(TalkInitializeIdentity{++state_->serial});
    row->value.manager=manager;row->source=std::move(source);row->text=std::move(text);out=row->value.identity;state_->requests.emplace(out->serial,std::move(row));return S::Ready;
}
S NativeTalkInitialize::PrepareIdentifier(ProcessHandle manager,std::string_view identifier,TalkInitializeRequest& out,ProcessAccess* access){
    if(identifier.size()>65535 || identifier.find('\0')!=std::string_view::npos)return S::InvalidSource;
    TalkInitializeRequest next;const auto status=PrepareDirect(manager,{},next,access);if(status!=S::Ready)return status;
    auto row=state_->Get(next);row->identifier=identifier;row->value.message_identifier=true;out=std::move(next);return S::Ready;
}
std::optional<ProcessCall> NativeTalkInitialize::Call(TalkInitializeRequest id)const {
    const auto row=state_->Get(id);if(!row || row->value.completed || state_->Text(row->value.manager)!=row->text)return {};
    auto call=Service(row->value.manager,row->value.message_identifier?Initialize:Direct);call.arguments[0]=id->serial;call.argument_count=1;return call;
}
std::optional<TalkInitializeObservation> NativeTalkInitialize::Observe(TalkInitializeRequest id)const {const auto row=state_->Get(id);return row?std::optional{row->value}:std::nullopt;}
S NativeTalkInitialize::Release(TalkInitializeRequest id,ProcessAccess* access){if(auto status=state_->Mutable(access);status!=S::Ready)return status;if(!state_->Get(id))return S::InvalidHandle;state_->requests.erase(id->serial);return S::Ready;}
void NativeTalkInitialize::Forget(TalkInitializeRequest id)noexcept{if(state_->Get(id))state_->requests.erase(id->serial);}
ProcessCall NativeTalkInitialize::ShadowInCall(ProcessHandle h){return Service(std::move(h),ShadowIn);}
ProcessCall NativeTalkInitialize::ShadowOutCall(ProcessHandle h){return Service(std::move(h),ShadowOut);}
ProcessType NativeTalkInitialize::ManagerType(){return {{{0,4,0x1e64e0},{0,8,0x1e5a88},{0,12,0x1e4938},{0,16,0x4ee170},{0,20,ShadowIn},{0,24,ShadowOut}}};}
bool NativeTalkInitialize::UsesScheduler(const NativeProcessScheduler& scheduler)const noexcept{return state_->scheduler.lock().get()==&scheduler;}
std::unique_ptr<ProcessContinuation> NativeTalkInitialize::Begin(const ProcessCall& call){
    if(call.this_adjustment || !state_->managers->Observe(call.process))return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->call=call;
    if(call.target==ShadowIn || call.target==ShadowOut){if(call.argument_count || (call.kind==ProcessCallKind::Service?call.has_self:
            (call.kind!=ProcessCallKind::Descriptor || !call.has_self || call.command!=13)))return {};}
    else if(call.target==Initialize || call.target==Direct){
        if(call.kind!=ProcessCallKind::Service || call.has_self || call.argument_count!=1)return {};
        const auto it=state_->requests.find(call.arguments[0]);if(it==state_->requests.end())return {};
        next->request=it->second;
        if(next->request->value.manager!=call.process || next->request->value.completed || (call.target==Initialize)!=next->request->value.message_identifier)return {};
    }else return {};
    return next;
}
}

namespace fates::runtime::native {
bool NativeTalkInitialize::UsesOwners(const NativeTalkControlContext& managers,const presentation::native::NativeTalkWindow& windows,const NativeTalkTokens& tokens)const noexcept {
    return state_->managers.get()==&managers && state_->windows.get()==&windows && state_->tokens.get()==&tokens;
}
}
