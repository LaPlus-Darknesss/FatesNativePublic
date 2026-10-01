#pragma once

namespace GfxMemory {

void Initialize();

void FreeAddress(void* address);

void* TryAllocDevice(unsigned int size, int alignment);
void* TryAllocVramAB(unsigned int size, int alignment);
void* TryAllocVramBA(unsigned int size, int alignment);

} // namespace GfxMemory
