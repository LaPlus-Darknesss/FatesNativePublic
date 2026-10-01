#include "fates/chapter/chapter_run_state.hpp"
namespace fates::chapter {
// Exact retail identity is source-owned; unresolved object layouts stay behind RunStateRuntime.
RunStateWord GameUserGlobalData__Initialize(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0020C300u,a,n);}
RunStateWord GameUserGlobalData__Deserialize(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0020C324u,a,n);}
RunStateWord GameUserGlobalData__UpdateLastRoute(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0020C570u,a,n);}
RunStateWord GameUserGlobalData__ShowMovieSubtitle(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0020C5B4u,a,n);}
RunStateWord GameUserGlobalData__Reset(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0020C61Cu,a,n);}
RunStateWord GameUserGlobalData__Viewer__Update(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0020C6E4u,a,n);}
RunStateWord GameUserGlobalData__Finalize(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0020C858u,a,n);}
RunStateWord GameUserGlobalData__GameUserGlobalData(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0020C900u,a,n);}
RunStateWord GameUserGlobalData__Serialize(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0044C2D4u,a,n);}
RunStateWord GameUserGlobalData__IsCheatRTC(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0050D154u,a,n);}
}
