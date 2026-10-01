#pragma once

#include "fates/io/thread_controller.hpp"

class FileController;
class IAllocator;

namespace fates::decomp_detail {

FileController* GetFileController();
bool ProbeFileSystemPathExists(const char* path);
int ComputeFileObjectSweepBudget(const IAllocator& allocator);

bool StartThreadControllerWorker(
    void*& stackMemory,
    ThreadController::EntryPoint entry,
    void* argument,
    int priority,
    unsigned int stackSize);
void StopThreadControllerWorker(void* stackMemory);
int GetThreadControllerPriority();
int GetCurrentThreadPriority();
void SetThreadControllerPriority(int priority);
void SignalFileControllerWorker();
void WaitFileControllerWorker();
void YieldFileControllerWorker();
void SleepFileControllerPoll();

} // namespace fates::decomp_detail
