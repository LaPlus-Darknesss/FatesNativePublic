#include "fates/presentation/native_talk_drawing_statics.hpp"
#include <limits>
#include <map>

namespace fates::presentation::native {
using namespace runtime::native;
using S=TalkDrawingStaticStatus;
namespace {
constexpr std::uint32_t Initialize=0x596734,Offset=0x1f0d00;
constexpr std::array<TalkColorBytes,12> Palette{{
    {255,255,255,255},{0,0,0,0},{255,255,255,255},{0,0,0,255},
    {255,0,0,255},{0,255,0,255},{0,0,255,255},{255,255,0,255},
    {255,0,255,255},{0,255,255,255},{128,128,128,255},{64,64,64,255}
}};
// Binary32 literals read by startup at59692C..59693C. All other words in the
// eleven vectors are written as positive zero by the actual initializer.
constexpr std::array<std::uint32_t,5> ShoutY{0x409c72b0,0xbf639581,0xc0800000,0xc03da1cb,0xbf84bc6a};
}
struct NativeTalkDrawingStatics::State {
    struct Request {TalkDrawingOffsetObservation value;std::int32_t index{};};
    std::weak_ptr<NativeProcessScheduler> scheduler;TalkDrawingStaticStorage storage;
    std::map<std::uint32_t,std::shared_ptr<Request>> requests;std::uint32_t serial{};
    bool published{};std::uint64_t guard_calls{};
    bool Live() const {const auto s=scheduler.lock();return s && s->root(2);}
    S Mutable(ProcessAccess* access) const {
        const auto s=scheduler.lock();if(!s || !s->root(2))return S::Retired;
        if(access)return access->BelongsTo(*s)?S::Ready:S::MismatchedDomain;
        return s->busy()?S::Busy:S::Ready;
    }
    std::shared_ptr<Request> Get(TalkDrawingOffsetRequest id) const {
        if(!id)return {};const auto it=requests.find(id->serial);
        return it!=requests.end() && it->second->value.identity==id?it->second:nullptr;
    }
    S EnsureZero() {
        if(!storage.zero_guard)return S::Unavailable;
        if(!(*storage.zero_guard&1u)) {
            ++guard_calls; // Exact reached simple guard call, including even nonzero words.
            if(*storage.zero_guard==0){storage.zero_guard=1u;for(auto& word:storage.zero_vector)word=0u;}
        }
        return S::Ready;
    }
};
struct NativeTalkDrawingStatics::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;std::shared_ptr<State::Request> request;bool initialized{},guard_checked{};
    ~Continuation() override {
        if(request && !request->value.completed){request->value.active=false;request->value.cancelled=true;request->value.status=S::Cancelled;}
    }
    ProcessCallbackStep Step(ProcessAccess& access) override {
        if(const auto status=state->Mutable(&access);status!=S::Ready){if(request)request->value.status=status;return ProcessCallbackStep::Blocked();}
        if(!request) {
            if(!initialized){
                for(std::size_t i=0;i<Palette.size();++i)state->storage.palette[i]=Palette[i];
                for(auto& row:state->storage.shout_offsets)row=TalkVectorBits{};
                for(std::size_t i=0;i<ShoutY.size();++i)(*state->storage.shout_offsets[i+5])[1]=ShoutY[i];
                state->published=true;initialized=true;
            }
            return ProcessCallbackStep::Return();
        }
        if(state->Get(request->value.identity)!=request){request->value.status=S::Cancelled;return ProcessCallbackStep::Blocked();}
        std::optional<TalkVectorBits> result;
        if(request->index>=0 && request->index<11)result=state->storage.shout_offsets[static_cast<std::size_t>(request->index)];
        else {
            if(!guard_checked){const auto status=state->EnsureZero();if(status!=S::Ready){request->value.status=status;return ProcessCallbackStep::Blocked();}guard_checked=true;}
            const auto& values=state->storage.zero_vector;
            if(values[0] && values[1] && values[2])result=TalkVectorBits{*values[0],*values[1],*values[2]};
        }
        if(!result){request->value.status=S::Unavailable;return ProcessCallbackStep::Blocked();}
        request->value.result=result;request->value.active=false;request->value.completed=true;request->value.status=S::Ready;
        return ProcessCallbackStep::Return();
    }
};
NativeTalkDrawingStatics::NativeTalkDrawingStatics(std::shared_ptr<State> state):state_(std::move(state)){}
NativeTalkDrawingStatics::~NativeTalkDrawingStatics()=default;
S NativeTalkDrawingStatics::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,std::shared_ptr<NativeTalkDrawingStatics>& out) {
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;auto next=std::shared_ptr<NativeTalkDrawingStatics>(new NativeTalkDrawingStatics(state));
    const std::array targets{Initialize,Offset};if(!registry->Register(targets,next))return S::DuplicateBinding;
    out=std::move(next);return S::Ready;
}
S NativeTalkDrawingStatics::ConstructFreshStorage(ProcessAccess* access) {
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    if(state_->published)return S::InvalidState;
    state_->storage.zero_guard=0u;for(auto& word:state_->storage.zero_vector)word=0u;
    for(auto& row:state_->storage.palette)row=TalkColorBytes{};
    for(auto& row:state_->storage.shout_offsets)row=TalkVectorBits{};
    state_->published=true;return S::Ready;
}
S NativeTalkDrawingStatics::Restore(TalkDrawingStaticStorage storage,ProcessAccess* access) {
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    state_->storage=std::move(storage);state_->published=true;return S::Ready;
}
std::optional<TalkDrawingStaticStorage> NativeTalkDrawingStatics::ObserveStorage() const{return state_->Live()?std::optional{state_->storage}:std::nullopt;}
ProcessCall NativeTalkDrawingStatics::InitializeCall(ProcessHandle process) {
    ProcessCall call;call.process=std::move(process);call.kind=ProcessCallKind::Service;call.target=Initialize;return call;
}
S NativeTalkDrawingStatics::PrepareOffset(std::int32_t index,TalkDrawingOffsetRequest& out,ProcessAccess* access) {
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    if(state_->serial==std::numeric_limits<std::uint32_t>::max())return S::IdentityExhausted;
    auto row=std::make_shared<State::Request>();row->index=index;row->value.identity=std::make_shared<TalkDrawingOffsetIdentity>(++state_->serial);
    state_->requests.emplace(state_->serial,row);out=row->value.identity;return S::Ready;
}
std::optional<ProcessCall> NativeTalkDrawingStatics::OffsetCall(ProcessHandle process,TalkDrawingOffsetRequest id) const {
    const auto row=state_->Get(id);if(!state_->Live() || !row || row->value.active || row->value.completed || row->value.cancelled)return {};
    auto call=InitializeCall(std::move(process));call.target=Offset;call.argument_count=1;call.arguments[0]=id->serial;return call;
}
std::optional<TalkDrawingOffsetObservation> NativeTalkDrawingStatics::Observe(TalkDrawingOffsetRequest id) const {
    const auto row=state_->Get(id);return state_->Live() && row?std::optional{row->value}:std::nullopt;
}
S NativeTalkDrawingStatics::Release(TalkDrawingOffsetRequest id,ProcessAccess* access) {
    if(const auto status=state_->Mutable(access);status!=S::Ready)return status;
    const auto row=state_->Get(id);if(!row)return S::InvalidRequest;if(row->value.active)return S::Busy;
    state_->requests.erase(id->serial);return S::Ready;
}
S NativeTalkDrawingStatics::EnsureZeroVector(ProcessAccess& access) {
    if(const auto status=state_->Mutable(&access);status!=S::Ready)return status;return state_->EnsureZero();
}
std::optional<std::uint32_t> NativeTalkDrawingStatics::ReadZeroWord(std::size_t component) const {
    return state_->Live() && component<3?state_->storage.zero_vector[component]:std::nullopt;
}
std::optional<TalkColorBytes> NativeTalkDrawingStatics::ReadPalette(std::size_t index) const {
    return state_->Live() && index<state_->storage.palette.size()?state_->storage.palette[index]:std::nullopt;
}
std::uint64_t NativeTalkDrawingStatics::guard_calls() const noexcept{return state_->guard_calls;}
bool NativeTalkDrawingStatics::UsesScheduler(const NativeProcessScheduler& scheduler) const noexcept{return state_->scheduler.lock().get()==&scheduler;}
void NativeTalkDrawingStatics::ForgetOffset(TalkDrawingOffsetRequest id){if(state_->Get(id))state_->requests.erase(id->serial);}
std::unique_ptr<ProcessContinuation> NativeTalkDrawingStatics::Begin(const ProcessCall& call) {
    if(!state_->Live() || !call.process || call.kind!=ProcessCallKind::Service || call.has_self || call.this_adjustment)return {};
    auto next=std::make_unique<Continuation>();next->state=state_;
    if(call.target==Initialize){if(call.argument_count)return {};return next;}
    if(call.target!=Offset || call.argument_count!=1)return {};
    const auto it=state_->requests.find(call.arguments[0]);if(it==state_->requests.end())return {};
    next->request=it->second;auto& value=next->request->value;if(value.active || value.completed || value.cancelled)return {};
    value.active=true;return next;
}
}
