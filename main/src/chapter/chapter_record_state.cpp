#include "fates/chapter/chapter_run_state.hpp"
namespace fates::chapter {
// Exact retail identity is source-owned; unresolved object layouts stay behind RunStateRuntime.
RunStateWord GameSkipControl__Record(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x001E0B60u,a,n);}
RunStateWord unit__util__ClearInvalidFlagEndOfChapter(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0041A4FCu,a,n);}
RunStateWord unit__Record__Deserialize(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0041B130u,a,n);}
RunStateWord unit__Record__Clear(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0041B184u,a,n);}
RunStateWord unit__Record__Serialize(RunStateRuntime& rt,const RunStateWord* a,std::size_t n){return InvokeRunState(rt,0x0053C05Cu,a,n);}
}
