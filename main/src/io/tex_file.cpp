#include "fates/io/tex_file.hpp"

#include "fates/graphics/i_texture.hpp"
#include "fates/io/package_file.hpp"
#include "fates/io/tex_object.hpp"

namespace {

FileObject* NewTexResourceObject() {
    return new TexObject();
}

const TexObject* AsTexObject(const FileObject* object) {
    return static_cast<const TexObject*>(object);
}

} // namespace

TexFile::TexFile() = default;

TexFile::TexFile(const char* path, unsigned int flags) {
    (void)Read(path, flags);
}

TexFile::~TexFile() {
    Free();
}

bool TexFile::Read(const char* path, unsigned int flags) {
    const FileType fileType = OpenFile(path, ReadType::Immediate);
    if (fileType == FileType::Missing) {
        EntryFile(NewTexResourceObject(), path, flags, ReadType::Immediate);
    }
    return CloseFile(fileType, ReadType::Immediate);
}

void TexFile::ReadPackage(
    const PackageFile& package,
    const char* identifier,
    unsigned int flags) {
    const FileType fileType = OpenPackage(package, identifier);
    if (fileType == FileType::Missing) {
        EntryPackage(NewTexResourceObject(), package, identifier, flags);
    }
    ClosePackage(fileType);
}

bool TexFile::TryRead(const char* path, unsigned int flags) {
    const FileType fileType = OpenFile(path, ReadType::TryImmediate);
    if (fileType == FileType::Missing) {
        EntryFile(NewTexResourceObject(), path, flags, ReadType::TryImmediate);
    }
    return CloseFile(fileType, ReadType::TryImmediate);
}

bool TexFile::ReadAsync(const char* path, unsigned int flags) {
    const FileType fileType = OpenFile(path, ReadType::Asynchronous);
    if (fileType == FileType::Missing) {
        EntryFile(NewTexResourceObject(), path, flags, ReadType::Asynchronous);
    }
    return CloseFile(fileType, ReadType::Asynchronous);
}

const ITexture* TexFile::GetTexture(const char* identifier) const {
    const FileObject* const object = GetFileObject();
    if (object != nullptr && identifier != nullptr) {
        if (const ITexture* const texture = AsTexObject(object)->FindTexture(identifier)) {
            return texture;
        }
    }
    return ITexture::GetDummyTex();
}

const ITexture* TexFile::GetTexture(int index) const {
    if (index != 0xFFFF) {
        const FileObject* const object = GetFileObject();
        if (object != nullptr) {
            if (const ITexture* const texture = AsTexObject(object)->GetTexture(index)) {
                return texture;
            }
        }
    }
    return ITexture::GetDummyTex();
}

int TexFile::GetTextureCount() const {
    const FileObject* const object = GetFileObject();
    return object != nullptr ? AsTexObject(object)->GetTextureCount() : 0;
}
