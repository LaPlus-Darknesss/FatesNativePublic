#pragma once

class ThreadController {
public:
    using EntryPoint = void (*)(void*);

    bool CreateThread(EntryPoint entry, void* argument, int priority, unsigned int stackSize);
    void ResumePriority();
    void CurrentPriority();
    bool DeleteThread();

    bool IsRunning() const { return running_; }

private:
    void* stackMemory_{};
    bool running_{};
    int savedPriority_{-1};
};
