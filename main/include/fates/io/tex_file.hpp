#pragma once

#include "fates/io/file_base.hpp"

class ITexture;
class PackageFile;

class TexFile : public FileBase {
public:
    TexFile();
    TexFile(const char* path, unsigned int flags);
    ~TexFile();

    bool Read(const char* path, unsigned int flags);
    void ReadPackage(
        const PackageFile& package,
        const char* identifier,
        unsigned int flags);
    bool TryRead(const char* path, unsigned int flags);
    bool ReadAsync(const char* path, unsigned int flags);

    const ITexture* GetTexture(const char* identifier) const;
    const ITexture* GetTexture(int index) const;
    int GetTextureCount() const;
};
