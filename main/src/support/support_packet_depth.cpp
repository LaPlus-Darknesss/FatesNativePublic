#include "fates/progression/progression_support_depth.hpp"

namespace fates::progression {

// Retail ownership is preserved while unresolved object layouts stay behind DepthRuntime.
DepthWord unit__Identifier__Clear(DepthRuntime& runtime,const DepthWord* args,std::size_t argc) { return InvokeDepth(runtime,0x00418C34u,args,argc); }
DepthWord unit__Identifier__Serialize(DepthRuntime& runtime,const DepthWord* args,std::size_t argc) { return InvokeDepth(runtime,0x0053AA40u,args,argc); }
DepthWord unit__Edit__Serialize(DepthRuntime& runtime,const DepthWord* args,std::size_t argc) { return InvokeDepth(runtime,0x0053AC88u,args,argc); }

} // namespace fates::progression
