#include "fates/map/native_camera_wait.hpp"
#include "fates/runtime/native_runtime.hpp"
#include <map>

namespace fates::map::native {
using namespace runtime::native;
using S=CameraWaitStatus;
namespace {constexpr std::uint32_t Tick=0x360154,Destroy=0x3601dc;constexpr std::array Targets{Tick,Destroy};}
struct NativeCameraWait::State {
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<NativeMapCamera> camera;
    std::shared_ptr<NativeGameSkip> skip;
    std::shared_ptr<presentation::native::NativeFadeSystem> fade;
    std::optional<bool> draw_present;
    std::map<std::uint64_t,ProcessHandle> rows;
    S Mutable(ProcessAccess* a) const {
        auto s=scheduler.lock();if(!s || !s->root(2))return S::Retired;
        if(a)return a->BelongsTo(*s)?S::Ready:S::MismatchedDomain;
        return s->busy()?S::Busy:S::Ready;
    }
    std::optional<bool> Blackout() const {
        const auto current=skip->Current();if(!current)return {};
        if(!*current)return false;
        const auto view=skip->Observe(*current);if(!view)return {};
        return view->state==3;
    }
    S Bind(ProcessAccess* access,ProcessHandle parent,ProcessHandle& child) {
        if(auto status=Mutable(access);status!=S::Ready)return status;
        bool scrolling{};
        // Original reads the camera predicate BEFORE checking a null parent.
        if(camera->ScrollProlixity(scrolling)!=MapCameraStatus::Ready)return S::CameraUnavailable;
        if(!scrolling || !parent){child.reset();return S::Ready;}
        auto scheduler_owner=scheduler.lock();const auto view=access?access->Observe(parent):scheduler_owner->Observe(parent);
        if(!view || !view->linked || (view->flags&1))return S::InvalidParent;
        auto type=ProcessType::Base();type.methods[0].target=Destroy;type.methods[1].target=Tick;
        ProcessHandle next;
        const auto status=access?access->Create(parent,ProcessProgram::Default(),"ProcWaitCameraProlixity",true,type,next):
            scheduler_owner->Create(parent,ProcessProgram::Default(),"ProcWaitCameraProlixity",true,type,next);
        if(status!=ProcessStatus::Ready)return S::InvalidParent;
        rows.emplace(next->serial,next);child=std::move(next);return S::Ready;
    }
};
struct NativeCameraWait::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;ProcessCall call;unsigned stage{};
    ProcessCallbackStep Step(ProcessAccess& access) override {
        if(state->Mutable(&access)!=S::Ready)return ProcessCallbackStep::Blocked();
        if(stage==2)return ProcessCallbackStep::Return(); // original Delete has finished marking
        const auto found=state->rows.find(call.process->serial);
        if(found==state->rows.end() || found->second!=call.process)return ProcessCallbackStep::Blocked();
        if(call.target==Destroy){state->rows.erase(found);return ProcessCallbackStep::Return();}
        if(stage==0) {
            const auto fade=state->fade->ObserveChannel(0);if(!fade)return ProcessCallbackStep::Blocked();
            bool instant=fade->blackout;
            if(!instant) {
                if(!state->draw_present)return ProcessCallbackStep::Blocked();
                instant=!*state->draw_present;
                if(!instant) {const auto blackout=state->Blackout();if(!blackout)return ProcessCallbackStep::Blocked();instant=*blackout;}
            }
            if(instant && state->camera->Instant(access)!=MapCameraStatus::Ready)return ProcessCallbackStep::Blocked();
            stage=1;
        }
        bool scrolling{};
        if(state->camera->ScrollProlixity(scrolling)!=MapCameraStatus::Ready)return ProcessCallbackStep::Blocked();
        if(scrolling)return ProcessCallbackStep::Return();
        if(stage==1){stage=2;return ProcessCallbackStep::Delete(call.process);}
        return ProcessCallbackStep::Return();
    }
};
NativeCameraWait::NativeCameraWait(std::shared_ptr<State> s):state_(std::move(s)){}
S NativeCameraWait::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<NativeMapCamera> camera,std::shared_ptr<NativeGameSkip> skip,
    std::shared_ptr<presentation::native::NativeFadeSystem> fade,std::shared_ptr<NativeCameraWait>& out) {
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !camera || !camera->UsesScheduler(*scheduler) ||
        !skip || !skip->UsesScheduler(*scheduler) || !fade || !fade->UsesScheduler(*scheduler))return S::MismatchedDomain;
    auto s=std::make_shared<State>();s->scheduler=scheduler;s->camera=std::move(camera);s->skip=std::move(skip);s->fade=std::move(fade);
    auto next=std::shared_ptr<NativeCameraWait>(new NativeCameraWait(s));
    if(!registry->Register(Targets,next))return S::DuplicateBinding;out=std::move(next);return S::Ready;
}
S NativeCameraWait::RestoreCarriedDrawPresence(bool present) {
    if(auto status=state_->Mutable(nullptr);status!=S::Ready)return status;state_->draw_present=present;return S::Ready;
}
S NativeCameraWait::Bind(ProcessHandle parent,ProcessHandle& child){return state_->Bind(nullptr,std::move(parent),child);}
S NativeCameraWait::Bind(ProcessAccess& access,ProcessHandle parent,ProcessHandle& child){return state_->Bind(&access,std::move(parent),child);}
S NativeCameraWait::EventWait(ProcessAccess& access,ProcessHandle event,CameraWaitOutcome& out) {
    if(auto status=state_->Mutable(&access);status!=S::Ready)return status;
    if(!runtime()->game.map_active){out={false,0};return S::Ready;}
    const auto blackout=state_->Blackout();if(!blackout)return S::UnknownState;
    if(*blackout) {
        if(state_->camera->Instant(access)!=MapCameraStatus::Ready)return S::CameraUnavailable;
        out={false,{}};return S::Ready;
    }
    const auto parent=access.Observe(event);if(!parent || !parent->linked || (parent->flags&1))return S::InvalidParent;
    ProcessHandle child;if(auto status=state_->Bind(&access,event,child);status!=S::Ready)return status;
    // Re-read after child creation: another existing blocking child also yields.
    const auto current=access.Observe(event);if(!current)return S::InvalidParent;
    const bool yielded=current->blocking_children!=0;out={yielded,yielded?1:0};return S::Ready;
}
bool NativeCameraWait::UsesScheduler(const NativeProcessScheduler& s) const noexcept{return state_->scheduler.lock().get()==&s;}
const std::shared_ptr<NativeRuntime>& NativeCameraWait::runtime() const noexcept{return state_->camera->runtime();}
std::vector<ProcessHandle> NativeCameraWait::Processes() const {
    std::vector<ProcessHandle> out;auto scheduler=state_->scheduler.lock();if(!scheduler || !scheduler->root(2))return out;
    for(const auto& [serial,h]:state_->rows){(void)serial;out.push_back(h);}return out;
}
std::unique_ptr<ProcessContinuation> NativeCameraWait::Begin(const ProcessCall& call) {
    if(!call.process || !call.has_self || call.this_adjustment || call.argument_count)return {};
    const auto row=state_->rows.find(call.process->serial);if(row==state_->rows.end() || row->second!=call.process)return {};
    if(call.target==Destroy){if(call.kind!=ProcessCallKind::Destroy)return {};}
    else if(call.target!=Tick || call.kind!=ProcessCallKind::Descriptor || call.command!=13)return {};
    auto next=std::make_unique<Continuation>();next->state=state_;next->call=call;return next;
}
}
