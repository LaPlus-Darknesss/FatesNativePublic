#pragma once

#include "fates/engine/ident_hash.hpp"
#include "fates/io/file_mount.hpp"
#include "fates/io/thread_controller.hpp"

#include <mutex>

class FileObject;

class FileController {
public:
    FileController();

    void FinishAsync(FileObject* object);
    static void ThreadFunction(void* controller);
    bool DeleteObject(FileObject* object);
    static void DelayCallback(void* object);
    void Sweep(int bytesToFree);
    void RemoveAsync(FileObject* object);
    bool IsExist(const char* path) const;

    // Source-level helpers used by the promoted File facade. These expose
    // behavior already present in the retail controller lists without
    // claiming additional retail StackTrace functions.
    void FinishAllAsync();
    bool HasAsyncLoading() const;

    FileObject* FindObject(const char* path) const;
    void QueueAsyncObject(FileObject* object);
    void TouchObject(FileObject* object);
    void RegisterObject(FileObject* object);

private:
    void UnlinkCache(FileObject* object);
    void AppendCache(FileObject* object);
    void UnlinkAsync(FileObject* object);
    void PrioritizeAsync(FileObject* object);
    FileObject* SelectHighestPriorityAsync();

    mutable std::mutex mutex_{};
    FileObject* cacheHead_{};
    FileObject* cacheTail_{};
    int cacheCount_{};
    FileObject* asyncHead_{};
    FileObject* asyncTail_{};
    int asyncCount_{};
    IdentHash index_{0x7F};
    FileMount mount_{};
    ThreadController thread_{};
};
