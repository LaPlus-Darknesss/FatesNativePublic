#pragma once

class FileMount;
class IAllocator;

class File {
public:
    struct ReadResult {
        unsigned int size{};
        void* data{};
    };

    static void Initialize();
    static void FinishAsync();
    static void MountRom();

    static ReadResult DirectRead(
        const char* path,
        IAllocator* allocator,
        unsigned int alignment);
    static ReadResult TryDirectRead(
        const char* path,
        IAllocator* allocator,
        unsigned int alignment);
    static void DirectFree(void* memory, IAllocator* allocator);
    static bool IsAsyncLoading();
    static ReadResult DirectReadBinary(
        const char* path,
        const void* source,
        unsigned int size,
        IAllocator* allocator,
        unsigned int alignment);

    static FileMount* GetMount();
    static const char* GetHookPath(const char* path);
    static bool IsMountPath(const char* path);
    static void MountPath(const char* path, const char* hookPath, int id);
    static void UnmountPath(int id);
    static void Sweep();
    static bool IsExist(const char* path);

    static wchar_t* GetFullPath(
        wchar_t* destination,
        unsigned int destinationCapacity,
        const char* path);
};
