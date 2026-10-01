#include "fates/event/event.hpp"
#include "fates/detail/event_runtime.hpp"
namespace event {
bool ScriptLoad(const char* p,ErrorLevel::Type e){ return fates::decomp_detail::EventRuntimeValue<bool>("event.ScriptLoad",p,e); }
void ScriptFree(const char* p,ErrorLevel::Type e){ fates::decomp_detail::EventRuntimeCall("event.ScriptFree",p,e); }
bool ScriptLoadRoute(const char* p,ErrorLevel::Type e){ return fates::decomp_detail::EventRuntimeValue<bool>("event.ScriptLoadRoute",p,e); }
void ScriptFreeRoute(const char* p,ErrorLevel::Type e){ fates::decomp_detail::EventRuntimeCall("event.ScriptFreeRoute",p,e); }
ProcInst* GetInstance(){return fates::decomp_detail::EventRuntimeValue<ProcInst*>("event.GetInstance");}
void PopInstance(ProcInst* p){fates::decomp_detail::EventRuntimeCall("event.PopInstance",p);} ProcInst* PushInstance(){return fates::decomp_detail::EventRuntimeValue<ProcInst*>("event.PushInstance");}
void InstantCall(Function::Type t){fates::decomp_detail::EventRuntimeCall("event.InstantCallType",t);} int InstantCall(const char* n){return fates::decomp_detail::EventRuntimeValue<int>("event.InstantCallName",n);} bool IsExistCall(const char* n){return fates::decomp_detail::EventRuntimeValue<bool>("event.IsExistCall",n);} void Call(ProcInst* p,const char* n){fates::decomp_detail::EventRuntimeCall("event.Call",p,n);}
Unit* GetUnit(){return fates::decomp_detail::EventRuntimeValue<Unit*>("event.GetUnit");} Unit* GetUnit2(){return fates::decomp_detail::EventRuntimeValue<Unit*>("event.GetUnit2");} void SetUnit(const Unit* u){fates::decomp_detail::EventRuntimeCall("event.SetUnit",u);} void SetUnit2(const Unit* u){fates::decomp_detail::EventRuntimeCall("event.SetUnit2",u);}
void ConsistencyCheck(Function::Type t){fates::decomp_detail::EventRuntimeCall("event.ConsistencyCheck",t);}
void InitializeCommand(){
    // PROVEN STRUCTURALLY: retail constructs exactly 403 unique CmCFunction
    // registrations. Raw ARM + StackTrace recover every ev::* command string and
    // callback symbol; see evidence/event_command_bindings_us_se_v11.*.
    fates::decomp_detail::EventRuntimeCall("event.InitializeCommand",GetCommandBindings(),GetCommandBindingCount());
}
namespace detail {
bool ProcEventFacade::CreateBind(ProcInst* p,Function::Type t,bool (*f)(cmvm::CmFunction*,const Arg*),const Arg* a){return fates::decomp_detail::EventRuntimeValue<bool>("event.ProcEvent.CreateBindTyped",p,t,f,a);} bool ProcEventFacade::CreateBind(ProcInst* p,cmvm::CmFunction* f){return fates::decomp_detail::EventRuntimeValue<bool>("event.ProcEvent.CreateBindFunction",p,f);}
#define STEP(N) void ProcEventFacade::N(){fates::decomp_detail::EventRuntimeCall("event.ProcEvent." #N);}
STEP(Persistent) STEP(SkipEnable) STEP(SkipEscape) STEP(SkipRollback) STEP(SetNextFunction) STEP(Tick) STEP(FadeEnd) STEP(DestroyOwnedState)
#undef STEP
bool ProcEventFacade::IsSearchFunction(){return fates::decomp_detail::EventRuntimeValue<bool>("event.ProcEvent.IsSearchFunction");} bool ProcEventFacade::IsSkipWait() const{return fates::decomp_detail::EventRuntimeValue<bool>("event.ProcEvent.IsSkipWait");}
}
}
