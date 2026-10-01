#pragma once

class EffectHeap {
public:
    EffectHeap() = default;
    ~EffectHeap();

    void* Alloc(unsigned int size, int alignment);
    void Free(void* address);
};
