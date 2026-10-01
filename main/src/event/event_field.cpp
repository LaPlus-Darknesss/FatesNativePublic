#include "fates/event/event.hpp"
#include "fates/detail/event_runtime.hpp"
namespace event {
void RecoveryUnit(ProcInst* p){fates::decomp_detail::EventRuntimeCall("event.RecoveryUnit",p);} void BandOpenCreate(ProcInst* p){fates::decomp_detail::EventRuntimeCall("event.BandOpenCreate",p);} void BandCloseCreate(){fates::decomp_detail::EventRuntimeCall("event.BandCloseCreate");} void CrossFadeCreate(ProcInst* p,int n){fates::decomp_detail::EventRuntimeCall("event.CrossFadeCreate",p,n);}
void MiracleShootCreate(ProcInst* p,float a,float b,float c,float d){fates::decomp_detail::EventRuntimeCall("event.MiracleShootCreate",p,a,b,c,d);} void MiracleTelopCreate(ProcInst* p){fates::decomp_detail::EventRuntimeCall("event.MiracleTelopCreate",p);}
void FieldObjectMoveBind(ProcInst* p,const char* o,int a,int b,int c,const char* m){fates::decomp_detail::EventRuntimeCall("event.FieldObjectMoveBind",p,o,a,b,c,m);} void FieldObjectWarpBind(ProcInst* p,const char* o,const char* t){fates::decomp_detail::EventRuntimeCall("event.FieldObjectWarpBind",p,o,t);} void FieldObjectFadeOutBind(ProcInst* p,const char* o,int f){fates::decomp_detail::EventRuntimeCall("event.FieldObjectFadeOutBind",p,o,f);}
namespace JoinUniqueGuest {bool IsExistCard(const Person* p){return fates::decomp_detail::EventRuntimeValue<bool>("event.JoinUniqueGuest.IsExistCard",p);} void Create(ProcInst* p,const Person* q,bool b){fates::decomp_detail::EventRuntimeCall("event.JoinUniqueGuest.Create",p,q,b);} void EntryCard(const Person* p){fates::decomp_detail::EventRuntimeCall("event.JoinUniqueGuest.EntryCard",p);}}
namespace detail {void FieldProcessStep(const char* name){fates::decomp_detail::EventRuntimeCall(name);} }
}
