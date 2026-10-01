#include "fates/event/native_proc_event.hpp"
#include <map>

namespace fates::event::native {
using namespace runtime::native;
using namespace presentation::native;
namespace {
constexpr std::uint32_t Enable=0x42f9ac,Escape=0x42f9c4,Rollback=0x42f9cc,
    Next=0x42f9d4,Search=0x42fa40,Tick=0x42fa54,FadeEnd=0x42fa90,
    Destroy=0x42fb78,Persistent=0x42f9a8,Wait=0x50bab8,Consistency=0x4242c8;
constexpr std::array Targets{Enable,Escape,Rollback,Next,Search,Tick,FadeEnd,Destroy,Persistent,Wait};
using S=ProcEventStatus;
using CS=cmvm::native::ScriptSessionStatus;
ProcessCall Service(ProcessHandle h,std::uint32_t target) {
    ProcessCall c;c.process=std::move(h);c.kind=ProcessCallKind::Service;c.target=target;return c;
}
}
struct NativeProcEvent::State {
    struct Row {
        ProcEventSnapshot view;
        cmvm::native::AttachedPhaseSelection phase_anchor;
        std::unique_ptr<PhaseEventVm> vm;
    };
    std::weak_ptr<NativeProcessScheduler> scheduler;
    std::shared_ptr<NativeGameSkip> skip;
    std::shared_ptr<NativeFadeSystem> fade;
    std::shared_ptr<NativeMapBinder> binder;
    std::shared_ptr<NativeResourceDelay> delay;
    std::shared_ptr<const cmvm::native::ScriptAttachmentSession> session;
    std::shared_ptr<const NativePhaseEventQueries> queries;
    std::shared_ptr<const NativeEventFlagCommands> flags;
    std::shared_ptr<const NativeUnitEventQueries> units;
    std::shared_ptr<map::native::NativeCameraWait> camera;
    std::shared_ptr<NativeEventCamera> camera_commands;
    std::map<std::uint64_t,Row> rows;
    ProcessHandle current;
    std::uint64_t started{},destroyed{};
    bool Live() const {auto s=scheduler.lock();return s && s->root(2);}
    S Mutable(ProcessAccess* access) const {
        auto s=scheduler.lock();if(!s || !s->root(2))return S::Retired;
        if(access)return access->BelongsTo(*s)?S::Ready:S::MismatchedDomain;
        return s->busy()?S::Busy:S::Ready;
    }
    Row* Find(ProcessHandle h) {
        if(!h)return nullptr;auto it=rows.find(h->serial);
        return it!=rows.end() && it->second.view.process==h?&it->second:nullptr;
    }
    S SearchNext(std::uint32_t type,ProcEventInspector inspector,const Row* previous,
        cmvm::native::AttachedScriptFunction& function,cmvm::native::AttachedPhaseSelection& phase) const {
        if(session->retired())return S::StaleSession;
        if(inspector!=ProcEventInspector::None && inspector!=ProcEventInspector::Phase)return S::InvalidKind;
        if(type>255)return S::NoMatch;
        CS found;
        if(inspector==ProcEventInspector::Phase) {
            if(type<16 || type>19)return S::InvalidKind;
            if(!queries)return S::UnknownState;
            std::int32_t turn{},force{};
            if(queries->Query(PhaseIntegerQuery::Turn,{},turn)!=PhaseQueryStatus::Ok ||
               queries->Query(PhaseIntegerQuery::ActiveForce,{},force)!=PhaseQueryStatus::Ok)return S::StaleNativeContext;
            found=session->FindNext(static_cast<PhaseEventKind>(type),static_cast<std::uint16_t>(turn),
                static_cast<std::uint8_t>(force),previous?&previous->phase_anchor:nullptr,phase);
            if(found==CS::Found)found=session->SelectFunction(phase.attachment(),phase.selection().declaration()->function_index,function);
        } else found=session->FindNextTyped(static_cast<std::uint8_t>(type),previous?&previous->view.function:nullptr,function);
        return found==CS::Found?S::Ready:found==CS::NoMatchInSession?S::NoMatch:S::StaleSession;
    }
    S CreateRow(ProcessAccess* access,ProcessHandle parent,std::uint8_t type,ProcEventInspector inspector,
        const cmvm::native::AttachedScriptFunction& function,
        cmvm::native::AttachedPhaseSelection phase,ProcessHandle& out) {
        auto s=scheduler.lock();if(!s)return S::Retired;
        const auto p=access?access->Observe(parent):s->Observe(parent);
        if(!p || !p->linked || (p->flags&1))return S::InvalidParent;
        const auto b=binder->Current();if(!b || !skip->Current())return S::UnknownState;
        // Preflight VM/session and allocation admission before publishing any
        // ownership. Type0 parameters require initialized VM argument words;
        // this original CreateBind overload supplies none, so they are refused.
        Row row;
        if(PhaseEventVm::CreateAttachedFunctionWithServices(function,{},256,session,{queries,flags,units,camera,camera_commands},row.vm)!=PhaseVmStatus::Ready)return S::VmRefused;
        GameSkipControlHandle control;
        const auto captured=access?skip->Capture(*access,control):skip->Capture(control);
        if(captured!=GameSkipStatus::Ready)return S::UnknownState;
        bool transitioned{};
        if(*b && (access?binder->Bind(*access,*b,transitioned):binder->Bind(*b,transitioned))!=MapBinderStatus::Ready) {
            if(access)skip->ReleaseControl(*access,control);else skip->ReleaseControl(control);
            return S::UnknownState;
        }
        auto proc_type=ProcessType::Base();proc_type.methods[0].target=Destroy;
        proc_type.methods[1].target=Tick;proc_type.methods[2].target=Persistent;
        ProcessHandle process;
        const auto created=access?access->Create(parent,NativeProcEvent::Program(),std::string("ProcEvent"),true,proc_type,process):
            s->Create(parent,NativeProcEvent::Program(),std::string("ProcEvent"),true,proc_type,process);
        if(created!=ProcessStatus::Ready) {
            if(*b) {if(access)binder->Unbind(*access,*b,transitioned);else binder->Unbind(*b,transitioned);}
            if(access)skip->ReleaseControl(*access,control);else skip->ReleaseControl(control);
            return S::InvalidParent;
        }
        row.view.process=process;row.view.type=type;row.view.inspector=inspector;
        row.view.function=function;row.view.skip_control=control;row.view.roots_started=1;
        row.phase_anchor=std::move(phase);rows.emplace(process->serial,std::move(row));
        current=process;++started;out=std::move(process);return S::Ready;
    }
    S Typed(ProcessAccess* access,ProcessHandle parent,std::uint32_t type,ProcEventInspector inspector,ProcessHandle& out) {
        if(auto status=Mutable(access);status!=S::Ready)return status;
        cmvm::native::AttachedScriptFunction function;cmvm::native::AttachedPhaseSelection phase;
        // The first current inspector match precedes active-instance exclusion.
        if(auto status=SearchNext(type,inspector,nullptr,function,phase);status!=S::Ready)return status;
        if(current)return S::AlreadyActive;
        return CreateRow(access,std::move(parent),static_cast<std::uint8_t>(type),inspector,function,std::move(phase),out);
    }
    S Direct(ProcessAccess* access,ProcessHandle parent,const cmvm::native::AttachedScriptFunction& function,ProcessHandle& out) {
        if(auto status=Mutable(access);status!=S::Ready)return status;
        if(current)return S::AlreadyActive;
        if(!function || !session->IsAttached(function.attachment()))return S::InvalidFunction;
        return CreateRow(access,std::move(parent),4,ProcEventInspector::None,function,{},out);
    }
    S SetFlags(ProcessAccess* access,ProcessHandle h,std::uint32_t value) {
        if(auto status=Mutable(access);status!=S::Ready)return status;
        auto* row=Find(h);if(!row)return S::InvalidHandle;row->view.flags=value;return S::Ready;
    }
    S BindServices(ProcessAccess* access,std::shared_ptr<const NativePhaseEventQueries> q,
        std::shared_ptr<const NativeEventFlagCommands> f) {
        if(auto status=Mutable(access);status!=S::Ready)return status;
        if(current)return S::AlreadyActive;
        if(!NativeEventServices{q,f,units,camera,camera_commands}.UsesOneRuntime())return S::MismatchedDomain;
        if((q && q->Validate()!=PhaseQueryStatus::Ok) ||
            (f && f->Validate()!=EventFlagStatus::Ok) ||
            (units && units->Validate()!=UnitEventQueryStatus::Ok))return S::StaleNativeContext;
        queries=std::move(q);flags=std::move(f);return S::Ready;
    }
};
struct NativeProcEvent::Continuation final:ProcessContinuation {
    std::shared_ptr<State> state;ProcessCall call;
    unsigned stage{},channel{};bool skipping{};
    ProcessCallbackStep Control(ProcessHandle context,GameSkipControlHandle control,GameSkipControlOperation operation) {
        auto nested=state->skip->ControlCall(std::move(context),std::move(control),operation);
        return nested?ProcessCallbackStep::Call(*nested):ProcessCallbackStep::Blocked();
    }
    ProcessCallbackStep Step(ProcessAccess& access) override {
        auto scheduler=state->scheduler.lock();if(!scheduler || !access.BelongsTo(*scheduler))return ProcessCallbackStep::Blocked();
        auto* row=state->Find(call.process);if(!row)return ProcessCallbackStep::Blocked();
        auto& v=row->view;
        if(call.target==Persistent)return ProcessCallbackStep::Return();
        if(call.target==Tick) {
            if(stage==0) {
                const auto escaped=state->skip->ControlIsEscape(v.skip_control);
                if(!escaped)return ProcessCallbackStep::Blocked();
                if(*escaped)return ProcessCallbackStep::Return();
                stage=1;
            }
            if(state->current!=call.process)return ProcessCallbackStep::Blocked();
            const ProcEventVmAccess vm_access(access,state->current);
            const auto result=row->vm->Run(1,vm_access);
            if(result.status==PhaseVmStatus::InstructionBudget)return ProcessCallbackStep::Continue();
            if(result.status==PhaseVmStatus::Yielded)return ProcessCallbackStep::Return();
            if(result.status!=PhaseVmStatus::Returned)return ProcessCallbackStep::Blocked();
            return access.Next(call.process)==ProcessStatus::Ready?ProcessCallbackStep::Return():ProcessCallbackStep::Blocked();
        }
        if(call.target==Enable || call.target==Escape || call.target==Rollback) {
            if(stage)return ProcessCallbackStep::Return();
            if(call.target==Enable && (v.flags&1))return ProcessCallbackStep::Return();
            auto nested=state->skip->ControlCall(call.process,v.skip_control,call.target==Enable?
                GameSkipControlOperation::Enable:call.target==Escape?GameSkipControlOperation::Escape:GameSkipControlOperation::Rollback);
            if(!nested)return ProcessCallbackStep::Blocked();stage=1;return ProcessCallbackStep::Call(*nested);
        }
        if(call.target==Wait) {
            auto wait=state->skip->ControlIsWait(v.skip_control);
            return wait?ProcessCallbackStep::Return(*wait?1u:0u):ProcessCallbackStep::Blocked();
        }
        if(call.target==Search)return ProcessCallbackStep::Return(v.type!=4?1u:0u);
        if(call.target==Next) {
            if(stage || v.type==4)return ProcessCallbackStep::Return();
            cmvm::native::AttachedScriptFunction function;cmvm::native::AttachedPhaseSelection phase;
            const auto found=state->SearchNext(v.type,v.inspector,row,function,phase);
            if(found==S::NoMatch) {v.function={};stage=1;return ProcessCallbackStep::Delete(call.process);}
            if(found!=S::Ready)return ProcessCallbackStep::Blocked();
            if(row->vm->RetargetAttachedFunction(function)!=PhaseVmStatus::Ready)return ProcessCallbackStep::Blocked();
            v.function=std::move(function);row->phase_anchor=std::move(phase);++v.roots_started;
            return ProcessCallbackStep::Return();
        }
        if(call.target==FadeEnd) {
            if(stage==0) {
                if(!(v.flags&2))return ProcessCallbackStep::Return();
                const auto current=state->skip->Current();if(!current)return ProcessCallbackStep::Blocked();
                if(*current) {
                    bool blackout{};
                    if(state->skip->PrepareEventFadeEnd(access,*current,skipping,blackout)!=GameSkipStatus::Ready)return ProcessCallbackStep::Blocked();
                    if(skipping && blackout)return ProcessCallbackStep::Return();
                }
                stage=1;
            }
            while(channel<2) {
                if(stage==1) {
                    if(skipping) {
                        const auto fade=state->fade->ObserveChannel(channel);if(!fade)return ProcessCallbackStep::Blocked();
                        if(fade->active) {++channel;continue;}
                    }
                    stage=2;return ProcessCallbackStep::Call(NativeFadeSystem::FadeCall(call.process,250,channel,FadeTone::Black,FadeDirection::Out));
                }
                if(stage==2) {stage=3;return ProcessCallbackStep::Call(NativeFadeSystem::WaitCall(call.process,channel));}
                ++channel;stage=1;
            }
            return ProcessCallbackStep::Return();
        }
        // Destructor order is observable: rollback, actual ConsistencyCheck,
        // CURRENT binder unbind, active-null publication, context release.
        if(stage==0) {
            auto nested=state->skip->ControlCall(scheduler->root(2),v.skip_control,GameSkipControlOperation::Rollback);
            if(!nested)return ProcessCallbackStep::Blocked();stage=1;return ProcessCallbackStep::Call(*nested);
        }
        if(stage==1) {
            auto nested=Service(scheduler->root(2),Consistency);nested.arguments[0]=v.type;nested.argument_count=1;
            stage=2;return ProcessCallbackStep::Call(std::move(nested));
        }
        if(stage==2) {
            const auto current_binder=state->binder->Current();if(!current_binder)return ProcessCallbackStep::Blocked();
            if(*current_binder) {bool changed{};if(state->binder->Unbind(access,*current_binder,changed)!=MapBinderStatus::Ready)return ProcessCallbackStep::Blocked();}
            stage=3;
        }
        // Retiring the control itself has no retail side effect. It releases
        // the native retained identity after the original rollback completed.
        if(state->skip->ReleaseControl(access,v.skip_control)!=GameSkipStatus::Ready)return ProcessCallbackStep::Blocked();
        state->current={};row->vm.reset();++state->destroyed;state->rows.erase(call.process->serial);
        return ProcessCallbackStep::Return();
    }
};
NativeProcEvent::NativeProcEvent(std::shared_ptr<State> s):state_(std::move(s)){}
NativeProcEvent::~NativeProcEvent()=default;
S NativeProcEvent::Create(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<NativeGameSkip> skip,std::shared_ptr<NativeFadeSystem> fade,std::shared_ptr<NativeMapBinder> binder,
    std::shared_ptr<NativeResourceDelay> delay,std::shared_ptr<const cmvm::native::ScriptAttachmentSession> session,
    std::shared_ptr<const NativePhaseEventQueries> queries,std::shared_ptr<const NativeEventFlagCommands> flags,std::shared_ptr<NativeProcEvent>& out) {
    return CreateWithServices(std::move(scheduler),std::move(registry),std::move(skip),std::move(fade),
        std::move(binder),std::move(delay),std::move(session),{std::move(queries),std::move(flags),{}},out);
}
S NativeProcEvent::CreateWithServices(std::shared_ptr<NativeProcessScheduler> scheduler,std::shared_ptr<ProcessCallbackRegistry> registry,
    std::shared_ptr<NativeGameSkip> skip,std::shared_ptr<NativeFadeSystem> fade,std::shared_ptr<NativeMapBinder> binder,
    std::shared_ptr<NativeResourceDelay> delay,std::shared_ptr<const cmvm::native::ScriptAttachmentSession> session,
    NativeEventServices services,std::shared_ptr<NativeProcEvent>& out) {
    if(!scheduler || !scheduler->root(2))return S::NullScheduler;
    if(!registry || !scheduler->UsesCallbacks(registry.get()) || !skip || !skip->UsesScheduler(*scheduler) ||
        !fade || !fade->UsesScheduler(*scheduler) || !binder || !binder->UsesScheduler(*scheduler) ||
        !delay || !delay->UsesScheduler(*scheduler))return S::MismatchedDomain;
    if(!session)return S::NullSession;if(session->retired())return S::StaleSession;
    if(!services.UsesOneRuntime())return S::MismatchedDomain;
    if(services.camera && !services.camera->UsesScheduler(*scheduler))return S::MismatchedDomain;
    if(services.camera_commands && !services.camera_commands->UsesScheduler(*scheduler))return S::MismatchedDomain;
    auto s=std::make_shared<State>();s->scheduler=scheduler;s->skip=std::move(skip);s->fade=std::move(fade);
    s->binder=std::move(binder);s->delay=std::move(delay);s->session=std::move(session);s->queries=std::move(services.phase);
    s->flags=std::move(services.flags);s->units=std::move(services.units);s->camera=std::move(services.camera);s->camera_commands=std::move(services.camera_commands);
    auto next=std::shared_ptr<NativeProcEvent>(new NativeProcEvent(s));
    if(!registry->Register(Targets,next))return S::DuplicateBinding;out=std::move(next);return S::Ready;
}
S NativeProcEvent::CreateTyped(ProcessHandle p,std::uint32_t t,ProcEventInspector i,ProcessHandle& out) {return state_->Typed(nullptr,std::move(p),t,i,out);}
S NativeProcEvent::CreateTyped(ProcessAccess& a,ProcessHandle p,std::uint32_t t,ProcEventInspector i,ProcessHandle& out) {return state_->Typed(&a,std::move(p),t,i,out);}
S NativeProcEvent::CreateFunction(ProcessHandle p,const cmvm::native::AttachedScriptFunction& f,ProcessHandle& out) {return state_->Direct(nullptr,std::move(p),f,out);}
S NativeProcEvent::CreateFunction(ProcessAccess& a,ProcessHandle p,const cmvm::native::AttachedScriptFunction& f,ProcessHandle& out) {return state_->Direct(&a,std::move(p),f,out);}
S NativeProcEvent::BindContextServices(std::shared_ptr<const NativePhaseEventQueries> q,std::shared_ptr<const NativeEventFlagCommands> f) {return state_->BindServices(nullptr,std::move(q),std::move(f));}
S NativeProcEvent::BindContextServices(ProcessAccess& a,std::shared_ptr<const NativePhaseEventQueries> q,std::shared_ptr<const NativeEventFlagCommands> f) {return state_->BindServices(&a,std::move(q),std::move(f));}
S NativeProcEvent::SetFlags(ProcessHandle h,std::uint32_t f) {return state_->SetFlags(nullptr,std::move(h),f);}
S NativeProcEvent::SetFlags(ProcessAccess& a,ProcessHandle h,std::uint32_t f) {return state_->SetFlags(&a,std::move(h),f);}
std::optional<ProcessHandle> NativeProcEvent::Current() const {return state_->Live()?std::optional{state_->current}:std::nullopt;}
std::optional<ProcEventSnapshot> NativeProcEvent::Observe(ProcessHandle h) const {
    if(state_->Live())if(auto* row=state_->Find(h)) {auto out=row->view;out.vm=row->vm->observation();return out;}return {};
}
std::uint64_t NativeProcEvent::contexts_started() const noexcept {return state_->started;}
std::uint64_t NativeProcEvent::contexts_destroyed() const noexcept {return state_->destroyed;}
bool NativeProcEvent::UsesScheduler(const NativeProcessScheduler& s) const noexcept {return state_->scheduler.lock().get()==&s;}
bool NativeProcEvent::UsesRuntime(const NativeRuntime& r) const noexcept {
    return state_->queries && state_->queries->runtime().get()==&r &&
        (!state_->flags || state_->flags->runtime().get()==&r) && (!state_->units || state_->units->runtime().get()==&r) &&
        (!state_->camera || state_->camera->runtime().get()==&r) &&
        (!state_->camera_commands || state_->camera_commands->runtime().get()==&r);
}
std::shared_ptr<const ProcessProgram> NativeProcEvent::Program() {
    static const auto program=ProcessProgram::Create({{4,0,0,0,0},{15,1,0,0x3d2940,0},
        {11,0,0,Enable,0},{13,0,0,8,1},{11,0,0,FadeEnd,0},{11,0,0,Escape,0},
        {12,0,0,Wait,0},{15,1,0,0x3d2940,0},{11,0,0,Rollback,0},{11,0,0,Next,0},
        {23,0,0,Search,0},{}});return program;
}
std::unique_ptr<ProcessContinuation> NativeProcEvent::Begin(const ProcessCall& c) {
    if(!state_->Live() || !state_->Find(c.process) || !c.has_self || c.this_adjustment || c.argument_count)return {};
    if(c.target==Destroy) {if(c.kind!=ProcessCallKind::Destroy)return {};}
    else if(c.target==Persistent) {if(c.kind!=ProcessCallKind::Persistent)return {};}
    else {
        if(c.kind!=ProcessCallKind::Descriptor)return {};
        const auto command=c.target==Tick?13:c.target==Wait?12:c.target==Search?23:
            (c.target==Enable || c.target==Escape || c.target==Rollback || c.target==Next || c.target==FadeEnd)?11:0;
        if(!command || c.command!=command)return {};
    }
    auto next=std::make_unique<Continuation>();next->state=state_;next->call=c;return next;
}
}
