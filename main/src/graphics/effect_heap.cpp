#include "fates/graphics/effect_heap.hpp"

#include "fates/graphics/gfx_memory.hpp"

EffectHeap::~EffectHeap() = default;

void* EffectHeap::Alloc(unsigned int size, int alignment) {
    // Retail 0x0011D384 is a two-instruction argument veneer that falls
    // directly into GfxMemory::TryAllocDevice at 0x0011D390.
    return GfxMemory::TryAllocDevice(size, alignment);
}

void EffectHeap::Free(void* address) {
    // Retail 0x0011D378 similarly falls through into the already-promoted
    // GfxMemory::FreeAddress entry at 0x0011D380.
    GfxMemory::FreeAddress(address);
}
