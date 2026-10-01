#include "fates/io/thread_controller.hpp"

#include "fates/detail/file_controller_runtime.hpp"

bool ThreadController::CreateThread(
    EntryPoint entry,
    void* argument,
    int priority,
    unsigned int stackSize) {
    if (running_) {
        return false;
    }

    running_ = true;
    if (!fates::decomp_detail::StartThreadControllerWorker(
            stackMemory_, entry, argument, priority, stackSize)) {
        running_ = false;
        stackMemory_ = nullptr;
        return false;
    }
    return true;
}

void ThreadController::CurrentPriority() {
    if (savedPriority_ >= 0 || !running_) {
        return;
    }

    savedPriority_ = fates::decomp_detail::GetThreadControllerPriority();
    fates::decomp_detail::SetThreadControllerPriority(
        fates::decomp_detail::GetCurrentThreadPriority());
}

void ThreadController::ResumePriority() {
    if (savedPriority_ < 0 || !running_) {
        return;
    }

    fates::decomp_detail::SetThreadControllerPriority(savedPriority_);
    savedPriority_ = -1;
}

bool ThreadController::DeleteThread() {
    if (!running_) {
        return false;
    }

    running_ = false;
    fates::decomp_detail::SignalFileControllerWorker();
    fates::decomp_detail::StopThreadControllerWorker(stackMemory_);
    stackMemory_ = nullptr;
    savedPriority_ = -1;
    return true;
}
