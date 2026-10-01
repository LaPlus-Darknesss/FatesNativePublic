#pragma once

class DelayManager {
public:
    using Callback = void (*)(void*);

    static void Initialize();
    static void Tick();
    static void Free(void* pointer);
    static void Entry(Callback callback, void* pointer);
    static void Dump();
    static bool IsActive();
};
