#pragma once

#include "fates/io/file_base.hpp"

class StructFile : public FileBase {
public:
    // Retail exposes only a compiler-shaped deleting destructor entry in the
    // current symbol/evidence set. This readable destructor preserves the
    // source-level Free() behavior but is not counted as a promoted retail
    // function in Pass 16.
    ~StructFile();

    void* TryGet(const char* identifier) const;
    void* GetHeader(const char* identifier) const;
    void* GetSurely(const char* identifier) const;
};
