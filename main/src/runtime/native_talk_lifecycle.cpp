#include "fates/runtime/native_talk_lifecycle.hpp"
#include <algorithm>
#include <limits>
#include <map>

namespace fates::runtime::native {
using namespace presentation::native;
using S=TalkControlStatus;
namespace {
constexpr std::array<std::uint32_t,9> Operations{0x1e58cc,0x1e4e20,0x1e58ec,0x1e5d04,0x1e62b0,0x1e5da4,0x1e5d9c,0x1e5ff0,0x1e62d8};
constexpr std::uint32_t BindDestroy=0x4f2ff8;
constexpr std::array<std::uint32_t,10> Targets{0x1e58cc,0x1e4e20,0x1e58ec,0x1e5d04,0x1e62b0,0x1e5da4,0x1e5d9c,0x1e5ff0,0x1e62d8,BindDestroy};
ProcessCall Service(ProcessHandle manager,std::uint32_t target){
    ProcessCall result;result.process=std::move(manager);result.kind=ProcessCallKind::Service;result.target=target;return result;
}
ProcessType BinderType(){auto value=ProcessType::Base();value.methods[0].target=BindDestroy;return value;}
}
struct NativeTalkLifecycle::State {
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<NativeTalkControlContext> managers;std::shared_ptr<NativeTalkWindow> windows;
    std::shared_ptr<NativeTalkWindowEffects> effects;std::shared_ptr<NativeTalkWindowDrawer> drawer;
    std::shared_ptr<NativeTalkSpeaker> speaker;std::shared_ptr<NativeTalkControlEffects> controls;
    std::shared_ptr<NativeTalkLog> log;std::shared_ptr<NativeTalkTokens> tokens;
    std::shared_ptr<NativeGameSkip> skip;std::shared_ptr<NativeFadeSystem> fade;std::shared_ptr<NativeTalkModeFade> mode;
    std::map<std::uint64_t,ProcessHandle> binders;
    S Bind(ProcessAccess& access,ProcessHandle parent,ProcessHandle& created) {
        const auto owner=scheduler.lock();if(!owner || !owner->root(2))return S::Retired;
        if(!access.BelongsTo(*owner))return S::MismatchedDomain;
        if(access.Create(std::move(parent),ProcessProgram::Default(),"ProcBind",true,BinderType(),created)!=ProcessStatus::Ready)return S::InvalidParent;
        binders.emplace(created->serial,created);return S::Ready;
    }
};
struct NativeTalkLifecycle::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;ProcessCall call;TalkLifecycleOperation operation{};
    unsigned stage{},index{};TalkWindowSource source;TalkLogSource talker;TalkControlRequest control;
    ProcessHandle created;TalkWindowHandle window;bool skipped{};std::uint8_t ending{};
    ~Continuation() override {if(control)state->controls->Forget(control);}
    ProcessCallbackStep Block()const{return ProcessCallbackStep::Blocked();}
    std::optional<TalkControlManagerView> Manager()const{return state->managers->Observe(call.process);}
    bool Write(ProcessAccess& access,const TalkLifecycleUpdate& value){return state->managers->WriteLifecycleState(call.process,value,access)==S::Ready;}
    bool Character(ProcessAccess& access,const TalkCharacterUpdate& value){return state->managers->WriteCharacterState(call.process,value,access)==S::Ready;}
    bool Cursor(){const auto row=Manager();if(!row || !row->message_cursor)return false;source=*row->message_cursor;return true;}
    std::optional<char16_t> Read(std::size_t offset=0)const{
        const auto word=state->windows->Read(source,offset);
        return word.status==TalkWindowStatus::Ready?std::optional{word.value}:std::nullopt;
    }
    bool Advance(ProcessAccess& access,std::size_t words){
        if(words>std::numeric_limits<std::size_t>::max()-source.offset)return false;
        auto value=source;value.offset+=words;TalkCharacterUpdate change;change.cursor=std::move(value);return Character(access,change);
    }
    bool GlobalSkip(){
        const auto current=state->skip->Current();if(!current)return false;
        if(!*current){skipped=false;return true;}
        const auto row=state->skip->Observe(*current);if(!row)return false;
        skipped=row->state!=0;return true;
    }
    ProcessCallbackStep Faces(unsigned next){
        if(index>=3){stage=next;return ProcessCallbackStep::Continue();}
        if(!GlobalSkip())return Block();
        const auto row=Manager();if(!row || !row->window_slots[index])return Block();
        const auto child=state->effects->Call(call.process,*row->window_slots[index],
            skipped?TalkWindowEffect::FadeOutFaceInSkip:TalkWindowEffect::FadeOutFace);
        if(!child)return Block();
        ++index;return ProcessCallbackStep::Call(*child);
    }
    ProcessCallbackStep Load(ProcessAccess& access){
        if(stage==0){TalkLifecycleUpdate change;change.render=std::uint8_t{1};if(!Write(access,change))return Block();
            stage=1;return ProcessCallbackStep::Call(NativeTalkWindowDrawer::InitializeCall(call.process,true));}
        TalkLifecycleUpdate change;change.drawer=std::uint8_t{1};if(!Write(access,change))return Block();
        return ProcessCallbackStep::Return();
    }
    ProcessCallbackStep WaitLoad(ProcessAccess& access){
        if(stage==0){stage=1;return ProcessCallbackStep::Call(NativeTalkWindowDrawer::TickLoadAsyncCall(call.process));}
        const auto ready=state->drawer->FilesReady();if(!ready)return Block();
        if(*ready && access.Next(call.process)!=ProcessStatus::Ready)return Block();
        return ProcessCallbackStep::Return();
    }
    ProcessCallbackStep Resume(ProcessAccess& access){
        if(stage==0){
            const auto row=Manager();if(!row || !row->auxiliary)return Block();
            if(*row->auxiliary)stage=2;
            else {
                const auto manager=access.Observe(call.process);if(!manager || !manager->parent)return Block();
                if(state->Bind(access,manager->parent,created)!=S::Ready)return Block();
                stage=1;
            }
        }
        if(stage==1){TalkLifecycleUpdate change;change.auxiliary=created;if(!Write(access,change))return Block();stage=2;}
        if(access.Jump(call.process,2)!=ProcessStatus::Ready)return Block();
        return ProcessCallbackStep::Return();
    }
    ProcessCallbackStep Skip(ProcessAccess& access){
        for(;;){
            switch(stage){
            case 0:{TalkLifecycleUpdate change;change.delete_in_skip=std::uint8_t{1};if(!Write(access,change))return Block();stage=1;break;}
            case 1:case 16:{
                if(!Cursor())return Block();
                const auto word=Read();if(!word)return Block();
                stage=*word?2u:20u;return ProcessCallbackStep::Continue();}
            case 2:{
                if(!Cursor())return Block();
                const auto word=Read();if(!word)return Block();
                stage=!*word?15u:*word==u'\n'?13u:*word==u'$'?3u:7u;break;}
            case 3:{
                const auto word=Read(1);if(!word)return Block();
                if(*word==u'$'){if(!Advance(access,2))return Block();stage=16;break;}
                stage=4;break;}
            case 4:{
                if(!control && state->controls->Prepare(call.process,source,0,TalkCodeOperation::Flash,control,&access)!=S::Ready)return Block();
                const auto child=state->controls->Call(control);if(!child)return Block();stage=5;return ProcessCallbackStep::Call(*child);}
            case 5:{
                const auto row=state->controls->Observe(control);if(!row || !row->completed)return Block();
                if(state->controls->Release(control,&access)!=S::Ready)return Block();
                control.reset();stage=6;break;}
            case 6:{
                if(!Cursor())return Block();
                NativeTalkControlScanner scanner(*state->tokens);
                const auto next=scanner.Skip([this](std::size_t at){return Read(at);},0);
                if(next.status!=TalkCodeStatus::Ready)return Block();
                if(next.next){if(!Advance(access,*next.next))return Block();}
                else {TalkCharacterUpdate change;change.cursor=TalkWindowSource{};if(!Character(access,change))return Block();}
                stage=16;break;}
            case 7:{
                const auto row=Manager();if(!row || !row->selection_pending)return Block();
                if(*row->selection_pending){TalkCharacterUpdate change;change.selection=std::uint8_t{0};if(!Character(access,change))return Block();stage=8;}
                else stage=11;
                break;}
            case 8:if(state->log->SetTalker({})!=TalkLogStatus::Ready)return Block();stage=9;break;
            case 9:{
                const auto row=Manager();if(!row || !row->window || !*row->window)return Block();
                const auto name=state->speaker->Get(*row->window,&access);if(name.status!=TalkSpeakerStatus::Ready)return Block();
                talker={};if(name.source.kind!=TalkWindowSource::Kind::Null){talker.kind=TalkLogSource::Kind::Reader;
                    const auto windows=state->windows;const auto text=name.source;
                    talker.reader=[windows,text](std::size_t at)->std::optional<char16_t>{const auto r=windows->Read(text,at);return r.status==TalkWindowStatus::Ready?std::optional{r.value}:std::nullopt;};}
                stage=10;break;}
            case 10:if(state->log->SetTalker(talker)!=TalkLogStatus::Ready)return Block();stage=11;break;
            case 11:{if(!Cursor())return Block();
                const auto word=Read();if(!word)return Block();
                if(state->log->Append(*word)!=TalkLogStatus::Ready)return Block();
                stage=12;break;}
            case 12:if(!Cursor() || !Advance(access,1))return Block();stage=16;break;
            case 13:if(state->log->NextLine()!=TalkLogStatus::Ready)return Block();stage=14;break;
            case 14:if(!Cursor() || !Advance(access,1))return Block();stage=16;break;
            case 15:if(state->log->NextLine()!=TalkLogStatus::Ready)return Block();stage=16;break;
            case 20:if(state->log->NextLine()!=TalkLogStatus::Ready)return Block();stage=21;break;
            case 21:if(state->log->NextLine()!=TalkLogStatus::Ready)return Block();stage=22;break;
            case 22:return Faces(23);
            default:return ProcessCallbackStep::Return();
            }
        }
    }
    ProcessCallbackStep End(ProcessAccess& access){
        if(stage==0){const auto row=Manager();if(!row || !row->suppress_face_fade)return Block();
            if(*row->suppress_face_fade){TalkLifecycleUpdate change;change.ending_fade=std::uint8_t{2};if(!Write(access,change))return Block();stage=2;}
            else stage=1;}
        if(stage==1)return Faces(2);
        if(stage==2){const auto row=Manager();if(!row || !row->ending_fade)return Block();ending=*row->ending_fade;
            if(ending!=1 && ending!=2)return ProcessCallbackStep::Return();
            if(operation!=TalkLifecycleOperation::EndFadeForce && !call.arguments[0]){
                const auto channel=state->fade->ObserveChannel(0);if(!channel)return Block();
                if(channel->color[3])return ProcessCallbackStep::Return();}
            stage=3;return ProcessCallbackStep::Call(ending==1?NativeTalkModeFade::BlackOutCall(call.process,500):NativeTalkModeFade::WhiteOutCall(call.process,500));}
        return ProcessCallbackStep::Return();
    }
    ProcessCallbackStep Release(ProcessAccess& access){
        if(stage==0){auto child=Service(call.process,0x41f6c4);child.arguments[0]=100;child.argument_count=1;
            stage=1;return ProcessCallbackStep::Call(child);}
        if(stage==1){TalkLifecycleUpdate change;change.render=std::uint8_t{0};if(!Write(access,change))return Block();stage=2;}
        if(stage==2){
            for(;index<3;++index){const auto row=Manager();if(!row || !row->window_slots[index])return Block();
                if(state->windows->Reset(*row->window_slots[index],&access)!=TalkWindowStatus::Ready)return Block();}
            stage=3;return ProcessCallbackStep::Call(Service(call.process,0x1964dc));}
        if(stage==3){const auto row=Manager();if(!row || !row->drawer_acquired)return Block();stage=4;
            if(*row->drawer_acquired)return ProcessCallbackStep::Call(NativeTalkWindowDrawer::FinalizeCall(call.process));}
        if(stage==4){TalkLifecycleUpdate change;change.drawer=std::uint8_t{0};if(!Write(access,change))return Block();stage=5;}
        if(stage==5){if(!GlobalSkip())return Block();if(skipped)return ProcessCallbackStep::Return();stage=6;}
        if(stage==6){const auto row=Manager();if(!row || !row->ending_fade)return Block();if(*row->ending_fade)return ProcessCallbackStep::Return();stage=7;}
        if(stage==7){const auto channel=state->fade->ObserveChannel(0);if(!channel)return Block();if(!channel->color[3])return ProcessCallbackStep::Return();
            stage=8;return ProcessCallbackStep::Call(NativeTalkModeFade::FadeInCall(call.process,500));}
        if(state->managers->WriteRevealCounter(call.process,0,access)!=S::Ready)return Block();
        return ProcessCallbackStep::Return();
    }
    ProcessCallbackStep Step(ProcessAccess& access) override {
        const auto owner=state->scheduler.lock();if(!owner || !access.BelongsTo(*owner))return Block();
        if(call.target==BindDestroy){state->binders.erase(call.process->serial);return ProcessCallbackStep::Return();}
        if(!Manager())return Block();
        switch(operation){
        case TalkLifecycleOperation::Load:return Load(access);
        case TalkLifecycleOperation::WaitLoadAsync:return WaitLoad(access);
        case TalkLifecycleOperation::Skip:return Skip(access);
        case TalkLifecycleOperation::Resume:return Resume(access);
        case TalkLifecycleOperation::StartFade:{const auto row=state->fade->ObserveChannel(0);if(!row)return Block();
            if(state->managers->WriteRevealCounter(call.process,row->blackout?30u:0u,access)!=S::Ready)return Block();
            return ProcessCallbackStep::Return();}
        case TalkLifecycleOperation::EndFade:case TalkLifecycleOperation::EndFadeForce:return End(access);
        case TalkLifecycleOperation::Release:return Release(access);
        case TalkLifecycleOperation::StartSkip:
            if(access.Jump(call.process,5)!=ProcessStatus::Ready)return Block();
            return ProcessCallbackStep::Return();
        }
        return Block();
    }
};
NativeTalkLifecycle::NativeTalkLifecycle(std::shared_ptr<State> state):state_(std::move(state)){}
NativeTalkLifecycle::~NativeTalkLifecycle()=default;
TalkControlStatus NativeTalkLifecycle::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<NativeTalkControlContext> managers,std::shared_ptr<NativeTalkWindow> windows,std::shared_ptr<NativeTalkWindowEffects> effects,
    std::shared_ptr<NativeTalkWindowDrawer> drawer,std::shared_ptr<NativeTalkSpeaker> speaker,std::shared_ptr<NativeTalkControlEffects> controls,
    std::shared_ptr<NativeTalkLog> log,std::shared_ptr<NativeTalkTokens> tokens,std::shared_ptr<NativeGameSkip> skip,
    std::shared_ptr<NativeFadeSystem> fade,std::shared_ptr<NativeTalkModeFade> mode,std::shared_ptr<NativeTalkLifecycle>& output){
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !managers || !managers->UsesScheduler(*scheduler)
        || !windows || !windows->UsesScheduler(*scheduler) || !managers->UsesWindows(*windows)
        || !effects || !effects->UsesOwners(*windows,*managers) || !drawer || !drawer->UsesScheduler(*scheduler)
        || !speaker || !speaker->UsesWindows(*windows) || !controls || !log || !tokens || !skip || !fade || !mode
        || !skip->UsesScheduler(*scheduler) || !skip->UsesFadeSystem(*fade) || !mode->UsesOwners(*managers,*windows,*skip)
        || !controls->UsesOwners(*managers,*windows,*log,*skip) || !controls->UsesTokens(*tokens))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->managers=std::move(managers);state->windows=std::move(windows);
    state->effects=std::move(effects);state->drawer=std::move(drawer);state->speaker=std::move(speaker);state->controls=std::move(controls);
    state->log=std::move(log);state->tokens=std::move(tokens);state->skip=std::move(skip);state->fade=std::move(fade);state->mode=std::move(mode);
    auto value=std::shared_ptr<NativeTalkLifecycle>(new NativeTalkLifecycle(state));if(!registry->Register(Targets,value))return S::DuplicateBinding;
    output=std::move(value);return S::Ready;
}
std::optional<ProcessCall> NativeTalkLifecycle::Call(ProcessHandle manager,TalkLifecycleOperation operation,bool force){
    const auto index=static_cast<std::size_t>(operation);if(index>=Operations.size())return {};
    auto call=Service(std::move(manager),Operations[index]);if(operation==TalkLifecycleOperation::EndFade){call.arguments[0]=force?1u:0u;call.argument_count=1;}return call;
}
std::vector<ProcessHandle> NativeTalkLifecycle::AuxiliaryProcesses()const {
    std::vector<ProcessHandle> result;const auto scheduler=state_->scheduler.lock();if(!scheduler || !scheduler->root(2))return result;
    for(const auto& [id,handle]:state_->binders){(void)id;if(scheduler->Observe(handle))result.push_back(handle);}return result;
}
bool NativeTalkLifecycle::UsesScheduler(const NativeProcessScheduler& owner)const noexcept{return state_->scheduler.lock().get()==&owner;}
std::unique_ptr<ProcessContinuation> NativeTalkLifecycle::Begin(const ProcessCall& call){
    const auto scheduler=state_->scheduler.lock();if(!scheduler || !scheduler->root(2) || call.this_adjustment)return {};
    const auto process=scheduler->Observe(call.process);if(!process)return {};
    auto result=std::make_unique<Continuation>();result->state=state_;result->call=call;
    if(call.target==BindDestroy){const auto found=state_->binders.find(call.process->serial);
        if(call.kind!=ProcessCallKind::Destroy || !call.has_self || call.argument_count || found==state_->binders.end() || found->second!=call.process
            || !scheduler->HasType(call.process,BinderType()))return {};
        return result;}
    const auto at=std::find(Operations.begin(),Operations.end(),call.target);if(!process->linked || at==Operations.end() || !state_->managers->Observe(call.process) || (process->flags&1u))return {};
    result->operation=static_cast<TalkLifecycleOperation>(at-Operations.begin());
    const bool service=call.kind==ProcessCallKind::Service && !call.has_self;
    // The canonical Talk program uses command 11 for these immediate callbacks.
    // The retained supplied-program interfaces (12/13) remain unchanged.
    const bool canonical_immediate=call.command==11 && (result->operation==TalkLifecycleOperation::Load
        || result->operation==TalkLifecycleOperation::StartFade || result->operation==TalkLifecycleOperation::Skip
        || result->operation==TalkLifecycleOperation::EndFadeForce || result->operation==TalkLifecycleOperation::Release);
    const bool descriptor=call.kind==ProcessCallKind::Descriptor && call.has_self
        && (call.command==12 || call.command==13 || canonical_immediate);
    if(!service && !descriptor)return {};
    if(call.argument_count!=(result->operation==TalkLifecycleOperation::EndFade?1u:0u))return {};
    return result;
}
bool NativeTalkLifecycle::UsesOwners(const NativeTalkControlContext& context,const NativeTalkWindow& windows,
    const NativeTalkLog& log,const NativeTalkTokens& tokens,const NativeGameSkip& skip) const noexcept {
    return state_->managers.get()==&context && state_->windows.get()==&windows && state_->log.get()==&log
        && state_->tokens.get()==&tokens && state_->skip.get()==&skip;
}

TalkControlStatus NativeTalkLifecycle::BindAuxiliary(ProcessAccess& access,ProcessHandle parent,ProcessHandle& out) {
    return state_->Bind(access,std::move(parent),out);
}
bool NativeTalkLifecycle::UsesContext(const NativeTalkControlContext& context)const noexcept {
    return state_->managers.get()==&context;
}

}
