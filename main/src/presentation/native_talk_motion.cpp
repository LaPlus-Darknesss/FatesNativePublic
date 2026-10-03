#include "fates/presentation/native_talk_motion.hpp"
#include <bit>
#include <cmath>
#include <map>
namespace fates::presentation::native {
using namespace runtime::native;
using S=TalkMotionStatus;
namespace {
constexpr std::uint32_t Persistent=0x4f1fdc,Destroy=0x4f231c,Scroll=0x33f4ac,ScrollDestroy=0x33f580,Page=0x18fc10;
constexpr std::array Targets{Persistent,Destroy,Scroll,ScrollDestroy,Page};
float Add(float a,float b){volatile float v=a+b;return v;}
float Sub(float a,float b){volatile float v=a-b;return v;}
float Mul(float a,float b){volatile float v=a*b;return v;}
float Div(float a,float b){volatile float v=a/b;return v;}
bool Vector(std::array<float,3> a,std::array<float,3> b,float t,float d,std::uint8_t kind,TalkVectorBits& output) {
    if(kind>4 || !std::isfinite(t) || !std::isfinite(d))return false;
    if(kind==3 || kind==4) {
        const auto half=Mul(d,0.5f);std::array<float,3> mid{};
        for(unsigned i=0;i<3;++i)mid[i]=Add(a[i],Mul(Sub(b[i],a[i]),0.5f));
        if(t<half){b=mid;kind=kind==3?1:2;}
        else{a=mid;t=Sub(t,half);kind=kind==3?2:1;}
        d=half;
    }
    const auto power=kind==2?Sub(d,t):t;
    const auto numerator=kind?Mul(power,power):power,denominator=kind?Mul(d,d):d;
    const auto reciprocal=Div(1.0f,denominator);
    TalkVectorBits next{};
    for(unsigned i=0;i<3;++i) {
        if(!std::isfinite(a[i]) || !std::isfinite(b[i]))return false;
        const auto value=Mul(Mul(Sub(b[i],a[i]),numerator),reciprocal);
        const auto result=kind==2?Sub(b[i],value):Add(a[i],value);
        if(!std::isfinite(value)||!std::isfinite(result))return false;
        next[i]=std::bit_cast<std::uint32_t>(result);
    }
    output=next;return true;
}
ProcessType Type(bool scroll){auto type=ProcessType::Base();type.methods[0].target=scroll?ScrollDestroy:Destroy;type.methods[2].target=scroll?Scroll:Persistent;return type;}
}
bool TalkVectorCurveExact(TalkVectorBits origin,TalkVectorBits target,std::uint32_t elapsed,std::uint32_t duration,std::uint8_t curve,TalkVectorBits& output) noexcept {
    std::array<float,3> a{},b{};for(unsigned i=0;i<3;++i){a[i]=std::bit_cast<float>(origin[i]);b[i]=std::bit_cast<float>(target[i]);}
    return Vector(a,b,static_cast<float>(std::bit_cast<std::int32_t>(elapsed)),static_cast<float>(std::bit_cast<std::int32_t>(duration)),curve,output);
}
struct NativeTalkMotion::State {
    struct Row {TalkMotionObservation value;std::shared_ptr<TalkPositionOwner> owner;};
    std::weak_ptr<NativeProcessScheduler> scheduler;std::shared_ptr<ObjectHandleRegistry> objects;
    std::shared_ptr<NativeTalkWindow> windows;std::map<std::uint64_t,Row> rows;
    S Mutable(ProcessAccess* access) const {
        const auto s=scheduler.lock();if(!s || !s->root(2))return S::Retired;
        if(access)return access->BelongsTo(*s)?S::Ready:S::MismatchedDomain;
        return s->busy()?S::Busy:S::Ready;
    }
    Row* Get(ProcessHandle process) {
        if(!process)return nullptr;
        const auto i=rows.find(process->serial);
        return i!=rows.end() && i->second.value.process==process?&i->second:nullptr;
    }
    S Bind(ProcessAccess* access,ProcessHandle parent,std::shared_ptr<TalkPositionOwner> owner,ObjectHandle handle,TalkVectorBits target,std::int32_t ms,std::uint32_t curve,bool blocking,std::optional<TalkWindowView> string,std::uint8_t fade,ProcessHandle& out) {
        if(const auto status=Mutable(access);status!=S::Ready)return status;
        const auto s=scheduler.lock();if(!owner || !owner->UsesScheduler(*s) || !owner->UsesObjectRegistry(*objects))return S::MismatchedDomain;
        const auto p=s->Observe(parent);if(!p || !p->linked || p->flags&1u)return S::InvalidParent;
        if(handle.Index()>=ObjectHandleRegistry::Capacity)return S::InvalidHandle;
        const auto identity=objects->Get(handle);std::optional<TalkVectorBits> origin;
        if(identity){origin=owner->ReadPosition(identity);if(!origin)return S::Unavailable;}
        ProcessHandle process;
        const auto status=access?access->Create(parent,ProcessProgram::Default(),string?"TalkStringScroll":"ProcCarrier",blocking,Type(bool(string)),process):
            s->Create(parent,ProcessProgram::Default(),string?"TalkStringScroll":"ProcCarrier",blocking,Type(bool(string)),process);
        if(status!=ProcessStatus::Ready)return S::InvalidParent;
        rows.emplace(process->serial,Row{{process,handle,origin,target,ProcessMillisecondsToFrames(ms),0,static_cast<std::uint8_t>(curve),fade,string,0},std::move(owner)});
        out=std::move(process);return S::Ready;
    }
};
struct NativeTalkMotion::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;ProcessCall call;TalkWindowHandle window;
    unsigned stage{},index{};std::uint32_t distance{};ObjectIdentity identity;TalkVectorBits computed{};
    ProcessCallbackStep PageStep(ProcessAccess& access) {
        if(stage==0){if(state->windows->StartPageState(window,access)!=TalkWindowStatus::Ready)return ProcessCallbackStep::Blocked();stage=1;}
        if(stage==1){const auto view=state->windows->Observe(window);if(!view || !view->text_layout)return ProcessCallbackStep::Blocked();distance=view->line*std::uint32_t((*view->text_layout)[2]);stage=2;}
        for(;index<8;++index) {
            const auto string=state->windows->ObserveString({window,static_cast<std::uint8_t>(index)});if(!string || !string->known.test(0))return ProcessCallbackStep::Blocked();
            if(!string->words[0])continue;
            auto position=string->position;
            const auto y=Sub(std::bit_cast<float>(position[1]),static_cast<float>(std::bit_cast<std::int32_t>(distance)));
            if(!std::isfinite(y))return ProcessCallbackStep::Blocked();
            position[1]=std::bit_cast<std::uint32_t>(y);
            ProcessHandle child;
            if(state->Bind(&access,call.process,state->windows,string->movable.handle,position,140,2,true,TalkWindowView{window,static_cast<std::uint8_t>(index)},1,child)!=S::Ready)return ProcessCallbackStep::Blocked();
        }
        if(state->windows->FinishPageState(window,access)!=TalkWindowStatus::Ready)return ProcessCallbackStep::Blocked();
        return ProcessCallbackStep::Return();
    }
    ProcessCallbackStep Step(ProcessAccess& access) override {
        if(state->Mutable(&access)!=S::Ready)return ProcessCallbackStep::Blocked();
        if(call.target==Page)return PageStep(access);
        auto* owned=state->Get(call.process);if(!owned)return ProcessCallbackStep::Blocked();auto& row=owned->value;
        if(call.target==Destroy || call.target==ScrollDestroy){state->rows.erase(call.process->serial);return ProcessCallbackStep::Return();}
        if(stage==0) {
            identity=state->objects->Get(row.target);
            if(!identity){stage=4;++row.deletion_requests;return ProcessCallbackStep::Delete(call.process);}
            if(!row.origin)return ProcessCallbackStep::Blocked();
            if(!TalkVectorCurveExact(*row.origin,row.destination,row.elapsed,row.duration,row.curve,computed)) {
                // Newly reached original zero-duration effect carrier: degree2
                // deceleration at elapsed0 computes (finite*0)*(1/0). VFP writes
                // its generated default NaN before the ordinary time advance and
                // exact-target/delete branch. Do not skip that first write.
                bool finite=true;for(unsigned i=0;i<3;++i)finite=finite && std::isfinite(std::bit_cast<float>((*row.origin)[i])) && std::isfinite(std::bit_cast<float>(row.destination[i]));
                if(row.duration!=0 || row.elapsed!=0 || row.curve!=2 || !finite)return ProcessCallbackStep::Blocked();
                computed={0x7fc00000u,0x7fc00000u,0x7fc00000u};
            }
            stage=1;
        }
        if(stage==1) {
            if(state->objects->Get(row.target)!=identity || !owned->owner->WritePosition(identity,computed,access))return ProcessCallbackStep::Blocked();
            row.elapsed+=access.frame_delta();stage=2;
        }
        if(stage==2) {
            if(std::bit_cast<std::int32_t>(row.elapsed)>std::bit_cast<std::int32_t>(row.duration)) {
                if(state->objects->Get(row.target)!=identity || !owned->owner->WritePosition(identity,row.destination,access))return ProcessCallbackStep::Blocked();
                stage=4;++row.deletion_requests;return ProcessCallbackStep::Delete(call.process);
            }
            stage=4;
        }
        if(row.string && state->windows->ScrollString(*row.string,row.elapsed,row.duration,row.fade,access)!=TalkWindowStatus::Ready)return ProcessCallbackStep::Blocked();
        return ProcessCallbackStep::Return();
    }
};
NativeTalkMotion::NativeTalkMotion(std::shared_ptr<State> state):state_(std::move(state)){}
NativeTalkMotion::~NativeTalkMotion()=default;
S NativeTalkMotion::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,std::shared_ptr<ObjectHandleRegistry> objects,std::shared_ptr<NativeTalkWindow> windows,std::shared_ptr<NativeTalkMotion>& out) {
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !objects || !windows || !windows->UsesScheduler(*scheduler) || !windows->UsesObjectRegistry(*objects))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->objects=std::move(objects);state->windows=std::move(windows);
    auto next=std::shared_ptr<NativeTalkMotion>(new NativeTalkMotion(state));if(!registry->Register(Targets,next))return S::DuplicateBinding;
    out=std::move(next);return S::Ready;
}
S NativeTalkMotion::BindCarrier(ProcessHandle parent,std::shared_ptr<TalkPositionOwner> owner,ObjectHandle handle,TalkVectorBits target,std::int32_t ms,std::uint32_t curve,bool blocking,ProcessHandle& out,ProcessAccess* access) {
    return state_->Bind(access,std::move(parent),std::move(owner),handle,target,ms,curve,blocking,{},0,out);
}
S NativeTalkMotion::BindScroll(ProcessHandle parent,TalkWindowView view,TalkVectorBits target,std::int32_t ms,std::uint32_t curve,std::uint8_t fade,ProcessHandle& out,ProcessAccess* access) {
    if(auto status=state_->Mutable(access);status!=S::Ready)return status;
    const auto string=state_->windows->ObserveString(view);if(!string)return S::InvalidHandle;
    return state_->Bind(access,std::move(parent),state_->windows,string->movable.handle,target,ms,curve,true,view,fade,out);
}
std::optional<ProcessCall> NativeTalkMotion::NextPageCall(ProcessHandle parent,TalkWindowHandle window) const {
    if(!state_->windows->Observe(window))return {};
    ProcessCall call;call.process=std::move(parent);call.kind=ProcessCallKind::Service;call.target=Page;call.argument_count=1;call.arguments[0]=window->serial;return call;
}
S NativeTalkMotion::RestoreClock(ProcessHandle process,std::uint32_t elapsed,std::uint32_t duration) {
    if(auto status=state_->Mutable(nullptr);status!=S::Ready)return status;
    auto* row=state_->Get(process);if(!row || !state_->scheduler.lock()->Observe(process))return S::InvalidHandle;
    row->value.elapsed=elapsed;row->value.duration=duration;return S::Ready;
}
std::optional<TalkMotionObservation> NativeTalkMotion::Observe(ProcessHandle process) const {
    const auto s=state_->scheduler.lock();if(!s || !s->root(2) || !s->Observe(process))return {};
    const auto* row=state_->Get(process);return row?std::optional(row->value):std::nullopt;
}
std::vector<ProcessHandle> NativeTalkMotion::Processes() const {
    std::vector<ProcessHandle> out;const auto s=state_->scheduler.lock();if(!s || !s->root(2))return out;
    for(const auto& [id,row]:state_->rows){(void)id;out.push_back(row.value.process);}return out;
}
bool NativeTalkMotion::UsesScheduler(const NativeProcessScheduler& s) const noexcept{return state_->scheduler.lock().get()==&s;}
std::unique_ptr<ProcessContinuation> NativeTalkMotion::Begin(const ProcessCall& call) {
    const auto s=state_->scheduler.lock();if(!s || !s->root(2) || !call.process || call.this_adjustment)return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->call=call;
    if(call.target==Page) {
        if(call.kind!=ProcessCallKind::Service || call.has_self || call.argument_count!=1)return {};
        // Only the existing window owner's identity is accepted, never an invented raw pointer.
        for(const auto& id:state_->windows->Handles())if(id->serial==call.arguments[0]){next->window=id;return next;}
        return {};
    }
    const auto* row=state_->Get(call.process);if(!row || !call.has_self || call.argument_count)return {};
    const bool scroll=bool(row->value.string);
    if(call.target==(scroll?ScrollDestroy:Destroy)){if(call.kind!=ProcessCallKind::Destroy)return {};}
    else if(call.target!=(scroll?Scroll:Persistent) || call.kind!=ProcessCallKind::Persistent)return {};
    return next;
}
}

namespace fates::presentation::native {bool NativeTalkMotion::UsesWindow(const NativeTalkWindow& windows) const noexcept{return state_->windows.get()==&windows;}}
