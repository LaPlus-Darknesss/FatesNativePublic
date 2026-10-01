#pragma once

#include <cstdint>

class FileObject;
class PackageFile;

namespace FileCache {
enum class Level : std::uint8_t {};
}

namespace FilePrior {
enum class Level : std::uint8_t {};
}

class FileBase {
public:
    enum class FileType : int {
        Missing = 0,
        Attached = 1,
        AlreadyAttached = 2,
        PackageUnavailable = 3,
    };

    enum class ReadType : int {
        Immediate = 0,
        Asynchronous = 1,
        TryImmediate = 2,
    };

    FileType OpenFile(const char* path, ReadType readType);
    FileType OpenBinary(const char* path);
    FileType OpenPackage(const PackageFile& package, const char* identifier);

    bool CloseBinary(FileType fileType);
    bool CloseFile(FileType fileType, ReadType readType);
    void ClosePackage(FileType fileType);

    void EntryFile(
        FileObject* object,
        const char* path,
        unsigned int flags,
        ReadType readType);
    bool EntryBinary(
        FileObject* object,
        const char* path,
        const void* source,
        unsigned int size,
        unsigned int flags);
    void EntryPackage(
        FileObject* object,
        const PackageFile& package,
        const char* identifier,
        unsigned int flags);

    void FinishAsync();
    bool TryFinishAsync();
    void Free();

    void SetFileCache(FileCache::Level level);
    void SetFilePrior(FilePrior::Level level);
    void ReplaceHandle(const FileBase& source);

    void* GetFileData();
    const char* GetFilePath() const;
    unsigned int GetFileSize() const;
    bool IsAsyncLoading() const;
    bool IsDone() const;

protected:
    FileObject* GetFileObject() const { return object_; }

private:
    FileType OpenExistingPath(const char* path);
    bool EnsureReady();

    FileObject* object_{};
};
