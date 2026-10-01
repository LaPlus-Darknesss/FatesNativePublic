#pragma once

#include "fates/io/file_base.hpp"

class ArchiveFile : public FileBase {
public:
    ArchiveFile();
    explicit ArchiveFile(const char* path);
    ~ArchiveFile();
};
