#pragma once

namespace fates::decomp_detail {

// Nintendo nngx initialization and its six retail tracking tables remain a
// platform-facing detail. GfxMemory owns the initialization policy.
bool InitializeGraphicsMemoryBackend();
void PanicGraphicsMemoryInitialization();
void ResetGraphicsMemoryTracking();

} // namespace fates::decomp_detail
