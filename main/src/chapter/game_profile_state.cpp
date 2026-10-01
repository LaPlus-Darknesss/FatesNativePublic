#include "fates/chapter/chapter_run_state.hpp"
namespace fates::chapter {
// Exact retail identity is source-owned; unresolved object layouts stay behind RunStateRuntime.
RunStateWord GameProfile__SetMessage(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0019AFA0u,a,n);}
RunStateWord GameProfile__Deserialize(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0019B018u,a,n);}
RunStateWord GameProfile__GetMessReal(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0019B118u,a,n);}
RunStateWord GameProfile__SetHideRank(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0019B168u,a,n);}
RunStateWord GameProfile__GetMessEpithet(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0019B188u,a,n);}
RunStateWord GameProfile__GetMessHometown(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0019B310u,a,n);}
RunStateWord GameProfile__SetCastleAddress(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0019B360u,a,n);}
RunStateWord GameProfile__GetExpressionName(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0019B370u,a,n);}
RunStateWord GameProfile__GetMessExpression(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0019B390u,a,n);}
RunStateWord GameProfile__SetEvaluationVisit(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0019B460u,a,n);}
RunStateWord GameProfile__SetEvaluationBattle(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0019B484u,a,n);}
RunStateWord GameProfile__Copy(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0019B4A8u,a,n);}
RunStateWord GameProfile__Reset(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0019B4BCu,a,n);}
RunStateWord GameProfile__Update(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0019B578u,a,n);}
RunStateWord GameProfile__GetCrc32(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0041706Cu,a,n);}
RunStateWord GameProfile__GetMessage(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x005085E4u,a,n);}
RunStateWord GameProfile__IsReliance(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x005085F4u,a,n);}
RunStateWord GameProfile__GetReliance(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x00508610u,a,n);}
RunStateWord GameProfile__IsBoughtRoute(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x00508620u,a,n);}
RunStateWord GameProfile__IsValidCastleAddress(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x00508640u,a,n);}
RunStateWord GameProfile__IsWrong(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x00508650u,a,n);}
RunStateWord GameProfile__Serialize(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x005086C0u,a,n);}
}
