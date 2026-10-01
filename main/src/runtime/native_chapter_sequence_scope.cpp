#include "fates/runtime/native_chapter_sequence_scope.hpp"

namespace fates::runtime::native {
using S=ChapterScopeStatus;
namespace {constexpr std::uint32_t Persistent=0x1da1d8;constexpr std::array Targets{Persistent};}
struct NativeChapterSequenceScope::State {
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<NativeGameSkip> skip;
    std::optional<ChapterScopeSnapshot> carried;
    S Admission() const {
        const auto s=scheduler.lock();if(!s || !s->root(2))return S::Retired;
        if(s->busy())return S::Busy;
        return carried?S::AlreadyBound:S::Ready;
    }
};
struct NativeChapterSequenceScope::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;ProcessHandle process;bool invoked{};
    ProcessCallbackStep Step(ProcessAccess& access) override {
        const auto s=state->scheduler.lock();
        if(!s || !access.BelongsTo(*s) || !access.Observe(process))return ProcessCallbackStep::Blocked();
        if(invoked)return ProcessCallbackStep::Return();
        const auto current=state->skip->Current();if(!current)return ProcessCallbackStep::Blocked();
        if(!*current)return ProcessCallbackStep::Return();
        auto call=state->skip->Call(process,*current,GameSkipOperation::Tick);
        if(!call)return ProcessCallbackStep::Blocked();invoked=true;return ProcessCallbackStep::Call(*call);
    }
};
NativeChapterSequenceScope::NativeChapterSequenceScope(std::shared_ptr<State> s):state_(std::move(s)){}
S NativeChapterSequenceScope::Create(std::shared_ptr<NativeProcessScheduler> scheduler,
    std::shared_ptr<ProcessCallbackRegistry> registry,std::shared_ptr<NativeGameSkip> skip,
    std::shared_ptr<NativeChapterSequenceScope>& out) {
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(scheduler->busy())return S::Busy;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !skip || !skip->UsesScheduler(*scheduler))return S::MismatchedDomain;
    auto state=std::make_shared<State>();state->scheduler=scheduler;state->skip=std::move(skip);
    auto next=std::shared_ptr<NativeChapterSequenceScope>(new NativeChapterSequenceScope(state));
    if(!registry->Register(Targets,next))return S::DuplicateBinding;out=std::move(next);return S::Ready;
}
S NativeChapterSequenceScope::BindCarried(ProcessHandle h,std::optional<std::uint32_t> flags) {
    if(auto status=state_->Admission();status!=S::Ready)return status;
    const auto s=state_->scheduler.lock();const auto view=s->Observe(h);
    if(!view || !view->linked || view->root || (view->flags&1) || view->program!=Program() ||
        view->persistent_target!=12 || view->persistent_adjustment!=1 || !s->HasType(h,Type()))return S::InvalidProcess;
    state_->carried=ChapterScopeSnapshot{std::move(h),flags};return S::Ready;
}
S NativeChapterSequenceScope::BindCarriedAbsent() {
    if(auto status=state_->Admission();status!=S::Ready)return status;
    state_->carried=ChapterScopeSnapshot{};return S::Ready;
}
std::optional<ProcessHandle> NativeChapterSequenceScope::Current() const {
    const auto s=state_->scheduler.lock();if(!s || !s->root(2) || !state_->carried)return {};
    const auto& h=state_->carried->process;
    if(!h)return ProcessHandle{};
    const auto view=s->Observe(h);
    if(!view || view->program!=Program() || !s->HasType(h,Type()))return {};
    return h;
}
std::optional<ChapterScopeSnapshot> NativeChapterSequenceScope::Observe(ProcessHandle h) const {
    const auto current=Current();if(!current || !h || *current!=h)return {};return state_->carried;
}
bool NativeChapterSequenceScope::UsesScheduler(const NativeProcessScheduler& s) const noexcept {return state_->scheduler.lock().get()==&s;}
ProcessType NativeChapterSequenceScope::Type() {
    // Actual vtable6397A0. The deleting destructor remains1DBA64, never the
    // base destructor or a fabricated current=null completion callback.
    auto type=ProcessType::Base();type.methods[0].target=0x1dba64;type.methods[2].target=Persistent;return type;
}
std::unique_ptr<ProcessContinuation> NativeChapterSequenceScope::Begin(const ProcessCall& call) {
    if(call.target!=Persistent || call.kind!=ProcessCallKind::Persistent || !call.has_self ||
        call.this_adjustment || call.argument_count || !call.process)return {};
    const auto current=Current();if(!current || *current!=call.process)return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->process=call.process;return next;
}
}

namespace fates::runtime::native {
// All88 records from executed original initializer5883A0..588CDC.
std::shared_ptr<const ProcessProgram> NativeChapterSequenceScope::Program() {
    static const auto program=ProcessProgram::Create({
        {0x00000004,0x00000000,0x00000000,0x00000000,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001db41c,0x00000000},
        {0x00000004,0x0000000c,0x00000000,0x00000000,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001da394,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001db1a0,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001db730,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001da2dc,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001db9e4,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001db1f8,0x00000000},
        {0x00000017,0x00000001,0x00000000,0x0050b98c,0x00000000},
        {0x00000008,0x00000000,0x00000000,0x0021752c,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001db714,0x00000000},
        {0x00000008,0x00000000,0x00000000,0x00430c90,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001db7a8,0x00000000},
        {0x00000004,0x00000001,0x00000000,0x00000000,0x00000000},
        {0x00000017,0x00000002,0x00000000,0x0050b970,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001db76c,0x00000000},
        {0x00000008,0x00000000,0x00000000,0x00430c80,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001da5d0,0x00000000},
        {0x00000004,0x00000002,0x00000000,0x00000000,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001da110,0x00000000},
        {0x00000008,0x00000000,0x00000000,0x00388cb4,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x004222e4,0x00000000},
        {0x00000018,0x00000003,0x00000000,0x0050b904,0x00000000},
        {0x00000012,0x000000fa,0x00000000,0x003cf600,0x00000000},
        {0x00000012,0x000000fa,0x00000001,0x003cf600,0x00000000},
        {0x0000000f,0x00000000,0x00000000,0x003cf6c4,0x00000000},
        {0x0000000f,0x00000001,0x00000000,0x003cf6c4,0x00000000},
        {0x00000004,0x00000003,0x00000000,0x00000000,0x00000000},
        {0x00000017,0x00000004,0x00000000,0x0050b904,0x00000000},
        {0x00000011,0x00000004,0x00000000,0x001b3adc,0x00000000},
        {0x00000008,0x00000000,0x00000000,0x004bcfe8,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001da58c,0x00000000},
        {0x00000008,0x00000000,0x00000000,0x0035ece4,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001e7d94,0x00000000},
        {0x00000008,0x00000000,0x00000000,0x0042f764,0x00000000},
        {0x00000003,0x00000005,0x00000000,0x00000000,0x00000000},
        {0x00000004,0x00000004,0x00000000,0x00000000,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001db1dc,0x00000000},
        {0x00000004,0x00000005,0x00000000,0x00000000,0x00000000},
        {0x00000011,0x00000005,0x00000000,0x001b3adc,0x00000000},
        {0x0000000f,0x00000001,0x00000000,0x003d2940,0x00000000},
        {0x00000008,0x00000000,0x00000000,0x003cf924,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001db1fc,0x00000000},
        {0x00000004,0x00000006,0x00000000,0x00000000,0x00000000},
        {0x00000012,0x000000fa,0x00000000,0x003cf620,0x00000000},
        {0x00000012,0x000000fa,0x00000001,0x003cf620,0x00000000},
        {0x0000000f,0x00000000,0x00000000,0x003cf6c4,0x00000000},
        {0x0000000f,0x00000001,0x00000000,0x003cf6c4,0x00000000},
        {0x00000008,0x00000000,0x00000000,0x00388dc0,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001db8f0,0x00000000},
        {0x00000008,0x00000000,0x00000000,0x00430c70,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001da488,0x00000000},
        {0x00000004,0x00000007,0x00000000,0x00000000,0x00000000},
        {0x00000008,0x00000000,0x00000000,0x00430b48,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001db40c,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001db7d0,0x00000000},
        {0x00000003,0x0000000d,0x00000000,0x00000000,0x00000000},
        {0x00000004,0x00000008,0x00000000,0x00000000,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001db414,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001db860,0x00000000},
        {0x00000003,0x0000000d,0x00000000,0x00000000,0x00000000},
        {0x00000004,0x00000009,0x00000000,0x00000000,0x00000000},
        {0x00000003,0x0000000d,0x00000000,0x00000000,0x00000000},
        {0x00000004,0x0000000a,0x00000000,0x00000000,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001db5b4,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001da394,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001da0e8,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001da5c0,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001da460,0x00000000},
        {0x00000003,0x00000000,0x00000000,0x00000000,0x00000000},
        {0x00000004,0x0000000b,0x00000000,0x00000000,0x00000000},
        {0x00000007,0x00000000,0x00000000,0x00000000,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001da2dc,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x00419bbc,0x00000000},
        {0x00000008,0x00000000,0x00000000,0x00430c70,0x00000000},
        {0x0000000f,0x00000001,0x00000000,0x003d2940,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001db6a0,0x00000000},
        {0x00000003,0x0000000d,0x00000000,0x00000000,0x00000000},
        {0x00000004,0x0000000d,0x00000000,0x00000000,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001db1f4,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001db980,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001da1f4,0x00000000},
        {0x0000000f,0x00000001,0x00000000,0x003d2940,0x00000000},
        {0x00000008,0x00000000,0x00000000,0x001e7d98,0x00000000},
        {0x00000008,0x00000000,0x00000000,0x003cf924,0x00000000},
        {0x0000000b,0x00000000,0x00000000,0x001da390,0x00000000},
        {0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
    });return program;
}
}
