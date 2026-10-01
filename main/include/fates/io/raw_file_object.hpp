#pragma once

#include "fates/io/file_object.hpp"

class IAllocator;

class RawFileObject : public FileObject {
public:
    ~RawFileObject() override = default;

    void Setup() override;
    void Cleanup() override;
    IAllocator* GetAllocator() const override;
    bool IsDelay() const override;
    unsigned int GetAlign() const override;
};
