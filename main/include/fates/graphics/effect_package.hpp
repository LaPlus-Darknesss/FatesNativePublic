#pragma once
#include "fates/io/file_base.hpp"
class EffectPackage : public FileBase {
public:
    EffectPackage() = default;
    ~EffectPackage();
    bool ReadSync(const char* path);
    void ReadAsync(const char* path);
    void Free();
};
using EffectFile = EffectPackage;
