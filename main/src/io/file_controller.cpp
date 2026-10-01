#include "fates/io/file_controller.hpp"

#include "fates/detail/file_controller_runtime.hpp"
#include "fates/io/file_object.hpp"
#include "fates/io/file.hpp"
#include "fates/io/file_cache_policies.hpp"

#include <cstdint>

namespace {
constexpr std::uint32_t kCleanupPendingFlag = 0x04000000u;
constexpr std::uint32_t kAsyncQueuedFlag = 0x20000000u;
constexpr std::uint32_t kAsyncReadingFlag = 0x40000000u;
constexpr std::uint32_t kAsyncCompleteFlag = 0x80000000u;
constexpr std::uint32_t kAsyncMask = kAsyncQueuedFlag | kAsyncReadingFlag;
}

FileController::FileController() {
    thread_.CreateThread(&FileController::ThreadFunction, this, 0x1F, 0x1000);
}

void FileController::UnlinkCache(FileObject* object) {
    if (object == nullptr || object->cacheOwner_ != this) {
        return;
    }
    if (object->cachePrevious_ != nullptr) {
        object->cachePrevious_->cacheNext_ = object->cacheNext_;
    } else {
        cacheHead_ = object->cacheNext_;
    }
    if (object->cacheNext_ != nullptr) {
        object->cacheNext_->cachePrevious_ = object->cachePrevious_;
    } else {
        cacheTail_ = object->cachePrevious_;
    }
    object->cachePrevious_ = nullptr;
    object->cacheNext_ = nullptr;
    object->cacheOwner_ = nullptr;
    --cacheCount_;
}

void FileController::AppendCache(FileObject* object) {
    if (object == nullptr) {
        return;
    }
    object->cachePrevious_ = cacheTail_;
    object->cacheNext_ = nullptr;
    object->cacheOwner_ = this;
    if (cacheTail_ != nullptr) {
        cacheTail_->cacheNext_ = object;
    } else {
        cacheHead_ = object;
    }
    cacheTail_ = object;
    ++cacheCount_;
}

void FileController::TouchObject(FileObject* object) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (object == nullptr || object->cacheOwner_ != this || cacheTail_ == object) {
        return;
    }
    UnlinkCache(object);
    AppendCache(object);
}

void FileController::RegisterObject(FileObject* object) {
    if (object == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (object->cacheOwner_ == this) {
        UnlinkCache(object);
    }
    AppendCache(object);
    index_.Set(object->path_.data(), object);
}

void FileController::UnlinkAsync(FileObject* object) {
    if (object == nullptr || object->asyncOwner_ != this) {
        return;
    }
    if (object->asyncPrevious_ != nullptr) {
        object->asyncPrevious_->asyncNext_ = object->asyncNext_;
    } else {
        asyncHead_ = object->asyncNext_;
    }
    if (object->asyncNext_ != nullptr) {
        object->asyncNext_->asyncPrevious_ = object->asyncPrevious_;
    } else {
        asyncTail_ = object->asyncPrevious_;
    }
    object->asyncPrevious_ = nullptr;
    object->asyncNext_ = nullptr;
    object->asyncOwner_ = nullptr;
    --asyncCount_;
}

void FileController::PrioritizeAsync(FileObject* object) {
    if (object == nullptr || object->asyncOwner_ != this || asyncHead_ == object) {
        return;
    }

    UnlinkAsync(object);

    FileObject* insertBefore = asyncHead_;
    if (insertBefore != nullptr &&
        (insertBefore->flags_ & kAsyncReadingFlag) != 0) {
        insertBefore = insertBefore->asyncNext_;
    }

    if (insertBefore == nullptr) {
        object->asyncPrevious_ = asyncTail_;
        if (asyncTail_ != nullptr) {
            asyncTail_->asyncNext_ = object;
        } else {
            asyncHead_ = object;
        }
        asyncTail_ = object;
    } else {
        object->asyncNext_ = insertBefore;
        object->asyncPrevious_ = insertBefore->asyncPrevious_;
        if (insertBefore->asyncPrevious_ != nullptr) {
            insertBefore->asyncPrevious_->asyncNext_ = object;
        } else {
            asyncHead_ = object;
        }
        insertBefore->asyncPrevious_ = object;
    }
    object->asyncOwner_ = this;
    ++asyncCount_;
}

FileObject* FileController::SelectHighestPriorityAsync() {
    FileObject* selected = nullptr;
    unsigned int best = 0;
    for (FileObject* object = asyncHead_;
         object != nullptr;
         object = object->asyncNext_) {
        if (selected == nullptr || object->priority_ > best) {
            selected = object;
            best = object->priority_;
        }
    }
    return selected;
}

void FileController::FinishAsync(FileObject* object) {
    if (object == nullptr) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        PrioritizeAsync(object);
    }

    thread_.CurrentPriority();
    fates::decomp_detail::SignalFileControllerWorker();
    while ((object->flags_ & kAsyncMask) != 0) {
        fates::decomp_detail::SleepFileControllerPoll();
    }
    thread_.ResumePriority();
    fates::decomp_detail::SignalFileControllerWorker();
}

void FileController::ThreadFunction(void* controllerPointer) {
    auto* const controller = static_cast<FileController*>(controllerPointer);
    if (controller == nullptr) {
        return;
    }

    while (controller->thread_.IsRunning()) {
        FileObject* object = nullptr;
        {
            std::lock_guard<std::mutex> lock(controller->mutex_);
            object = controller->SelectHighestPriorityAsync();
            if (object != nullptr) {
                object->flags_ |= kAsyncReadingFlag;
            }
        }

        if (object == nullptr) {
            fates::decomp_detail::WaitFileControllerWorker();
            continue;
        }

        const auto result = File::TryDirectRead(
            object->path_.data(),
            object->GetAllocator(),
            object->GetAlign());
        object->data_ = result.data;
        object->size_ = result.size;

        {
            std::lock_guard<std::mutex> lock(controller->mutex_);
            controller->UnlinkAsync(object);
            object->flags_ &= ~kAsyncMask;
            object->flags_ |= kAsyncCompleteFlag;
        }
        fates::decomp_detail::YieldFileControllerWorker();
    }
}

bool FileController::DeleteObject(FileObject* object) {
    if (object == nullptr) {
        return true;
    }

    if ((object->flags_ & kAsyncMask) != 0) {
        std::lock_guard<std::mutex> lock(mutex_);
        if ((object->flags_ & kAsyncReadingFlag) != 0) {
            return false;
        }
        if ((object->flags_ & kAsyncQueuedFlag) != 0) {
            UnlinkAsync(object);
        }
    }

    index_.Delete(object->path_.data());
    {
        std::lock_guard<std::mutex> lock(mutex_);
        UnlinkCache(object);
    }
    delete object;
    return true;
}

void FileController::DelayCallback(void* pointer) {
    auto* const object = static_cast<FileObject*>(pointer);
    if (object == nullptr) {
        return;
    }

    object->bindCount_ = fates::io::FileReleaseReference(object->bindCount_);
    if (object->bindCount_ != 0) {
        return;
    }

    // The original captures the current controller before virtual Cleanup.
    FileController* const controller = fates::decomp_detail::GetFileController();
    if ((object->flags_ & kCleanupPendingFlag) != 0) {
        if (object->data_ != nullptr) {
            object->Cleanup();
        }
        object->flags_ &= ~kCleanupPendingFlag;
    }

    if (fates::io::FileKeepAfterRelease(object->data_ != nullptr,
            object->cacheLevel_, object->flags_)) {
        return;
    }

    if (controller != nullptr) {
        controller->DeleteObject(object);
    }
}

void FileController::Sweep(int bytesToFree) {
    for (unsigned int cacheLevel = 0; cacheLevel < 4 && bytesToFree > 0; ++cacheLevel) {
        FileObject* object = cacheHead_;
        while (object != nullptr && bytesToFree > 0) {
            FileObject* const next = object->cacheNext_;
            if (fates::io::FileSweepEligible(object->cacheLevel_,
                    object->bindCount_, object->flags_, cacheLevel)) {
                bytesToFree = fates::io::FileSweepSubtract(bytesToFree, object->size_);
                DeleteObject(object);
            }
            object = next;
        }
    }
}

void FileController::RemoveAsync(FileObject* object) {
    std::lock_guard<std::mutex> lock(mutex_);
    UnlinkAsync(object);
}

bool FileController::IsExist(const char* path) const {
    if (path == nullptr) {
        return false;
    }
    if (index_.GetSurely(path) != nullptr) {
        return true;
    }

    const char* probePath = path;
    if (FileMount::Node* const mount = mount_.Find(path)) {
        probePath = mount->GetHookPath();
    }
    return fates::decomp_detail::ProbeFileSystemPathExists(probePath);
}


void FileController::FinishAllAsync() {
    for (;;) {
        FileObject* pending = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (FileObject* object = cacheHead_;
                 object != nullptr;
                 object = object->cacheNext_) {
                if ((object->flags_ & kAsyncMask) != 0) {
                    pending = object;
                    break;
                }
            }
        }

        if (pending == nullptr) {
            return;
        }
        FinishAsync(pending);
    }
}

bool FileController::HasAsyncLoading() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return asyncCount_ > 0;
}

FileObject* FileController::FindObject(const char* path) const {
    if (path == nullptr) {
        return nullptr;
    }
    return static_cast<FileObject*>(index_.GetSurely(path));
}

void FileController::QueueAsyncObject(FileObject* object) {
    if (object == nullptr) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (object->asyncOwner_ == this) {
            return;
        }
        object->asyncPrevious_ = asyncTail_;
        object->asyncNext_ = nullptr;
        object->asyncOwner_ = this;
        if (asyncTail_ != nullptr) {
            asyncTail_->asyncNext_ = object;
        } else {
            asyncHead_ = object;
        }
        asyncTail_ = object;
        ++asyncCount_;
    }
    fates::decomp_detail::SignalFileControllerWorker();
}
