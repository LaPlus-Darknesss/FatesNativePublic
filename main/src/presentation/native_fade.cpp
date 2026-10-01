#include "fates/presentation/native_fade.hpp"
#include <algorithm>
#include <bit>
#include <limits>
#include <map>

namespace fates::presentation::native {
using namespace runtime::native;
namespace {
constexpr std::uint32_t Initialize=0x105050,CreateFade=0x33a2c8,WaitBind=0x3cf6c4,
    WaitTick=0x33a454,WaitDestroy=0x33a47c,Persistent=0x33a48c,Tick=0x33a5b0,Destroy=0x33a720;
constexpr std::array Targets{Initialize,CreateFade,WaitBind,WaitTick,WaitDestroy,Persistent,Tick,Destroy};
std::shared_ptr<const ProcessProgram> FadeProgram() {
    //005BC3A0 initializes a distinct program at75201C, separate from default.
    static const auto value=ProcessProgram::Create({{13,0,0,8,1},{}});return value;
}
ProcessType FadeType(bool wait) {
    auto type=ProcessType::Base();type.methods[0].target=wait?WaitDestroy:Destroy;
    type.methods[1].target=wait?WaitTick:Tick;
    if(!wait)type.methods[2].target=Persistent;
    return type;
}
std::int32_t Duration(std::int32_t ms) {
    const float input=static_cast<float>(ms);
    const float converted=input*std::bit_cast<float>(std::uint32_t{0x3d75c28f});
    return static_cast<std::int32_t>(converted);
}
std::uint8_t Interpolate(std::uint8_t start,std::uint8_t goal,std::int32_t elapsed,std::int32_t duration) {
    // Match the separately rounded VMUL, VDIV, VADD and unsigned truncating
    // conversion. In particular, don't replace this with integer division or FMA.
    const float delta=static_cast<float>(int(goal)-int(start));
    const volatile float product=delta*static_cast<float>(elapsed);
    const volatile float quotient=product/static_cast<float>(duration);
    const float value=quotient+static_cast<float>(start);
    if(!(value>0.0f))return 0;
    if(value>=4294967296.0f)return 255;
    return static_cast<std::uint8_t>(static_cast<std::uint32_t>(value));
}
}
struct NativeFadeSystem::State {
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<FadeDrawSink> sink;
    std::map<std::uint64_t,FadeProcessSnapshot> records;
    std::array<ProcessHandle,2> current;
    FadeProcessSnapshot* Find(const ProcessHandle& h) {
        if(!h)return nullptr;auto it=records.find(h->serial);
        return it!=records.end() && it->second.process==h?&it->second:nullptr;
    }
    bool Live() const {auto s=scheduler.lock();return s && s->root(2);}
};
struct NativeFadeSystem::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;
    ProcessCall call;
    unsigned stage{};
    Continuation(std::shared_ptr<State> s,ProcessCall c):state(std::move(s)),call(std::move(c)){}
    ProcessCallbackStep Step(ProcessAccess& access) override {
        auto scheduler=state->scheduler.lock();
        if(!scheduler || !access.BelongsTo(*scheduler))return ProcessCallbackStep::Blocked();
        if(stage)return ProcessCallbackStep::Return(); // nested Delete has completed
        if(call.target==Initialize) {state->current={};return ProcessCallbackStep::Return();}
        if(call.target==WaitBind) {
            const auto target=call.arguments[0];auto* current=state->Find(state->current[target]);
            if(!current || !current->active)return ProcessCallbackStep::Return();
            ProcessHandle handle;
            if(access.Create(call.process,ProcessProgram::Default(),"FadeWait",true,FadeType(true),handle)!=ProcessStatus::Ready)
                return ProcessCallbackStep::Blocked();
            FadeProcessSnapshot row;row.process=handle;row.logical_target=static_cast<std::uint8_t>(target);row.wait_process=true;
            state->records.emplace(handle->serial,std::move(row));return ProcessCallbackStep::Return();
        }
        if(call.target==CreateFade) {
            const auto ms=std::bit_cast<std::int32_t>(call.arguments[0]);const auto target=call.arguments[1];
            const auto tone=call.arguments[2];const auto direction=call.arguments[3];
            const auto old_handle=state->current[target];auto* old=state->Find(old_handle);
            if((!old && direction==0) || (old && old->direction==direction && ms!=0))return ProcessCallbackStep::Return();
            FadeProcessSnapshot row;row.logical_target=static_cast<std::uint8_t>(target);
            row.tone=static_cast<std::uint8_t>(tone);row.direction=static_cast<std::uint8_t>(direction);
            row.duration=Duration(ms);row.active=true;row.first_draw_pending=true;
            row.start=row.goal=row.color=tone==0?FadeColor{0,0,0,255}:FadeColor{255,255,255,255};
            row.start[3]=direction==0?255:0;row.goal[3]=direction==0?0:255;row.color[3]=row.start[3];
            if(row.duration==0)row.color=row.goal;
            // Create invokes no callbacks. Admission failure publishes no partial
            // binding; the original publish->Create window has no observer here.
            ProcessHandle handle;
            if(access.Create(scheduler->root(2),FadeProgram(),"ProcFade",false,FadeType(false),handle)!=ProcessStatus::Ready)
                return ProcessCallbackStep::Blocked();
            row.process=handle;state->records.emplace(handle->serial,row);state->current[target]=handle;
            if(old) {
                auto& created=state->records.at(handle->serial);
                created.color=created.start=old->color;
                stage=1;return ProcessCallbackStep::Delete(old_handle);
            }
            return ProcessCallbackStep::Return();
        }
        auto* row=state->Find(call.process);if(!row)return ProcessCallbackStep::Blocked();
        if(call.target==WaitTick) {
            auto* current=state->Find(state->current[row->logical_target]);
            if(current && current->active)return ProcessCallbackStep::Return();
            stage=1;return ProcessCallbackStep::Delete(call.process);
        }
        if(call.target==WaitDestroy || call.target==Destroy) {
            if(call.target==Destroy && state->current[row->logical_target]==call.process)state->current[row->logical_target]={};
            state->records.erase(call.process->serial);return ProcessCallbackStep::Return();
        }
        if(call.target==Persistent) {
            // A render consumer receives a retained identity and immutable color.
            // The original clears first-draw pending only after DrawRect/PopState.
            state->sink->Draw({row->process,row->logical_target,row->color,1000});
            row->first_draw_pending=false;return ProcessCallbackStep::Return();
        }
        if(call.target==Tick) {
            if(row->elapsed<row->duration) {
                const auto sum=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(row->elapsed)+access.frame_delta());
                row->elapsed=std::min(sum,row->duration);
            }
            if(row->elapsed==row->duration) {
                row->active=false;
                if(row->direction==0) {stage=1;return ProcessCallbackStep::Delete(call.process);}
            }
            if(row->duration>0)
                for(unsigned i=0;i<4;++i)row->color[i]=Interpolate(row->start[i],row->goal[i],row->elapsed,row->duration);
            return ProcessCallbackStep::Return();
        }
        return ProcessCallbackStep::Blocked();
    }
};
NativeFadeSystem::NativeFadeSystem(std::shared_ptr<State> state):state_(std::move(state)){}
NativeFadeSystem::~NativeFadeSystem()=default;
FadeStatus NativeFadeSystem::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<FadeDrawSink> sink,std::shared_ptr<NativeFadeSystem>& out) {
    if(!scheduler || !scheduler->root(2))return FadeStatus::NullScheduler;
    if(!sink)return FadeStatus::MissingSink;
    if(!registry || !scheduler->UsesCallbacks(registry.get()))return FadeStatus::MismatchedRegistry;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->sink=std::move(sink);
    auto next=std::shared_ptr<NativeFadeSystem>(new NativeFadeSystem(state));
    if(!registry->Register(Targets,next))return FadeStatus::DuplicateBinding;
    out=std::move(next);return FadeStatus::Ready;
}
ProcessCall NativeFadeSystem::FadeCall(ProcessHandle context,std::int32_t ms,std::uint32_t target,FadeTone tone,FadeDirection direction) {
    ProcessCall c;c.process=std::move(context);c.kind=ProcessCallKind::Service;c.target=CreateFade;
    c.arguments={static_cast<std::uint32_t>(ms),target,static_cast<std::uint32_t>(tone),static_cast<std::uint32_t>(direction)};
    c.argument_count=4;return c;
}
ProcessCall NativeFadeSystem::WaitCall(ProcessHandle parent,std::uint32_t target) {
    ProcessCall c;c.process=std::move(parent);c.kind=ProcessCallKind::Service;c.target=WaitBind;c.has_self=true;
    c.arguments[0]=target;c.argument_count=1;return c;
}
ProcessStatus NativeFadeSystem::BeginFade(std::int32_t ms,std::uint32_t target,FadeTone tone,FadeDirection direction) {
    auto s=state_->scheduler.lock();if(!s || !s->root(2))return ProcessStatus::Retired;
    if(target>1 || static_cast<unsigned>(tone)>1 || static_cast<unsigned>(direction)>1)return ProcessStatus::InvalidCall;
    return s->BeginServiceCall(FadeCall(s->root(2),ms,target,tone,direction));
}
ProcessStatus NativeFadeSystem::BeginWaitBind(ProcessHandle parent,std::uint32_t target) {
    auto s=state_->scheduler.lock();if(!s || !s->root(2))return ProcessStatus::Retired;
    if(target>1)return ProcessStatus::InvalidCall;
    return s->BeginServiceCall(WaitCall(std::move(parent),target));
}
ProcessStatus NativeFadeSystem::BeginInitialize() {
    auto s=state_->scheduler.lock();if(!s || !s->root(2))return ProcessStatus::Retired;
    ProcessCall c;c.process=s->root(2);c.kind=ProcessCallKind::Service;c.target=Initialize;return s->BeginServiceCall(c);
}
std::optional<FadeChannelSnapshot> NativeFadeSystem::ObserveChannel(std::uint32_t target) const {
    if(target>1 || !state_->Live())return {};
    FadeChannelSnapshot result;
    if(auto* row=state_->Find(state_->current[target])) {
        auto scheduler=state_->scheduler.lock();if(!scheduler->Observe(row->process))return {};
        result.process=row->process;result.color=row->color;result.goal=row->goal;
        result.active=row->active;result.active_fade_in=row->active && row->direction==0;result.blackout=row->color[3]==255;
    }
    return result;
}
std::vector<FadeProcessSnapshot> NativeFadeSystem::ObserveProcesses() const {
    std::vector<FadeProcessSnapshot> result;if(!state_->Live())return result;
    for(const auto& [key,value]:state_->records)result.push_back(value);return result;
}
std::optional<bool> NativeFadeSystem::IsBlackOutAll() const {
    auto a=ObserveChannel(0),b=ObserveChannel(1);if(!a || !b)return {};return a->blackout && b->blackout;
}
bool NativeFadeSystem::UsesScheduler(const NativeProcessScheduler& scheduler) const noexcept {
    return state_->scheduler.lock().get()==&scheduler;
}
std::unique_ptr<ProcessContinuation> NativeFadeSystem::Begin(const ProcessCall& call) {
    if(call.this_adjustment || !state_->Live())return {};
    const bool service=call.target==Initialize || call.target==CreateFade || call.target==WaitBind;
    if(service) {
        if(call.kind!=ProcessCallKind::Service)return {};
        if(call.target==CreateFade && (call.has_self || call.argument_count!=4 || call.arguments[1]>1 || call.arguments[2]>1 || call.arguments[3]>1))return {};
        if(call.target==WaitBind && (!call.has_self || call.argument_count!=1 || call.arguments[0]>1))return {};
        if(call.target==Initialize && (call.has_self || call.argument_count))return {};
    } else {
        auto* row=state_->Find(call.process);if(!row || !call.has_self || call.argument_count)return {};
        if(call.target==Tick && (row->wait_process || call.kind!=ProcessCallKind::Descriptor || call.command!=13))return {};
        else if(call.target==WaitTick && (!row->wait_process || call.kind!=ProcessCallKind::Descriptor || call.command!=13))return {};
        else if(call.target==Persistent && (row->wait_process || call.kind!=ProcessCallKind::Persistent))return {};
        else if(call.target==Destroy && (row->wait_process || call.kind!=ProcessCallKind::Destroy))return {};
        else if(call.target==WaitDestroy && (!row->wait_process || call.kind!=ProcessCallKind::Destroy))return {};
        if(std::find(Targets.begin(),Targets.end(),call.target)==Targets.end())return {};
    }
    return std::make_unique<Continuation>(state_,call);
}
}
