#pragma once
#include <cstddef>

class Path {
public:
    static bool IsCompressPath(const char* path);
    static bool IsRootPath(const char* path);
    static bool IsRootPath(const wchar_t* path);
    static char* GetDirName(char* destination, unsigned int capacity, const char* path);
    static const char* GetExtension(const char* path);
    static char* GetNameNoExt(char* destination, unsigned int capacity, const char* path);
    static int GetPathNumber(const char* path);
};
