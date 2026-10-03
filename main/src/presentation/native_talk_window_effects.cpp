#include "fates/presentation/native_talk_window_effects.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

namespace fates::presentation::native {
using namespace runtime::native;
using WS=TalkWindowStatus;
using ES=TalkEffectStatus;
namespace {
constexpr std::array<std::uint32_t,10> Targets{0x18fed0,0x18eb88,0x18ef58,0x18e898,0x18f2f4,0x1cb614,0x1cb720,0x18e6e0,0x18ea24,0x18f394};
ProcessCall Service(ProcessHandle parent,std::uint32_t target,std::initializer_list<std::uint32_t> args) {
    ProcessCall call;call.process=std::move(parent);call.kind=ProcessCallKind::Service;call.target=target;
    call.argument_count=static_cast<std::uint8_t>(args.size());std::copy(args.begin(),args.end(),call.arguments.begin());return call;
}
std::uint32_t Bits(float value){return std::bit_cast<std::uint32_t>(value);}
float Add(float a,float b){volatile float result=a+b;return result;}
}
struct NativeTalkWindowEffects::State {
    std::weak_ptr<NativeProcessScheduler> scheduler;std::shared_ptr<NativeTalkWindow> windows;
    std::shared_ptr<NativeTalkMotion> motion;std::shared_ptr<NativeTalkColorFader> faders;
    std::shared_ptr<NativeTalkLayout> layout;std::shared_ptr<TalkEffectManagerState> manager;
    TalkEffectStaticColors colors;
    bool Live() const {auto owner=scheduler.lock();return owner && owner->root(2);}
};
struct NativeTalkWindowEffects::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;ProcessCall call;TalkWindowHandle window;TalkWindowEffect effect{};
    unsigned step{},index{};std::int32_t duration{};std::uint32_t distance{},side{};TalkVectorBits position{};
    bool Carrier(ProcessAccess& access,TalkMemberIdentity member,TalkVectorBits target,std::int32_t ms,bool blocking,std::uint32_t curve=2) {
        ProcessHandle child;return state->motion->BindCarrier(call.process,state->windows,member.handle,target,ms,curve,blocking,child,&access)==TalkMotionStatus::Ready;
    }
    bool Fade(ProcessAccess& access,TalkMemberIdentity member,TalkColorBytes target,std::int32_t ms,bool blocking,std::uint32_t curve=2) {
        ProcessHandle child;return state->faders->Bind(call.process,state->windows,member.handle,target,ms,curve,15,blocking,child,&access)==TalkColorFaderStatus::Ready;
    }
    ProcessCallbackStep Child(TalkWindowEffect child) {
        const auto target=Targets[static_cast<std::size_t>(child)];return ProcessCallbackStep::Call(Service(call.process,target,{window->serial,call.arguments[1]}));
    }
    ProcessCallbackStep NameFade(ProcessAccess& access) {
        const bool fade_in=effect==TalkWindowEffect::NameFadeIn;
        auto& cached=fade_in?state->colors.name_in_shadow:state->colors.name_out_shadow;
        if(step==0) {
            if(!cached){const auto palette=state->windows->ObservePalette();if(!palette)return ProcessCallbackStep::Blocked();cached=palette->black;(*cached)[3]=static_cast<std::uint8_t>(fade_in?255:0);}
            duration=std::bit_cast<std::int32_t>(call.arguments[1]);step=1;
        }
        for(;index<2;++index) {
            const auto row=state->windows->Observe(window);if(!row)return ProcessCallbackStep::Blocked();
            const auto color=index?*cached:TalkColorBytes{255,255,255,static_cast<std::uint8_t>(fade_in?255:0)};
            if(!Fade(access,row->name_plate.color_members[index],color,duration,false,fade_in?2u:1u))return ProcessCallbackStep::Blocked();
        }
        return ProcessCallbackStep::Return();
    }
    ProcessCallbackStep Open(ProcessAccess& access) {
        auto row=state->windows->Observe(window);if(!row)return ProcessCallbackStep::Blocked();
        if(step==0){if(row->active)return ProcessCallbackStep::Return();if(state->windows->StartOpeningState(window,access)!=WS::Ready)return ProcessCallbackStep::Blocked();
            step=1;}
        if(step==1) {
            row=state->windows->Observe(window);if(!row || !row->talk_type)return ProcessCallbackStep::Blocked();
            step=2;const auto type=*row->talk_type;
            if(type==0)return Child(TalkWindowEffect::OpenFace);
            if(type==1 || type==2)return Child(TalkWindowEffect::OpenStand);
        }
        return ProcessCallbackStep::Return();
    }
    ProcessCallbackStep OpenCategory(ProcessAccess& access) {
        const bool face=effect==TalkWindowEffect::OpenFace;
        for(;;) {
            auto row=state->windows->Observe(window);if(!row)return ProcessCallbackStep::Blocked();
            if(step==0) {const auto skip=state->manager->Skip(call.process);if(!skip)return ProcessCallbackStep::Blocked();duration=*skip?0:160;step=1;}
            if(step==1) {
                if(face) {
                    // Return is unused, but original CURRENT coordinate lookup is
                    // still a real dependency and must execute before carriers.
                    if(!state->layout || !state->layout->MinimumFaceWindowWidth())return ProcessCallbackStep::Blocked();
                    const auto result=state->windows->Side(window);if(!result)return ProcessCallbackStep::Blocked();side=*result;
                } else {
                    if(!row->location)return ProcessCallbackStep::Blocked();
                    position=row->position;
                    const auto y=Add(std::bit_cast<float>(position[1]),*row->location==0?-50.0f:50.0f);
                    if(!std::isfinite(y))return ProcessCallbackStep::Blocked();
            position[1]=Bits(y);
                    if(!state->windows->WritePosition(row->movable.identity,position,access))return ProcessCallbackStep::Blocked();
                }
                step=2;
            }
            if(step==2) {
                if(!Carrier(access,face?row->offset:row->movable,face?row->offset_position:TalkVectorBits{},duration,true))return ProcessCallbackStep::Blocked();
            step=3;
            }
            if(step==3) {
                if(face && !Carrier(access,row->movable,{},duration,false))return ProcessCallbackStep::Blocked();
            step=4;
            }
            if(step==4) {
                const auto palette=state->windows->ObservePalette();if(!palette)return ProcessCallbackStep::Blocked();
                if(!Fade(access,row->color_member,palette->frame,duration,!face))return ProcessCallbackStep::Blocked();
            step=5;
            }
            // Constructor-null face: the separate portrait movement branch is
            // not entered. No fabricated FaceManager, font result or face name.
            if(step==5) {
                if(!row->name_effect_enabled)return ProcessCallbackStep::Blocked();
                if(!*row->name_effect_enabled)return ProcessCallbackStep::Return();
                if(!face){const auto result=state->windows->Side(window);if(!result)return ProcessCallbackStep::Blocked();side=*result;}
                const auto x=face?(side==2?-50.0f:50.0f):(side==0?69.0f:(side==2?-69.0f:0.0f));
                position={Bits(x),0,0};if(!state->windows->WritePosition(row->name_plate.movable.identity,position,access))return ProcessCallbackStep::Blocked();
            step=6;
            }
            if(step==6) {
                // Preserve the original same-position target; do not invent an
                // intended zero destination for this stationary blocking child.
                if(!Carrier(access,row->name_plate.movable,position,duration,true))return ProcessCallbackStep::Blocked();
            step=7;
            }
            if(step==7) {step=8;return ProcessCallbackStep::Call(Service(call.process,Targets[5],{window->serial,std::bit_cast<std::uint32_t>(duration)}));}
            return ProcessCallbackStep::Return();
        }
    }
    ProcessCallbackStep Close(ProcessAccess& access) {
        auto row=state->windows->Observe(window);if(!row)return ProcessCallbackStep::Blocked();
        if(step==0) {
            if(!row->active)return ProcessCallbackStep::Return();
            if(state->windows->HideNextIcon(window,access)!=WS::Ready)return ProcessCallbackStep::Blocked();
            step=1;return ProcessCallbackStep::Call(Service(call.process,0x41f6c4,{100}));
        }
        if(step==1) {
            const auto skip=state->manager->Skip(call.process);if(!skip)return ProcessCallbackStep::Blocked();duration=*skip?0:160;
            if(state->windows->CloseEffectState(window,*skip!=0,access)!=WS::Ready)return ProcessCallbackStep::Blocked();
            step=2;
        }
        if(step==2) {
            row=state->windows->Observe(window);if(!row || !row->location)return ProcessCallbackStep::Blocked();
            position={0,Bits(*row->location==0?-50.0f:50.0f),0};
            if(!Carrier(access,row->movable,position,duration,false))return ProcessCallbackStep::Blocked();
            step=3;
        }
        if(step==3) {
            row=state->windows->Observe(window);if(!row || !Carrier(access,row->offset,row->offset_position,0,true))return ProcessCallbackStep::Blocked();
            step=4;
        }
        if(step==4) {
            row=state->windows->Observe(window);if(!row || !Fade(access,row->color_member,{255,255,255,0},duration,false))return ProcessCallbackStep::Blocked();
            step=5;
        }
        // Original tests face pointer, NOT name_override. Genuinely no-face
        // windows therefore do not run the separate nameplate close branch.
        return ProcessCallbackStep::Return();
    }
    ProcessCallbackStep KeyWait(ProcessAccess& access) {
        auto row=state->windows->Observe(window);if(!row)return ProcessCallbackStep::Blocked();
        if(step==0) {
            const auto value=static_cast<std::uint8_t>(call.arguments[1]);
            if(state->windows->RestoreEffectFlags(window,row->active,row->first_message,value,&access)!=WS::Ready)return ProcessCallbackStep::Blocked();
            if(!call.arguments[1])return ProcessCallbackStep::Return();
            if(!row->first_message)return ProcessCallbackStep::Blocked();
            if(*row->first_message) {
                if(state->windows->RestoreEffectFlags(window,row->active,std::uint8_t{0},value,&access)!=WS::Ready)return ProcessCallbackStep::Blocked();
            return ProcessCallbackStep::Return();
            }
            step=1;
        }
        if(step==1) {if(!row->text_layout)return ProcessCallbackStep::Blocked();distance=(row->line+1u)*std::uint32_t((*row->text_layout)[2]);step=2;}
        for(;index<8;) {
            const TalkWindowView view{window,static_cast<std::uint8_t>(index)};const auto text=state->windows->ObserveString(view);
            if(!text || !text->known.test(0))return ProcessCallbackStep::Blocked();
            if(step==2) {
                if(!text->words[0]){++index;continue;}
                position=text->position;auto shifted=text->position;
                const auto y=Add(std::bit_cast<float>(shifted[1]),static_cast<float>(std::bit_cast<std::int32_t>(distance)));
                if(!std::isfinite(y))return ProcessCallbackStep::Blocked();
            shifted[1]=Bits(y);
                if(!state->windows->WritePosition(text->movable.identity,shifted,access))return ProcessCallbackStep::Blocked();
            step=3;
            }
            if(step==3) {ProcessHandle child;if(state->motion->BindScroll(call.process,view,position,80,2,0,child,&access)!=TalkMotionStatus::Ready)return ProcessCallbackStep::Blocked();
            step=4;}
            if(state->windows->SetStringAlpha(view,0,access)!=WS::Ready)return ProcessCallbackStep::Blocked();
            ++index;step=2;
        }
        return ProcessCallbackStep::Return();
    }
    ProcessCallbackStep Step(ProcessAccess& access) override {
        const auto scheduler=state->scheduler.lock();if(!scheduler || !access.BelongsTo(*scheduler) || !state->windows->Observe(window))return ProcessCallbackStep::Blocked();
        switch(effect) {
        case TalkWindowEffect::FadeOutFace: {
            // Only constructor-null attached faces are admitted by this owner.
            // Active windows tail-call the actual close effect and do not then
            // execute the inactive branch's identifier/override clears.
            const auto row=state->windows->Observe(window);
            if(!row)return ProcessCallbackStep::Blocked();
            if(step==0 && row->active){step=1;return Child(TalkWindowEffect::StartClose);}
            if(step==0 && state->windows->ClearFaceState(window,false,access)!=WS::Ready)return ProcessCallbackStep::Blocked();
            return ProcessCallbackStep::Return();}
        case TalkWindowEffect::FadeOutFaceInSkip:
            if(state->windows->ClearFaceState(window,true,access)!=WS::Ready)return ProcessCallbackStep::Blocked();
            return ProcessCallbackStep::Return();
        case TalkWindowEffect::StartOpen:return Open(access);
        case TalkWindowEffect::OpenFace:case TalkWindowEffect::OpenStand:return OpenCategory(access);
        case TalkWindowEffect::StartClose:return Close(access);
        case TalkWindowEffect::NameFadeIn:case TalkWindowEffect::NameFadeOut:return NameFade(access);
        case TalkWindowEffect::SetKeyWait:return KeyWait(access);
        case TalkWindowEffect::WaitNextMessage: {
            const auto row=state->windows->Observe(window);if(!row || !Carrier(access,row->offset,row->offset_position,0,true))return ProcessCallbackStep::Blocked();
            return ProcessCallbackStep::Return();}
        }
        return ProcessCallbackStep::Blocked();
    }
};
NativeTalkWindowEffects::NativeTalkWindowEffects(std::shared_ptr<State> state):state_(std::move(state)){}
NativeTalkWindowEffects::~NativeTalkWindowEffects()=default;
ES NativeTalkWindowEffects::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<ObjectHandleRegistry> objects,std::shared_ptr<NativeTalkWindow> windows,std::shared_ptr<NativeTalkMotion> motion,std::shared_ptr<NativeTalkColorFader> faders,
    std::shared_ptr<NativeTalkLayout> layout,std::shared_ptr<TalkEffectManagerState> manager,std::shared_ptr<NativeTalkWindowEffects>& output) {
    if(!scheduler || !scheduler->root(2))return ES::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !windows || !windows->UsesScheduler(*scheduler) || !objects || !windows->UsesObjectRegistry(*objects) || !motion || !motion->UsesScheduler(*scheduler) || !motion->UsesWindow(*windows) || !faders || !faders->UsesScheduler(*scheduler) || !faders->UsesObjectRegistry(*objects) || (layout && !layout->UsesScheduler(*scheduler)) || !manager || !manager->UsesScheduler(*scheduler))return ES::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->windows=std::move(windows);state->motion=std::move(motion);state->faders=std::move(faders);state->layout=std::move(layout);state->manager=std::move(manager);
    auto owner=std::shared_ptr<NativeTalkWindowEffects>(new NativeTalkWindowEffects(state));if(!registry->Register(Targets,owner))return ES::DuplicateBinding;
    output=std::move(owner);return ES::Ready;
}
std::optional<ProcessCall> NativeTalkWindowEffects::Call(ProcessHandle parent,TalkWindowHandle window,TalkWindowEffect effect,std::int32_t parameter) const {
    if(!state_->Live() || !state_->windows->Observe(window) || static_cast<std::size_t>(effect)>=Targets.size())return {};
    return Service(std::move(parent),Targets[static_cast<std::size_t>(effect)],{window->serial,std::bit_cast<std::uint32_t>(parameter)});
}
std::optional<TalkEffectStaticColors> NativeTalkWindowEffects::ObserveStaticColors() const {return state_->Live()?std::optional(state_->colors):std::nullopt;}
bool NativeTalkWindowEffects::UsesScheduler(const NativeProcessScheduler& scheduler) const noexcept{return state_->scheduler.lock().get()==&scheduler;}
std::unique_ptr<ProcessContinuation> NativeTalkWindowEffects::Begin(const ProcessCall& call) {
    if(!state_->Live() || call.kind!=ProcessCallKind::Service || call.has_self || call.this_adjustment || call.argument_count!=2 || !call.process)return {};
    const auto parent=state_->scheduler.lock()->Observe(call.process);if(!parent || !parent->linked || parent->flags&1u)return {};
    const auto found=std::find(Targets.begin(),Targets.end(),call.target);if(found==Targets.end())return {};
    for(const auto& window:state_->windows->Handles())if(window->serial==call.arguments[0]) {
        auto next=std::make_unique<Continuation>();next->state=state_;next->call=call;next->window=window;next->effect=static_cast<TalkWindowEffect>(found-Targets.begin());return next;
    }
    return {};
}
}

namespace fates::presentation::native {
bool NativeTalkWindowEffects::UsesOwners(const NativeTalkWindow& w,const TalkEffectManagerState& m) const noexcept {return state_->windows.get()==&w && state_->manager.get()==&m;}
}
