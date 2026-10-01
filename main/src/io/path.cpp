#include "fates/io/path.hpp"

#include <algorithm>
#include <cstring>
#include <cwchar>

namespace {

template <typename CharT>
bool IsRootPathImpl(const CharT* path) {
    if (path == nullptr) {
        return false;
    }
    for (const CharT* p = path; *p != 0; ++p) {
        if (*p == static_cast<CharT>(':')) {
            return true;
        }
        if (*p == static_cast<CharT>('/') || *p == static_cast<CharT>('\\')) {
            return false;
        }
    }
    return false;
}

void CopyBounded(char* destination, unsigned int capacity, const char* source, std::size_t length) {
    if (destination == nullptr || capacity == 0) {
        return;
    }
    const std::size_t count = std::min<std::size_t>(length, capacity);
    std::memcpy(destination, source, count);
    if (count < capacity) {
        destination[count] = '\0';
    } else {
        destination[capacity - 1] = '\0';
    }
}

} // namespace

bool Path::IsCompressPath(const char* path) {
    if (path == nullptr) {
        return false;
    }
    const char* dot = std::strrchr(path, '.');
    return dot != nullptr && std::strcmp(dot + 1, "lz") == 0;
}

bool Path::IsRootPath(const char* path) {
    return IsRootPathImpl(path);
}

bool Path::IsRootPath(const wchar_t* path) {
    return IsRootPathImpl(path);
}

char* Path::GetDirName(char* destination, unsigned int capacity, const char* path) {
    if (destination == nullptr || capacity == 0 || path == nullptr) {
        return destination;
    }

    // Retail tracks the final two separators. If fewer than two are present,
    // the same bounded-copy behavior naturally falls back toward the input.
    const char* previous = path;
    const char* last = path;
    for (const char* p = path; *p != '\0'; ++p) {
        if (*p == '/' || *p == '\\') {
            previous = last;
            last = p;
        }
    }

    if (previous == last) {
        CopyBounded(destination, capacity, path, std::strlen(path));
    } else {
        const std::size_t length = static_cast<std::size_t>(last - previous);
        CopyBounded(destination, capacity, previous + 1, length);
    }
    return destination;
}

const char* Path::GetExtension(const char* path) {
    if (path == nullptr) {
        return nullptr;
    }
    const char* result = path;
    for (const char* p = path; *p != '\0'; ++p) {
        if (*p == '.') {
            result = p + 1;
        }
    }
    return result;
}

char* Path::GetNameNoExt(char* destination, unsigned int capacity, const char* path) {
    if (destination == nullptr || capacity == 0 || path == nullptr) {
        return destination;
    }
    const char* name = path;
    for (const char* p = path; *p != '\0'; ++p) {
        if (*p == '/' || *p == '\\') {
            name = p + 1;
        }
    }
    CopyBounded(destination, capacity, name, std::strlen(name));
    if (char* dot = std::strchr(destination, '.')) {
        *dot = '\0';
    }
    return destination;
}

int Path::GetPathNumber(const char* path) {
    if (path == nullptr) {
        return 0;
    }
    const char* name = path;
    for (const char* p = path; *p != '\0'; ++p) {
        if (*p == '/' || *p == '\\') {
            name = p + 1;
        }
    }
    int value = 0;
    for (const char* p = name; *p != '\0' && *p != '.'; ++p) {
        if (*p >= '0' && *p <= '9') {
            value = value * 10 + (*p - '0');
        }
    }
    return value;
}
