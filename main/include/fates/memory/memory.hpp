#pragma once

class IAllocator;

namespace Memory {

// Retail builds five allocator routes: one application heap, two device-memory
// heaps, and two VRAM heaps. The concrete ism heap implementations remain the
// next promoted frontier.
void InitializeStartUp();
void InitializeVRAM();

// Returns the owning allocator for an ARM32 address, or nullptr when the
// address is outside every registered retail allocator.
IAllocator* GetAllocator(const void* address);

} // namespace Memory
