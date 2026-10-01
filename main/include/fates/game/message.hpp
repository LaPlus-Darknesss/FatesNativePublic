#pragma once

class Mess {
public:
    static void Initialize();

    static bool IsFileLoad(const char* archiveName);
    static void FileLoad(const char* archiveName);
    static void FileFree(const char* archiveName);
    static void FileLoadRoute(const char* archiveName);
    static void FileFreeRoute(const char* archiveName);
    static bool IsFileExistRoute(const char* archiveName);

    static void SetArgument(int index, const wchar_t* text);
    static void SetArgument(int index, int value);
    static void SetLinkName(int index, const wchar_t* text);

    static const wchar_t* GetNoReplace(const char* identifier);
    static const wchar_t* Get(const char* identifier);
    static bool IsExist(const char* identifier);

    static int GetBufferSize();
    static wchar_t* GetArgumentBuffer(int index);
    static int GetArgumentBufferSize(int index);
    static wchar_t* GetBuffer();
    static void SetBuffer(wchar_t* buffer, int size);

    static void Finalize();
};
