#include "fates/io/file.hpp"

#include "fates/detail/file_controller_runtime.hpp"
#include "fates/detail/file_platform_runtime.hpp"
#include "fates/detail/file_registry_runtime.hpp"
#include "fates/engine/sign.hpp"
#include "fates/io/compress.hpp"
#include "fates/io/file_controller.hpp"
#include "fates/io/file_mount.hpp"
#include "fates/io/path.hpp"
#include "fates/memory/iallocator.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cwchar>

namespace fates::decomp_detail {
extern FileMount* gFileMount;
}

namespace {

unsigned int Align4(unsigned int value) {
    return (value + 3u) & ~3u;
}

unsigned short AllocationAlignment(unsigned int alignment) {
    return static_cast<unsigned short>(Align4(alignment));
}

const std::uint8_t* Bytes(const void* source) {
    return static_cast<const std::uint8_t*>(source);
}

std::uint8_t* Bytes(void* destination) {
    return static_cast<std::uint8_t*>(destination);
}

} // namespace

void File::Initialize() {
    fates::decomp_detail::InitializePlatformFileSystem();
    sign::Initialize();

    auto* const controller = new FileController();
    fates::decomp_detail::InstallFileController(controller);

    // Retail then applies nn::cfg filesystem-latency emulation and debug-mode
    // state. Keep that CTR-specific plumbing behind the platform boundary.
    fates::decomp_detail::ConfigurePlatformFileLatency();
}

void File::FinishAsync() {
    if (FileController* const controller = fates::decomp_detail::GetFileController()) {
        controller->FinishAllAsync();
    }
}

void File::MountRom() {
    fates::decomp_detail::MountRomFileSystem();
}

File::ReadResult File::DirectRead(
    const char* path,
    IAllocator* allocator,
    unsigned int alignment) {
    // Retail 0x0011B290 is a thin wrapper around TryDirectRead.
    return TryDirectRead(path, allocator, alignment);
}

File::ReadResult File::TryDirectRead(
    const char* path,
    IAllocator* allocator,
    unsigned int alignment) {
    std::array<wchar_t, 0x50> fullPath{};
    GetFullPath(fullPath.data(), fullPath.size(), path);

    if (allocator == nullptr) {
        allocator = fates::decomp_detail::GetDefaultFileAllocator();
    }
    if (allocator == nullptr) {
        return {};
    }

    unsigned int fileSize = 0;
    if (!fates::decomp_detail::TryGetPlatformFileSize(
            fullPath.data(), fileSize)) {
        return {};
    }

    const unsigned short allocAlign = AllocationAlignment(alignment);

    if (!Path::IsCompressPath(path)) {
        void* const data = allocator->Malloc(fileSize, allocAlign);
        if (data == nullptr) {
            return {};
        }
        (void)fates::decomp_detail::ReadPlatformFileRange(
            fullPath.data(), 0, data, fileSize);
        return {fileSize, data};
    }

    if (fileSize < sizeof(std::uint32_t)) {
        return {};
    }

    std::uint32_t descriptor = 0;
    if (fates::decomp_detail::ReadPlatformFileRange(
            fullPath.data(), 0, &descriptor, sizeof(descriptor)) !=
        static_cast<int>(sizeof(descriptor))) {
        return {};
    }

    const unsigned int payloadSize =
        fileSize - static_cast<unsigned int>(sizeof(descriptor));

    // Retail bit 1 selects the overlap-safe layout. The outer word carries
    // the overlap allocation size; the file bytes after it begin with the
    // inner compression header that Compress::Uncompress consumes.
    if ((descriptor & 0x02u) != 0) {
        const unsigned int alignedInputSize = (fileSize - 1u) & ~3u;
        const unsigned int padding = alignedInputSize - payloadSize;
        const unsigned int allocationSize =
            Align4(Compress::GetOverlapSize(&descriptor) + padding);

        void* const data = allocator->Malloc(allocationSize, allocAlign);
        if (data == nullptr) {
            return {};
        }

        const unsigned int inputOffset = allocationSize - alignedInputSize;
        void* const compressedInput = Bytes(data) + inputOffset;
        const int read = fates::decomp_detail::ReadPlatformFileRange(
            fullPath.data(),
            sizeof(descriptor),
            compressedInput,
            payloadSize);

        unsigned int resultSize = payloadSize;
        if (read == static_cast<int>(payloadSize)) {
            resultSize = Compress::Uncompress(compressedInput, data);
        }
        return {resultSize, data};
    }

    // A zero low byte marks an uncompressed payload wrapped in the four-byte
    // .lz container header.
    if ((descriptor & 0xFFu) == 0) {
        void* const data = allocator->Malloc(payloadSize, allocAlign);
        if (data == nullptr) {
            return {};
        }
        (void)fates::decomp_detail::ReadPlatformFileRange(
            fullPath.data(),
            sizeof(descriptor),
            data,
            payloadSize);
        return {payloadSize, data};
    }

    // Ordinary compressed .lz files are read into a temporary buffer because
    // the decompressor consumes the header together with the payload.
    void* const temporary = allocator->Malloc(fileSize, 8);
    if (temporary == nullptr) {
        return {};
    }

    std::memcpy(temporary, &descriptor, sizeof(descriptor));
    const int read = fates::decomp_detail::ReadPlatformFileRange(
        fullPath.data(),
        sizeof(descriptor),
        Bytes(temporary) + sizeof(descriptor),
        payloadSize);

    ReadResult result{};
    if (read == static_cast<int>(payloadSize)) {
        const unsigned int expectedSize =
            Compress::GetUncompressSize(temporary);
        result.data = allocator->Malloc(expectedSize, allocAlign);
        if (result.data != nullptr) {
            result.size = Compress::Uncompress(temporary, result.data);
        }
    }

    allocator->Free(temporary);
    return result;
}

void File::DirectFree(void* memory, IAllocator* allocator) {
    if (allocator == nullptr) {
        allocator = fates::decomp_detail::GetDefaultFileAllocator();
    }
    if (memory != nullptr && allocator != nullptr) {
        allocator->Free(memory);
    }
}

bool File::IsAsyncLoading() {
    FileController* const controller =
        fates::decomp_detail::GetFileController();
    return controller != nullptr && controller->HasAsyncLoading();
}

File::ReadResult File::DirectReadBinary(
    const char* path,
    const void* source,
    unsigned int size,
    IAllocator* allocator,
    unsigned int alignment) {
    if (allocator == nullptr) {
        allocator = fates::decomp_detail::GetDefaultFileAllocator();
    }
    if (allocator == nullptr) {
        return {};
    }

    const unsigned short allocAlign = AllocationAlignment(alignment);

    if (!Path::IsCompressPath(path)) {
        void* const data = allocator->Malloc(size, allocAlign);
        if (data != nullptr) {
            std::memcpy(data, source, size);
        }
        return {data != nullptr ? size : 0u, data};
    }

    if (size < sizeof(std::uint32_t)) {
        return {};
    }

    const std::uint32_t descriptor =
        *static_cast<const std::uint32_t*>(source);
    const unsigned int payloadSize =
        size - static_cast<unsigned int>(sizeof(descriptor));

    if ((descriptor & 0x02u) != 0) {
        const unsigned int alignedInputSize = (size - 1u) & ~3u;
        const unsigned int padding = alignedInputSize - payloadSize;
        const unsigned int allocationSize =
            Align4(Compress::GetOverlapSize(source) + padding);

        void* const data = allocator->Malloc(allocationSize, allocAlign);
        if (data == nullptr) {
            return {};
        }
        const unsigned int actualSize =
            Compress::Uncompress(Bytes(source) + sizeof(descriptor), data);
        return {actualSize, data};
    }

    if ((descriptor & 0xFFu) != 0) {
        const unsigned int expectedSize =
            Compress::GetUncompressSize(source);
        void* const data = allocator->Malloc(expectedSize, allocAlign);
        if (data == nullptr) {
            return {};
        }
        return {Compress::Uncompress(source, data), data};
    }

    void* const data = allocator->Malloc(payloadSize, allocAlign);
    if (data != nullptr) {
        std::memcpy(
            data,
            Bytes(source) + sizeof(descriptor),
            payloadSize);
    }
    return {data != nullptr ? payloadSize : 0u, data};
}

FileMount* File::GetMount() {
    return fates::decomp_detail::gFileMount;
}

const char* File::GetHookPath(const char* path) {
    FileMount* const mount = GetMount();
    if (mount == nullptr) {
        return nullptr;
    }
    FileMount::Node* const node = mount->Find(path);
    return node != nullptr ? node->GetHookPath() : nullptr;
}

bool File::IsMountPath(const char* path) {
    FileMount* const mount = GetMount();
    return mount != nullptr && mount->IsExist(path);
}

void File::MountPath(const char* path, const char* hookPath, int id) {
    FileMount* const mount = GetMount();
    if (mount != nullptr) {
        mount->Entry(path, hookPath, id);
    }
}

void File::UnmountPath(int id) {
    FileMount* const mount = GetMount();
    if (mount != nullptr) {
        mount->Remove(id);
    }
}

wchar_t* File::GetFullPath(
    wchar_t* destination,
    unsigned int destinationCapacity,
    const char* path) {
    if (destination == nullptr || destinationCapacity == 0 || path == nullptr) {
        return destination;
    }

    if (const char* const hook = GetHookPath(path)) {
        path = hook;
    }
    if (*path == '/') {
        ++path;
    }

    unsigned int used = 0;
    if (!fates::decomp_detail::IsRootPath(path)) {
        const wchar_t* const root = fates::decomp_detail::GetRomRootPath();
        if (root != nullptr) {
            std::wcsncpy(destination, root, destinationCapacity);
            destination[destinationCapacity - 1] = L'\0';
            used = static_cast<unsigned int>(std::wcslen(destination));
            if (used >= destinationCapacity) {
                used = destinationCapacity - 1;
            }
        }
    }

    fates::decomp_detail::ShiftJisToUtf16(
        destination + used,
        destinationCapacity - used,
        path);
    return destination;
}

void File::Sweep() {
    if (FileController* const controller =
            fates::decomp_detail::GetFileController()) {
        controller->Sweep(0x40000000);
    }
}

bool File::IsExist(const char* path) {
    FileController* const controller =
        fates::decomp_detail::GetFileController();
    return controller != nullptr && controller->IsExist(path);
}
