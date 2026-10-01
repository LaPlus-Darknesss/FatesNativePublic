#include "fates/progression/progression_support_depth.hpp"

namespace fates::progression {

// Retail ownership is preserved while unresolved object layouts stay behind DepthRuntime.
DepthWord ClassChangeEnumerator__EnumerateBuddy(DepthRuntime& runtime,const DepthWord* args,std::size_t argc) { return InvokeDepth(runtime,0x0021A26Cu,args,argc); }
DepthWord ClassChangeEnumerator__EnumerateMarrige(DepthRuntime& runtime,const DepthWord* args,std::size_t argc) { return InvokeDepth(runtime,0x0021A614u,args,argc); }
DepthWord ClassChangeEnumerator__EnumerateParallel(DepthRuntime& runtime,const DepthWord* args,std::size_t argc) { return InvokeDepth(runtime,0x0021A8E8u,args,argc); }

} // namespace fates::progression
