#pragma once

#include "fates/detail/resource_runtime.hpp"
#include "fates/io/file_object.hpp"

namespace nw::h3d::res {
struct ResourceBinary;
}

class ResObject : public FileObject {
public:
    ResObject();
    ~ResObject() override;

    void Link(nw::h3d::res::ResourceBinary* resourceBinary);

    void Setup() override;
    void Cleanup() override;
    IAllocator* GetAllocator() const override;
    bool IsDelay() const override;
    unsigned int GetAlign() const override;

    unsigned int GetRelocatableSize() const;

protected:
    bool IsResourceSetup() const;
    const fates::decomp_detail::ResourceObjectState& ResourceState() const;
    fates::decomp_detail::ResourceObjectState& ResourceState();

private:
    fates::decomp_detail::ResourceObjectState resourceState_{};
};
