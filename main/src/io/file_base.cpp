#include "fates/io/file_base.hpp"

#include "fates/detail/file_controller_runtime.hpp"
#include "fates/io/delay_manager.hpp"
#include "fates/io/file_controller.hpp"
#include "fates/io/file.hpp"
#include "fates/io/file_object.hpp"
#include "fates/io/package_file.hpp"
#include "fates/io/file_cache_policies.hpp"

#include <cstdint>
#include <cstring>

namespace {
constexpr std::uint32_t kSkipDirectReadFlag = 0x02000000u;
constexpr std::uint32_t kSetupCompleteFlag = 0x04000000u;
constexpr std::uint32_t kDeleteCondition08000000 = 0x08000000u;
constexpr std::uint32_t kDeleteCondition10000000 = 0x10000000u;
constexpr std::uint32_t kAsyncQueuedFlag = 0x20000000u;
constexpr std::uint32_t kAsyncReadingFlag = 0x40000000u;
constexpr std::uint32_t kAsyncCompleteFlag = 0x80000000u;
constexpr std::uint32_t kAsyncMask = kAsyncQueuedFlag | kAsyncReadingFlag;

} // namespace

FileBase::FileType FileBase::OpenExistingPath(const char* path) {
    FileController* const controller = fates::decomp_detail::GetFileController();
    FileObject* const found = controller != nullptr ? controller->FindObject(path) : nullptr;
    if (found == nullptr) {
        Free();
        return FileType::Missing;
    }

    found->Touch();
    if (object_ == found) {
        return FileType::AlreadyAttached;
    }

    Free();
    object_ = found;
    return FileType::Attached;
}

FileBase::FileType FileBase::OpenFile(const char* path, ReadType) {
    return OpenExistingPath(path);
}

FileBase::FileType FileBase::OpenBinary(const char* path) {
    return OpenExistingPath(path);
}

FileBase::FileType FileBase::OpenPackage(
    const PackageFile& package,
    const char* identifier) {
    if (!package.IsDone()) {
        return FileType::PackageUnavailable;
    }

    char path[0x50]{};
    package.GetPath(path, sizeof(path), identifier);
    return OpenExistingPath(path);
}

bool FileBase::EnsureReady() {
    if (object_ == nullptr) {
        return false;
    }

    if ((object_->flags_ & kSetupCompleteFlag) == 0) {
        if ((object_->flags_ & kAsyncMask) != 0) {
            if (FileController* const controller = fates::decomp_detail::GetFileController()) {
                controller->FinishAsync(object_);
            }
        }

        object_->flags_ &= 0x1FFFFFFFu;
        if (object_->data_ != nullptr) {
            object_->Setup();
        }
        object_->flags_ |= kSetupCompleteFlag;
    }

    return object_->data_ != nullptr;
}

bool FileBase::CloseBinary(FileType fileType) {
    if (fileType != FileType::AlreadyAttached && object_ != nullptr) {
        ++object_->bindCount_;
        object_->Touch();
    }

    if (EnsureReady()) {
        return true;
    }
    Free();
    return false;
}

bool FileBase::CloseFile(FileType fileType, ReadType readType) {
    if (fileType != FileType::AlreadyAttached && object_ != nullptr) {
        ++object_->bindCount_;
        object_->Touch();
    }

    if (readType == ReadType::Asynchronous) {
        return true;
    }

    if (EnsureReady()) {
        return true;
    }
    Free();
    return false;
}

void FileBase::ClosePackage(FileType fileType) {
    (void)CloseFile(fileType, ReadType::Immediate);
}

void FileBase::EntryFile(
    FileObject* object,
    const char* path,
    unsigned int flags,
    ReadType readType) {
    object_ = object;
    if (object_ == nullptr) {
        return;
    }

    object_->flags_ |= flags;
    object_->Sweep();
    object_->dataAllocator_ = object_->GetAllocator();
    object_->SetPath(path);

    FileController* const controller = fates::decomp_detail::GetFileController();
    if (readType == ReadType::Asynchronous) {
        object_->data_ = nullptr;
        object_->size_ = 0;
        object_->flags_ |= kAsyncQueuedFlag;
        if (controller != nullptr) {
            controller->RegisterObject(object_);
            controller->QueueAsyncObject(object_);
        }
        return;
    }

    File::ReadResult result{};
    if (readType == ReadType::TryImmediate) {
        result = File::TryDirectRead(
            path,
            object_->dataAllocator_,
            object_->GetAlign());
    } else {
        result = File::DirectRead(
            path,
            object_->dataAllocator_,
            object_->GetAlign());
    }

    object_->data_ = result.data;
    object_->size_ = result.size;
    if (controller != nullptr) {
        controller->RegisterObject(object_);
    }
}

bool FileBase::EntryBinary(
    FileObject* object,
    const char* path,
    const void* source,
    unsigned int size,
    unsigned int flags) {
    object_ = object;
    if (object_ == nullptr) {
        return false;
    }
    object_->flags_ |= flags;
    object_->Sweep();
    return object_->ReadBinary(path, source, size);
}

void FileBase::EntryPackage(
    FileObject* object,
    const PackageFile& package,
    const char* identifier,
    unsigned int flags) {
    const PackageFileInfo* const info = package.GetInfo(identifier);
    if (info == nullptr) {
        return;
    }

    char path[0x50]{};
    package.GetPath(path, sizeof(path), identifier);
    (void)EntryBinary(
        object,
        path,
        package.GetData(info->entryIndex),
        package.GetSize(info->entryIndex),
        flags);
}

void FileBase::FinishAsync() {
    if (object_ == nullptr) {
        return;
    }
    if (!EnsureReady()) {
        Free();
    }
}

bool FileBase::TryFinishAsync() {
    if (object_ == nullptr) {
        return false;
    }
    if (EnsureReady()) {
        return true;
    }
    Free();
    return false;
}

void FileBase::Free() {
    FileObject* const object = object_;
    if (object == nullptr) {
        return;
    }

    object->bindCount_ = fates::io::FileReleaseReference(object->bindCount_);
    if (object->bindCount_ == 0) {
        if (!object->IsDelay()) {
            if ((object->flags_ & kSetupCompleteFlag) != 0) {
                if (object->data_ != nullptr) {
                    object->Cleanup();
                }
                object->flags_ &= ~kSetupCompleteFlag;
            }

            if (object->data_ == nullptr || object->cacheLevel_ == 0 ||
                (object->flags_ & kDeleteCondition08000000) != 0 ||
                (object->flags_ & kDeleteCondition10000000) != 0) {
                if (FileController* const controller = fates::decomp_detail::GetFileController()) {
                    controller->DeleteObject(object);
                }
            }
        } else {
            ++object->bindCount_;
            object->Touch();
            DelayManager::Entry(&FileController::DelayCallback, object);
        }
    }

    object_ = nullptr;
}

void FileBase::SetFileCache(FileCache::Level level) {
    if (object_ != nullptr) {
        object_->cacheLevel_ = static_cast<std::uint8_t>(level);
    }
}

void FileBase::SetFilePrior(FilePrior::Level level) {
    if (object_ != nullptr) {
        object_->priority_ = static_cast<std::uint8_t>(level);
    }
}

void FileBase::ReplaceHandle(const FileBase& source) {
    if (object_ == source.object_) {
        return;
    }

    Free();
    if (source.object_ == nullptr) {
        return;
    }

    object_ = source.object_;
    ++object_->bindCount_;
    object_->Touch();
    (void)EnsureReady();
}

void* FileBase::GetFileData() {
    return object_ != nullptr ? object_->data_ : nullptr;
}

const char* FileBase::GetFilePath() const {
    static constexpr char kEmpty[] = "";
    return object_ != nullptr ? object_->path_.data() : kEmpty;
}

unsigned int FileBase::GetFileSize() const {
    return object_ != nullptr ? object_->size_ : 0;
}

bool FileBase::IsAsyncLoading() const {
    return object_ != nullptr && (object_->flags_ & kAsyncMask) != 0;
}

bool FileBase::IsDone() const {
    return object_ != nullptr && (object_->flags_ & kSetupCompleteFlag) != 0;
}
