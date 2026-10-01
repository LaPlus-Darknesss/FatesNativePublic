#include "fates/event/native_band.hpp"
#include <cmath>
#include <limits>
#include <map>

namespace fates::event::native {
using namespace runtime::native;
using S=BandStatus;
namespace {
constexpr std::uint32_t Tick=0x4324a4,Persistent=0x4323d8,Destroy=0x432514;
constexpr std::array Targets{Tick,Persistent,Destroy};
ProcessType Type(){auto t=ProcessType::Base();t.methods[0].target=Destroy;t.methods[1].target=Tick;t.methods[2].target=Persistent;return t;}
float Mul(float a,float b) noexcept{return a*b;}
float Add(float a,float b) noexcept{return a+b;}
bool Valid(const BandMotionState& s) {
    return s.time.kind==2 && s.time.subkind==2 && std::isfinite(s.time.elapsed) && std::isfinite(s.time.duration) &&
        std::isfinite(s.origin) && std::isfinite(s.target) && std::isfinite(s.current);
}
std::uint8_t Alpha(float value) {
    const float n=Mul(value,255.0f);
    // ARM VCVT.U32.F32 saturates, then STRB retains only the low byte.
    if(n<=0.0f)return 0;
    if(n>=4294967296.0f)return 255;
    return static_cast<std::uint8_t>(static_cast<std::uint32_t>(n)&255u);
}
}
struct NativeBand::State {
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<presentation::native::NativeFadeSystem> fade;
    std::shared_ptr<BandDrawSink> sink;
    std::map<std::uint64_t,BandSnapshot> rows;
    S Mutable(ProcessAccess* a) const {
        auto s=scheduler.lock();if(!s || !s->root(2))return S::Retired;
        if(a)return a->BelongsTo(*s)?S::Ready:S::MismatchedDomain;
        return s->busy()?S::Busy:S::Ready;
    }
    S Admission(ProcessAccess* a,ProcessHandle parent,ProcessHandle& found) const {
        if(auto status=Mutable(a);status!=S::Ready)return status;
        auto s=scheduler.lock();found=s->FindByName("ProcBand");
        if(found) {
            const auto row=rows.find(found->serial);
            if(row==rows.end() || row->second.process!=found || !s->HasType(found,Type()))return S::UnownedNamedProcess;
        } else {
            const auto p=s->Observe(parent);
            if(!p || !p->linked || (p->flags&1))return S::InvalidParent;
        }
        if(!fade->ObserveChannel(0))return S::UnknownFade;
        return S::Ready;
    }
    S Open(ProcessAccess* a,ProcessHandle parent,ProcessHandle& out) {
        ProcessHandle found;if(auto status=Admission(a,parent,found);status!=S::Ready)return status;
        // CanOpen and Open cannot interleave a scheduler operation. Preflight
        // makes refusal atomic without changing successful original ordering.
        const bool blackout=fade->ObserveChannel(0)->blackout;
        if(!found) {
            const auto status=a?a->Create(parent,ProcessProgram::Default(),"ProcBand",false,Type(),found):
                scheduler.lock()->Create(parent,ProcessProgram::Default(),"ProcBand",false,Type(),found);
            if(status!=ProcessStatus::Ready)return S::InvalidParent;
            rows.emplace(found->serial,BandSnapshot{found,{}});
        }
        auto& n=rows.at(found->serial).motion;
        if(blackout){n.origin=n.target=n.current=1.0f;n.time.duration=n.time.elapsed=0.0f;}
        else if(n.target!=1.0f){n.origin=n.current;n.target=1.0f;SetMoveTimeExact(n.time,500);}
        out=found;return S::Ready;
    }
};
struct NativeBand::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;ProcessCall call;bool deleting{};
    ProcessCallbackStep Step(ProcessAccess& access) override {
        if(state->Mutable(&access)!=S::Ready)return ProcessCallbackStep::Blocked();
        if(deleting)return ProcessCallbackStep::Return();
        const auto row=state->rows.find(call.process->serial);
        if(row==state->rows.end() || row->second.process!=call.process)return ProcessCallbackStep::Blocked();
        if(call.target==Destroy){state->rows.erase(row);return ProcessCallbackStep::Return();}
        auto& motion=row->second.motion;
        if(call.target==Persistent) {
            if(motion.current==0.0f)return ProcessCallbackStep::Return();
            const auto height=Mul(motion.current,20.0f);
            if(!std::isfinite(height))return ProcessCallbackStep::Blocked();
            state->sink->Draw({call.process,{{{0,0,400,height},{0,240.0f-height,400,height}}},{0,0,0,Alpha(motion.current)}});
            return ProcessCallbackStep::Return();
        }
        auto n=motion;bool evaluated{};
        // ProcBand passes float(delta), and MoveTime::Evaluate multiplies that
        // by the SAME global delta. Do not turn this into a single delta step.
        if(!EvaluateMoveTimeExact(n.time,static_cast<float>(access.frame_delta()),access.frame_delta(),evaluated))return ProcessCallbackStep::Blocked();
        if(evaluated) {
            float rate;if(!MoveTimeRateExact(n.time,rate))return ProcessCallbackStep::Blocked();
            const float difference=n.target-n.origin;n.current=Add(n.origin,Mul(difference,rate));
            if(!Valid(n))return ProcessCallbackStep::Blocked();motion=n;
        } else if(n.current==0.0f){deleting=true;return ProcessCallbackStep::Delete(call.process);}
        return ProcessCallbackStep::Return();
    }
};
NativeBand::NativeBand(std::shared_ptr<State> s):state_(std::move(s)){}
S NativeBand::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<presentation::native::NativeFadeSystem> fade,std::shared_ptr<BandDrawSink> sink,std::shared_ptr<NativeBand>& out) {
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !fade || !fade->UsesScheduler(*scheduler))return S::MismatchedDomain;
    if(!sink)return S::MissingSink;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->fade=std::move(fade);state->sink=std::move(sink);
    auto next=std::shared_ptr<NativeBand>(new NativeBand(state));
    if(!registry->Register(Targets,next))return S::DuplicateBinding;out=std::move(next);return S::Ready;
}
S NativeBand::Open(ProcessHandle parent,ProcessHandle& out){return state_->Open(nullptr,std::move(parent),out);}
S NativeBand::Open(ProcessAccess& a,ProcessHandle parent,ProcessHandle& out){return state_->Open(&a,std::move(parent),out);}
S NativeBand::CanOpen(ProcessAccess& a,ProcessHandle parent) const {ProcessHandle found;return state_->Admission(&a,std::move(parent),found);}
S NativeBand::RestoreCarriedMotion(ProcessHandle p,const BandMotionState& motion) {
    if(auto status=state_->Mutable(nullptr);status!=S::Ready)return status;
    const auto row=p?state_->rows.find(p->serial):state_->rows.end();const auto view=state_->scheduler.lock()->Observe(p);
    if(row==state_->rows.end() || row->second.process!=p || !view || !view->linked || (view->flags&1))return S::InvalidHandle;
    if(!Valid(motion))return S::InvalidState;row->second.motion=motion;return S::Ready;
}
std::optional<BandSnapshot> NativeBand::Observe(ProcessHandle p) const {
    auto s=state_->scheduler.lock();if(!s || !s->Observe(p))return {};
    const auto row=p?state_->rows.find(p->serial):state_->rows.end();
    return row!=state_->rows.end() && row->second.process==p?std::optional<BandSnapshot>(row->second):std::nullopt;
}
std::vector<ProcessHandle> NativeBand::Processes() const {
    std::vector<ProcessHandle> out;auto s=state_->scheduler.lock();if(!s || !s->root(2))return out;
    for(const auto& [serial,row]:state_->rows){(void)serial;out.push_back(row.process);}return out;
}
bool NativeBand::UsesScheduler(const NativeProcessScheduler& s) const noexcept{return state_->scheduler.lock().get()==&s;}
std::unique_ptr<ProcessContinuation> NativeBand::Begin(const ProcessCall& call) {
    if(!call.process || !call.has_self || call.this_adjustment || call.argument_count)return {};
    const auto row=state_->rows.find(call.process->serial);if(row==state_->rows.end() || row->second.process!=call.process)return {};
    if(call.target==Destroy){if(call.kind!=ProcessCallKind::Destroy)return {};}
    else if(call.target==Persistent){if(call.kind!=ProcessCallKind::Persistent)return {};}
    else if(call.target!=Tick || call.kind!=ProcessCallKind::Descriptor || call.command!=13)return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->call=call;return next;
}
}
