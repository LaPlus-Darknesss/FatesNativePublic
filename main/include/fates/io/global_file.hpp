#pragma once

class GlobalFile {
public:
    static void Initialize();

    // Return types are reconstructed from retail tail-calls to
    // FileBase::GetFileData; StackTrace does not encode return types.
    static void* FileLoad(const char* path, unsigned int priority);
    static void FileFree(const char* path);

    static void* ArchiveLoad(const char* path, unsigned int priority);
    static void ArchiveFree(const char* path);

    static bool IsFileExist(const char* path);
};
