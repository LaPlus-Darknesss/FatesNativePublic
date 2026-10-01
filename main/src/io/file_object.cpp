#include "fates/io/file_object.hpp"

#include "fates/detail/file_controller_runtime.hpp"
#include "fates/io/file_controller.hpp"
#include "fates/io/file.hpp"
#include "fates/memory/iallocator.hpp"

#include <cstring>

namespace {
constexpr std::uint32_t kSkipDirectReadFlag = 0x02000000u;
constexpr std::uint32_t kResizedFlag = 0x10000000u;
}

FileObject::FileObject() = default;

FileObject::~FileObject() {
    if (dataAllocator_ != nullptr) {
        dataAllocator_->Free(data_);
    }
    dataAllocator_ = nullptr;
    data_ = nullptr;
    size_ = 0;
}

void FileObject::SetPath(const char* path) {
    path_.fill('\0');
    if (path != nullptr) {
        std::strncpy(path_.data(), path, path_.size());
        path_.back() = '\0';
    }
}

void FileObject::Touch() {
    if (FileController* const controller = fates::decomp_detail::GetFileController()) {
        controller->TouchObject(this);
    }
}

bool FileObject::ReadBinary(
    const char* path,
    const void* source,
    unsigned int size) {
    if ((flags_ & kSkipDirectReadFlag) == 0) {
        dataAllocator_ = GetAllocator();
        const auto result = File::DirectReadBinary(
            path,
            source,
            size,
            dataAllocator_,
            GetAlign());
        data_ = result.data;
        size_ = result.size;
    } else {
        data_ = const_cast<void*>(source);
        size_ = 0;
    }

    path_.fill('\0');
    if (path != nullptr) {
        std::strncpy(path_.data(), path, path_.size());
        path_.back() = '\0';
    }

    if (FileController* const controller = fates::decomp_detail::GetFileController()) {
        controller->RegisterObject(this);
    }
    return size_ != 0;
}

void FileObject::Resize(unsigned int size) {
    if (dataAllocator_ != nullptr && dataAllocator_->Resize(data_, size)) {
        size_ = size;
        flags_ |= kResizedFlag;
    }
}

void FileObject::Sweep() {
    IAllocator* const allocator = GetAllocator();
    FileController* const controller = fates::decomp_detail::GetFileController();
    if (allocator == nullptr || controller == nullptr) {
        return;
    }

    const int budget = fates::decomp_detail::ComputeFileObjectSweepBudget(*allocator);
    if (budget > 0) {
        controller->Sweep(budget);
    }
}
