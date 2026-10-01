#pragma once

class IAllocator {
public:
    // Return types are reconstructed from retail control flow because C++
    // StackTrace signatures do not encode return types.
    void* Alloc(unsigned int size, unsigned char alignment);
    void* Malloc(unsigned int size, unsigned short alignment);
    void* TryMalloc(unsigned int size, unsigned short alignment);

    void Free(void* memory);
    bool Resize(void* memory, unsigned int size);
    bool HasAddress(const void* memory) const;
};
