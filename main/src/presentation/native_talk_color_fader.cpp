#include "fates/presentation/native_talk_color_fader.hpp"
#include <bit>
#include <cmath>
#include <limits>
#include <map>
namespace fates::presentation::native {
using namespace runtime::native;
using S=TalkColorFaderStatus;
namespace {
constexpr std::uint32_t Persistent=0x4f2428,Destroy=0x4f28f8;
constexpr std::array Targets{Persistent,Destroy};
float Add(float a,float b){volatile float value=a+b;return value;}
float Sub(float a,float b){volatile float value=a-b;return value;}
float Mul(float a,float b){volatile float value=a*b;return value;}
float Div(float a,float b){volatile float value=a/b;return value;}
std::uint32_t Unsigned(float value) {
    if(std::isnan(value) || value<=0.0f)return 0;
    if(value>=4294967296.0f)return std::numeric_limits<std::uint32_t>::max();
    return static_cast<std::uint32_t>(value);
}
std::uint8_t Byte(float value){return static_cast<std::uint8_t>(Unsigned(value)&255u);}
std::uint8_t Scalar(std::uint8_t a,std::uint8_t b,float elapsed,float duration,std::uint8_t kind) {
    const float difference=static_cast<float>(std::int32_t(b)-std::int32_t(a));
    const float phase=kind==2?Sub(duration,elapsed):elapsed;
    const float numerator=kind?Mul(phase,phase):phase;
    const float denominator=kind?Mul(duration,duration):duration;
    const float delta=Div(Mul(difference,numerator),denominator);
    return Byte(kind==2?Sub(static_cast<float>(b),delta):Add(delta,static_cast<float>(a)));
}
ProcessType Type(){auto type=ProcessType::Base();type.methods[0].target=Destroy;type.methods[2].target=Persistent;return type;}
}
bool TalkColorCurveExact(TalkColorBytes origin,TalkColorBytes target,std::uint32_t elapsed,std::uint32_t duration,std::uint8_t kind,TalkColorBytes& output) noexcept {
    if(kind>4)return false;
    const auto time=static_cast<float>(std::bit_cast<std::int32_t>(elapsed)),full=static_cast<float>(std::bit_cast<std::int32_t>(duration));
    TalkColorBytes next{};
    for(unsigned index=0;index<4;++index) {
        auto a=origin[index],b=target[index];auto t=time,d=full;auto profile=kind;
        if(kind==3) {
            d=Mul(full,0.5f);
            // The executable converts the half-difference to UNSIGNED before
            // adding it and taking a byte. Descending channels retain origin as
            // midpoint; replacing this with the arithmetic average is different.
            const auto increment=Byte(Mul(static_cast<float>(std::int32_t(b)-std::int32_t(a)),0.5f));
            const auto middle=static_cast<std::uint8_t>(std::uint32_t(a)+std::uint32_t(increment));
            if(t<d){b=middle;profile=1;}else{a=middle;t=Sub(t,d);profile=2;}
        } else if(kind==4) {
            d=Mul(full,0.5f);if(d<t)t=Sub(full,t);profile=0;
        }
        next[index]=Scalar(a,b,t,d,profile);
    }
    output=next;return true;
}
struct NativeTalkColorFader::State {
    struct Row {TalkColorFaderObservation value;std::shared_ptr<TalkColorOwner> owner;};
    std::weak_ptr<NativeProcessScheduler> scheduler;std::shared_ptr<ObjectHandleRegistry> objects;
    std::map<std::uint64_t,Row> rows;
    S Mutable(ProcessAccess* access) const {
        const auto s=scheduler.lock();if(!s || !s->root(2))return S::Retired;
        if(access)return access->BelongsTo(*s)?S::Ready:S::MismatchedDomain;
        return s->busy()?S::Busy:S::Ready;
    }
    Row* Get(ProcessHandle process){if(!process)return nullptr;const auto i=rows.find(process->serial);return i!=rows.end() && i->second.value.process==process?&i->second:nullptr;}
};
struct NativeTalkColorFader::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;ProcessCall call;ObjectIdentity identity;TalkColorBytes computed{};unsigned stage{},channel{};
    ProcessCallbackStep Step(ProcessAccess& access) override {
        if(state->Mutable(&access)!=S::Ready)return ProcessCallbackStep::Blocked();
        auto* owned=state->Get(call.process);if(!owned)return ProcessCallbackStep::Blocked();auto& row=owned->value;
        if(call.target==Destroy){state->rows.erase(call.process->serial);return ProcessCallbackStep::Return();}
        if(stage==0) {
            identity=state->objects->Get(row.target);
            if(!identity){stage=4;++row.deletion_requests;return ProcessCallbackStep::Delete(call.process);}
            if(!row.origin || !TalkColorCurveExact(*row.origin,row.destination,row.elapsed,row.duration,row.curve,computed))return ProcessCallbackStep::Blocked();
            stage=1;
        }
        for(;;) {
            if(stage==1 || stage==2) {
                for(;channel<4;++channel) {
                    if(!(row.channels&(1u<<channel)))continue;
                    if(state->objects->Get(row.target)!=identity || !owned->owner->WriteColorChannel(identity,static_cast<std::uint8_t>(channel),computed[channel],access))return ProcessCallbackStep::Blocked();
                }
                channel=0;
                if(stage==1) {
                    row.elapsed+=access.frame_delta();stage=3;
                    if(std::bit_cast<std::int32_t>(row.elapsed)>std::bit_cast<std::int32_t>(row.duration)){computed=row.destination;stage=2;continue;}
                } else {stage=4;++row.deletion_requests;return ProcessCallbackStep::Delete(call.process);}
            }
            return ProcessCallbackStep::Return();
        }
    }
};
NativeTalkColorFader::NativeTalkColorFader(std::shared_ptr<State> state):state_(std::move(state)){}
NativeTalkColorFader::~NativeTalkColorFader()=default;
S NativeTalkColorFader::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<ObjectHandleRegistry> objects,std::shared_ptr<NativeTalkColorFader>& output) {
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !objects)return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->objects=std::move(objects);
    auto owner=std::shared_ptr<NativeTalkColorFader>(new NativeTalkColorFader(state));
    if(!registry->Register(Targets,owner))return S::DuplicateBinding;
    output=std::move(owner);return S::Ready;
}
S NativeTalkColorFader::Bind(ProcessHandle parent,std::shared_ptr<TalkColorOwner> owner,ObjectHandle handle,TalkColorBytes target,
    std::int32_t milliseconds,std::uint32_t curve,std::uint32_t channels,bool blocking,ProcessHandle& output,ProcessAccess* access) {
    if(auto result=state_->Mutable(access);result!=S::Ready)return result;
    const auto s=state_->scheduler.lock();if(!owner || !owner->UsesScheduler(*s) || !owner->UsesObjectRegistry(*state_->objects))return S::MismatchedDomain;
    const auto p=s->Observe(parent);if(!p || !p->linked || (p->flags&1u))return S::InvalidParent;
    if(handle.Index()>=ObjectHandleRegistry::Capacity)return S::InvalidHandle;
    const auto identity=state_->objects->Get(handle);std::optional<TalkColorBytes> origin;
    if(identity){origin=owner->ReadColor(identity);if(!origin)return S::Unavailable;}
    ProcessHandle child;
    const auto status=access?access->Create(parent,ProcessProgram::Default(),"ProcColorFader",blocking,Type(),child):
        s->Create(parent,ProcessProgram::Default(),"ProcColorFader",blocking,Type(),child);
    if(status!=ProcessStatus::Ready)return S::InvalidParent;
    state_->rows.emplace(child->serial,State::Row{{child,handle,origin,target,ProcessMillisecondsToFrames(milliseconds),0,
        static_cast<std::uint8_t>(curve),static_cast<std::uint8_t>(channels),0},std::move(owner)});
    output=std::move(child);return S::Ready;
}
S NativeTalkColorFader::RestoreClock(ProcessHandle process,std::uint32_t elapsed,std::uint32_t duration) {
    if(auto result=state_->Mutable(nullptr);result!=S::Ready)return result;
    auto* row=state_->Get(process);if(!row || !state_->scheduler.lock()->Observe(process))return S::InvalidHandle;
    row->value.elapsed=elapsed;row->value.duration=duration;return S::Ready;
}
std::optional<TalkColorFaderObservation> NativeTalkColorFader::Observe(ProcessHandle process) const {
    const auto s=state_->scheduler.lock();if(!s || !s->root(2) || !s->Observe(process))return {};
    const auto* row=state_->Get(process);return row?std::optional(row->value):std::nullopt;
}
std::vector<ProcessHandle> NativeTalkColorFader::Processes() const {
    std::vector<ProcessHandle> rows;const auto s=state_->scheduler.lock();if(!s || !s->root(2))return rows;
    for(const auto& [id,row]:state_->rows){(void)id;rows.push_back(row.value.process);}return rows;
}
bool NativeTalkColorFader::UsesScheduler(const NativeProcessScheduler& s) const noexcept{return state_->scheduler.lock().get()==&s;}
std::unique_ptr<ProcessContinuation> NativeTalkColorFader::Begin(const ProcessCall& call) {
    const auto s=state_->scheduler.lock();if(!s || !s->root(2) || !state_->Get(call.process) || !call.has_self || call.argument_count || call.this_adjustment)return {};
    if(call.target==Destroy){if(call.kind!=ProcessCallKind::Destroy)return {};}
    else if(call.target!=Persistent || call.kind!=ProcessCallKind::Persistent)return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->call=call;return next;
}
}

namespace fates::presentation::native {bool NativeTalkColorFader::UsesObjectRegistry(const runtime::native::ObjectHandleRegistry& objects) const noexcept{return state_->objects.get()==&objects;}}
