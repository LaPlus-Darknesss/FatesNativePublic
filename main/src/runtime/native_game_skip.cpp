#include "fates/runtime/native_game_skip.hpp"
#include "fates/services/core_services.hpp"
#include "fates/event/backend/backend_spine.hpp"
#include <algorithm>
#include <bit>
#include <limits>
#include <map>

namespace fates::runtime::native {
using namespace presentation::native;
namespace {
constexpr std::array<std::uint32_t,5> Operations{0x4eb4ec,0x4eb9fc,0x4eb814,0x4ebaf8,0x4eb870};
constexpr std::array<std::uint32_t,4> Controls{0x1e0b00,0x1e0b24,0x1e0b9c,0x1e0bf0};
constexpr std::uint32_t RendererPersistent=0x1ed3f8,RendererDestroy=0x1ed414;
constexpr std::array Targets{Operations[0],Operations[1],Operations[2],Operations[3],Operations[4],
    Controls[0],Controls[1],Controls[2],Controls[3],RendererPersistent,RendererDestroy};
constexpr char MessageKey[]="MID_SYS_\x83\x58\x83\x4c\x83\x62\x83\x76\x92\x86";
ProcessCall Service(ProcessHandle context,std::uint32_t target,std::uint64_t serial) {
    ProcessCall c;c.process=std::move(context);c.kind=ProcessCallKind::Service;c.target=target;
    c.arguments[0]=static_cast<std::uint32_t>(serial);c.arguments[1]=static_cast<std::uint32_t>(serial>>32);c.argument_count=2;
    return c;
}
std::uint64_t Serial(const ProcessCall& c) {return c.arguments[0]|(std::uint64_t(c.arguments[1])<<32);}
}
struct NativeGameSkip::State {
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<NativeFadeSystem> fade;
    std::shared_ptr<GameSkipInputSource> input;
    std::shared_ptr<GameSkipAudioSink> audio;
    std::shared_ptr<GameSkipDrawSink> draw;
    std::map<std::uint64_t,GameSkipSnapshot> instances;
    std::map<std::uint64_t,GameSkipControlSnapshot> controls;
    std::map<std::uint64_t,ProcessHandle> renderers;
    std::optional<GameSkipHandle> current;
    std::uint64_t serial{},control_serial{};
    bool Live() const {auto s=scheduler.lock();return s && s->root(2);}
    GameSkipSnapshot* Find(GameSkipHandle h) {
        if(!h)return nullptr;auto it=instances.find(h->serial);
        return it!=instances.end() && it->second.identity==h?&it->second:nullptr;
    }
    GameSkipControlSnapshot* Find(GameSkipControlHandle h) {
        if(!h)return nullptr;auto it=controls.find(h->serial);
        return it!=controls.end() && it->second.identity==h?&it->second:nullptr;
    }
    bool CurrentKnown() {return current && (!*current || Find(*current));}
    GameSkipStatus Mutable(ProcessAccess* access) const {
        auto s=scheduler.lock();if(!s || !s->root(2))return GameSkipStatus::Retired;
        if(access)return access->BelongsTo(*s)?GameSkipStatus::Ready:GameSkipStatus::MismatchedDomain;
        return s->busy()?GameSkipStatus::Busy:GameSkipStatus::Ready;
    }
    GameSkipStatus Construct(ProcessAccess* access,GameSkipHandle& out) {
        if(auto s=Mutable(access);s!=GameSkipStatus::Ready)return s;
        if(serial==std::numeric_limits<std::uint64_t>::max())return GameSkipStatus::IdentityExhausted;
        GameSkipSnapshot row;row.identity=std::make_shared<const GameSkipIdentity>(GameSkipIdentity{++serial});
        out=row.identity;instances.emplace(serial,std::move(row));current=out;return GameSkipStatus::Ready;
    }
    GameSkipStatus Absent(ProcessAccess* access) {
        if(auto s=Mutable(access);s!=GameSkipStatus::Ready)return s;
        current=GameSkipHandle{};return GameSkipStatus::Ready;
    }
    GameSkipStatus Capture(ProcessAccess* access,GameSkipControlHandle& out) {
        if(auto s=Mutable(access);s!=GameSkipStatus::Ready)return s;
        if(!CurrentKnown())return GameSkipStatus::UnknownCurrent;
        if(control_serial==std::numeric_limits<std::uint64_t>::max())return GameSkipStatus::IdentityExhausted;
        GameSkipControlSnapshot row;row.identity=std::make_shared<const GameSkipControlIdentity>(GameSkipControlIdentity{++control_serial});
        row.captured=*current;
        if(auto* instance=Find(row.captured)) {
            row.was_skipping=event::backend::GameSkipStateIsSkipping(instance->state);row.flags=instance->flags;
        }
        out=row.identity;controls.emplace(control_serial,std::move(row));return GameSkipStatus::Ready;
    }
    GameSkipStatus Release(ProcessAccess* access,GameSkipControlHandle h) {
        if(auto s=Mutable(access);s!=GameSkipStatus::Ready)return s;
        if(!Find(h))return GameSkipStatus::InvalidHandle;
        controls.erase(h->serial);return GameSkipStatus::Ready;
    }
};
struct NativeGameSkip::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;
    ProcessCall call;
    GameSkipHandle owner;
    GameSkipControlHandle control;
    unsigned stage{},target{};
    Continuation(std::shared_ptr<State> s,ProcessCall c):state(std::move(s)),call(std::move(c)){}
    ProcessCallbackStep Step(ProcessAccess& access) override {
        auto scheduler=state->scheduler.lock();
        if(!scheduler || !access.BelongsTo(*scheduler))return ProcessCallbackStep::Blocked();
        if(call.target==RendererDestroy) {state->renderers.erase(call.process->serial);return ProcessCallbackStep::Return();}
        if(control) {
            auto* record=state->Find(control);if(!record)return ProcessCallbackStep::Blocked();
            if(!record->captured)return ProcessCallbackStep::Return();
            auto* row=state->Find(record->captured);if(!row)return ProcessCallbackStep::Blocked();
            if(call.target==Controls[0])row->flags&=~3u;
            else if(call.target==Controls[3])row->flags=*record->flags|(row->flags&4u);
            else {
                row->flags|=2u;
                if(row->state!=0 && !*record->was_skipping)row->flags|=4u;
                if(call.target==Controls[2])row->flags|=1u;
            }
            return ProcessCallbackStep::Return();
        }
        if(call.target==RendererPersistent && stage==0) {
            if(!state->CurrentKnown())return ProcessCallbackStep::Blocked();
            owner=*state->current;
            if(!owner)return ProcessCallbackStep::Return();
            stage=100;
        }
        auto* row=state->Find(owner);if(!row)return ProcessCallbackStep::Blocked();
        if(call.target==Operations[2]) {row->flags=services::GameSkipRequestEscape(row->state,row->flags);return ProcessCallbackStep::Return();}
        if(call.target==Operations[3]) {
            if(stage==0) {stage=1;if(row->renderer)return ProcessCallbackStep::Delete(row->renderer);}
            row->renderer={};state->instances.erase(owner->serial);return ProcessCallbackStep::Return();
        }
        if(call.target==Operations[4] || call.target==RendererPersistent) {
            if(stage!=101) {
                if(row->state==0)return ProcessCallbackStep::Return();
                if(row->state==1) {
                    auto fade=state->fade->ObserveChannel(0);if(!fade)return ProcessCallbackStep::Blocked();
                    if(fade->active_fade_in)return ProcessCallbackStep::Return();
                }
                if(!row->render_counter)return ProcessCallbackStep::Blocked();
                *row->render_counter+=access.frame_delta()*6u;stage=101;
            }
            auto fade=state->fade->ObserveChannel(0);if(!fade)return ProcessCallbackStep::Blocked();
            const auto count=*row->render_counter;
            const auto signed_count=std::bit_cast<std::int32_t>(count);
            const auto quotient=signed_count>=0?signed_count/256:-1-((-1-signed_count)/256);
            const auto cycle=quotient-(quotient/3)*3;
            const auto low=static_cast<std::uint8_t>(count);
            const auto component=cycle==0?std::uint8_t{255}:cycle==1?static_cast<std::uint8_t>(255-low):low;
            state->draw->Draw({owner,call.process,{component,255,component,fade->color[3]},1001,MessageKey});
            return ProcessCallbackStep::Return();
        }
        const bool tick=call.target==Operations[0];
        // Each stage is retained around every possible unknown input or nested
        // call. Only pure local steps are coalesced within one host callback.
        for(;;) {
            if(stage==0) {
                if(!tick) {if(row->state!=0)return ProcessCallbackStep::Return();stage=10;}
                else if(row->flags&3u)stage=20;
                else stage=1;
            } else if(stage==1) {
                auto buttons=state->input->TriggerButtons();if(!buttons)return ProcessCallbackStep::Blocked();
                stage=(*buttons&8u)?2u:20u;
            } else if(stage==2) {
                auto tutorial=state->input->Tutorial();if(!tutorial)return ProcessCallbackStep::Blocked();
                const bool active=tutorial->present && tutorial->mode==0;
                stage=!active && row->state==0?10u:20u;
            } else if(stage==10) {row->state=1;row->flags&=~4u;stage=11;}
            else if(stage==11 || stage==12) {
                const auto channel=stage-11;auto fade=state->fade->ObserveChannel(channel);
                if(!fade)return ProcessCallbackStep::Blocked();row->saved_goals[channel]=fade->goal;++stage;
            } else if(stage==13) {
                if(!row->renderer) {
                    auto type=ProcessType::Base();type.methods[0].target=RendererDestroy;type.methods[2].target=RendererPersistent;
                    ProcessHandle h;
                    if(access.Create(scheduler->root(2),ProcessProgram::Default(),"GameSkipRenderer",false,type,h)!=ProcessStatus::Ready)
                        return ProcessCallbackStep::Blocked();
                    row->renderer=h;state->renderers.emplace(h->serial,h);
                }
                if(!tick)return ProcessCallbackStep::Return();stage=20;
            } else if(stage==20) {
                if(row->state==0) {row->flags&=~4u;return ProcessCallbackStep::Return();}
                if(row->state==1 || row->state==2 || row->state==4) {target=0;stage=21;}
                else if(row->state==3) {
                    if(row->wait_counter<8) {
                        const auto sum=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(row->wait_counter)+access.frame_delta());
                        row->wait_counter=std::min<std::int32_t>(sum,8);return ProcessCallbackStep::Return();
                    }
                    if(!(row->flags&5u))return ProcessCallbackStep::Return();target=0;stage=40;
                } else return ProcessCallbackStep::Return();
            } else if(stage==21) {
                auto fade=state->fade->ObserveChannel(target);if(!fade)return ProcessCallbackStep::Blocked();
                if(fade->active)return ProcessCallbackStep::Return();
                if(++target<2)continue;
                if(row->state==2) {row->state=3;return ProcessCallbackStep::Return();}
                if(row->state==4) {
                    row->state=0;row->flags&=~4u;stage=50;
                    if(row->renderer)return ProcessCallbackStep::Delete(row->renderer);
                } else if(row->flags&5u) {row->wait_counter=8;row->state=3;return ProcessCallbackStep::Return();}
                else {target=0;stage=30;}
            } else if(stage==30) {
                if(target==2) {
                    row->wait_counter=0;row->state=2;row->render_counter=0;stage=31;
                    state->audio->StopBgm(250);
                } else {
                    auto fade=state->fade->ObserveChannel(target);if(!fade)return ProcessCallbackStep::Blocked();
                    const auto channel=target++;
                    if(!fade->blackout)return ProcessCallbackStep::Call(NativeFadeSystem::FadeCall(call.process,250,channel,FadeTone::Black,FadeDirection::Out));
                }
            } else if(stage==31) {state->audio->StopLse(250);return ProcessCallbackStep::Return();}
            else if(stage==40) {
                if(target==2) {row->state=4;return ProcessCallbackStep::Return();}
                auto fade=state->fade->ObserveChannel(target);if(!fade)return ProcessCallbackStep::Blocked();
                if(fade->blackout) {
                    if(!row->saved_goals[target])return ProcessCallbackStep::Blocked();
                    const auto color=*row->saved_goals[target];
                    if(color[3]!=255) {
                        const auto channel=target++;
                        return ProcessCallbackStep::Call(NativeFadeSystem::FadeCall(call.process,250,channel,
                            color[0]==255?FadeTone::White:FadeTone::Black,FadeDirection::In));
                    }
                }
                ++target;
            } else if(stage==50) {row->renderer={};return ProcessCallbackStep::Return();}
            else return ProcessCallbackStep::Blocked();
        }
    }
};
NativeGameSkip::NativeGameSkip(std::shared_ptr<State> s):state_(std::move(s)){}
NativeGameSkip::~NativeGameSkip()=default;
GameSkipStatus NativeGameSkip::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<NativeFadeSystem> fade,std::shared_ptr<GameSkipInputSource> input,std::shared_ptr<GameSkipAudioSink> audio,
    std::shared_ptr<GameSkipDrawSink> draw,std::shared_ptr<NativeGameSkip>& out) {
    if(!scheduler || !scheduler->root(2))return GameSkipStatus::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !fade || !fade->UsesScheduler(*scheduler))return GameSkipStatus::MismatchedDomain;
    if(!input)return GameSkipStatus::MissingInput;if(!audio || !draw)return GameSkipStatus::MissingSink;
    auto s=std::make_shared<State>();s->scheduler=scheduler;s->fade=std::move(fade);s->input=std::move(input);s->audio=std::move(audio);s->draw=std::move(draw);
    auto next=std::shared_ptr<NativeGameSkip>(new NativeGameSkip(s));
    if(!registry->Register(Targets,next))return GameSkipStatus::DuplicateBinding;
    out=std::move(next);return GameSkipStatus::Ready;
}
GameSkipStatus NativeGameSkip::ConstructAndPublish(GameSkipHandle& h) {return state_->Construct(nullptr,h);}
GameSkipStatus NativeGameSkip::ConstructAndPublish(ProcessAccess& a,GameSkipHandle& h) {return state_->Construct(&a,h);}
GameSkipStatus NativeGameSkip::PublishAbsent() {return state_->Absent(nullptr);}
GameSkipStatus NativeGameSkip::PublishAbsent(ProcessAccess& a) {return state_->Absent(&a);}
GameSkipStatus NativeGameSkip::Capture(GameSkipControlHandle& h) {return state_->Capture(nullptr,h);}
GameSkipStatus NativeGameSkip::Capture(ProcessAccess& a,GameSkipControlHandle& h) {return state_->Capture(&a,h);}
GameSkipStatus NativeGameSkip::ReleaseControl(GameSkipControlHandle h) {return state_->Release(nullptr,std::move(h));}
GameSkipStatus NativeGameSkip::ReleaseControl(ProcessAccess& a,GameSkipControlHandle h) {return state_->Release(&a,std::move(h));}
std::optional<ProcessCall> NativeGameSkip::Call(ProcessHandle context,GameSkipHandle h,GameSkipOperation op) const {
    if(!state_->Live() || !state_->Find(h) || static_cast<unsigned>(op)>=Operations.size())return {};
    return Service(std::move(context),Operations[static_cast<unsigned>(op)],h->serial);
}
std::optional<ProcessCall> NativeGameSkip::ControlCall(ProcessHandle context,GameSkipControlHandle h,GameSkipControlOperation op) const {
    if(!state_->Live() || !state_->Find(h) || static_cast<unsigned>(op)>=Controls.size())return {};
    return Service(std::move(context),Controls[static_cast<unsigned>(op)],h->serial);
}
ProcessStatus NativeGameSkip::Begin(GameSkipHandle h,GameSkipOperation op) {
    auto s=state_->scheduler.lock();if(!s || !s->root(2))return ProcessStatus::Retired;
    auto c=Call(s->root(2),std::move(h),op);return c?s->BeginServiceCall(*c):ProcessStatus::InvalidCall;
}
ProcessStatus NativeGameSkip::BeginControl(GameSkipControlHandle h,GameSkipControlOperation op) {
    auto s=state_->scheduler.lock();if(!s || !s->root(2))return ProcessStatus::Retired;
    auto c=ControlCall(s->root(2),std::move(h),op);return c?s->BeginServiceCall(*c):ProcessStatus::InvalidCall;
}
std::optional<GameSkipHandle> NativeGameSkip::Current() const {
    return state_->Live() && state_->CurrentKnown()?state_->current:std::optional<GameSkipHandle>{};
}
std::optional<GameSkipSnapshot> NativeGameSkip::Observe(GameSkipHandle h) const {
    if(state_->Live())if(auto* row=state_->Find(std::move(h)))return *row;return {};
}
std::optional<GameSkipControlSnapshot> NativeGameSkip::ObserveControl(GameSkipControlHandle h) const {
    if(state_->Live())if(auto* row=state_->Find(std::move(h)))return *row;return {};
}
std::vector<ProcessHandle> NativeGameSkip::RenderProcesses() const {
    std::vector<ProcessHandle> result;if(!state_->Live())return result;
    for(const auto& [key,h]:state_->renderers)result.push_back(h);return result;
}
std::optional<bool> NativeGameSkip::ControlIsSkip(GameSkipControlHandle h) const {
    auto c=ObserveControl(h);if(!c)return {};if(!c->captured)return false;
    auto row=Observe(c->captured);return row?std::optional<bool>{event::backend::GameSkipStateIsSkipping(row->state)}:std::optional<bool>{};
}
std::optional<bool> NativeGameSkip::ControlIsWait(GameSkipControlHandle h) const {
    auto c=ObserveControl(h);if(!c)return {};if(!c->captured)return false;
    auto row=Observe(c->captured);return row?std::optional<bool>{services::GameSkipIsWait(row->state,row->flags)}:std::optional<bool>{};
}
std::optional<bool> NativeGameSkip::ControlIsEscape(GameSkipControlHandle h) const {
    auto c=ObserveControl(h);if(!c)return {};if(!c->captured)return false;
    auto row=Observe(c->captured);return row?std::optional<bool>{(row->flags&4u)!=0}:std::optional<bool>{};
}
bool NativeGameSkip::UsesScheduler(const NativeProcessScheduler& s) const noexcept {
    return state_->scheduler.lock().get()==&s;
}
GameSkipStatus NativeGameSkip::PrepareEventFadeEnd(ProcessAccess& access,GameSkipHandle h,
    bool& skipping,bool& blackout) {
    if(auto s=state_->Mutable(&access);s!=GameSkipStatus::Ready)return s;
    auto* row=state_->Find(h);if(!row)return GameSkipStatus::InvalidHandle;
    row->flags|=2u;skipping=event::backend::GameSkipStateIsSkipping(row->state);
    blackout=row->state==3;
    if(skipping)row->saved_goals={FadeColor{0,0,0,255},FadeColor{0,0,0,255}};
    return GameSkipStatus::Ready;
}
std::unique_ptr<ProcessContinuation> NativeGameSkip::Begin(const ProcessCall& c) {
    if(!state_->Live() || !c.process || c.this_adjustment)return {};
    auto next=std::make_unique<Continuation>(state_,c);
    if(c.target==RendererPersistent || c.target==RendererDestroy) {
        if(!c.has_self || c.argument_count || c.kind!=(c.target==RendererPersistent?ProcessCallKind::Persistent:ProcessCallKind::Destroy))return {};
        auto it=state_->renderers.find(c.process->serial);
        if(it==state_->renderers.end() || it->second!=c.process)return {};
    } else {
        if(c.kind!=ProcessCallKind::Service || c.has_self || c.argument_count!=2)return {};
        const auto id=Serial(c);
        if(std::find(Operations.begin(),Operations.end(),c.target)!=Operations.end()) {
            auto it=state_->instances.find(id);if(it==state_->instances.end())return {};next->owner=it->second.identity;
        } else if(std::find(Controls.begin(),Controls.end(),c.target)!=Controls.end()) {
            auto it=state_->controls.find(id);if(it==state_->controls.end())return {};next->control=it->second.identity;
        } else return {};
    }
    return next;
}
}

namespace fates::runtime::native {
bool NativeGameSkip::UsesInput(const GameSkipInputSource& source) const noexcept {return state_->input.get()==&source;}
}

namespace fates::runtime::native {
GameSkipStatus NativeGameSkip::DisableCurrent(ProcessAccess& access) {
    const auto status=state_->Mutable(&access);if(status!=GameSkipStatus::Ready)return status;
    if(!state_->CurrentKnown())return GameSkipStatus::UnknownCurrent;
    if(auto* row=state_->Find(*state_->current))row->flags|=1u;
    return GameSkipStatus::Ready;
}
}

namespace fates::runtime::native {
GameSkipStatus NativeGameSkip::SetSavedFadeGoal(ProcessAccess& access,GameSkipHandle handle,
    std::uint32_t target,presentation::native::FadeColor color) {
    if(const auto result=state_->Mutable(&access);result!=GameSkipStatus::Ready)return result;
    auto* row=state_->Find(handle);if(!row || target>=2)return GameSkipStatus::InvalidHandle;
    row->saved_goals[target]=color;return GameSkipStatus::Ready;
}
bool NativeGameSkip::UsesFadeSystem(const presentation::native::NativeFadeSystem& fade)const noexcept {
    return state_->fade.get()==&fade;
}
}

namespace fates::runtime::native {
GameSkipStatus NativeGameSkip::MergeFlags(ProcessAccess& access,GameSkipHandle handle,std::uint32_t bits) {
    if(auto status=state_->Mutable(&access);status!=GameSkipStatus::Ready)return status;
    auto* row=state_->Find(handle);if(!row)return GameSkipStatus::InvalidHandle;
    row->flags|=bits;return GameSkipStatus::Ready;
}
}
