#include "fates/io/res_object.hpp"

#include "fates/detail/resource_runtime.hpp"
#include "fates/io/delay_manager.hpp"

ResObject::ResObject() = default;

// Retail's source-level ResObject destructor is a no-op veneer into the
// FileObject base destructor. Resource cleanup is an explicit virtual Cleanup
// operation performed by FileBase before deletion.
ResObject::~ResObject() = default;

void ResObject::Link(nw::h3d::res::ResourceBinary* resourceBinary) {
    if (!IsResourceSetup() || resourceBinary == nullptr) {
        return;
    }
    fates::decomp_detail::LinkResourceContents(resourceState_, resourceBinary);
}

void ResObject::Setup() {
    if (IsResourceSetup()) {
        return;
    }

    const void* const data = GetData();
    if (data == nullptr || !fates::decomp_detail::HasSupportedResourceHeader(data)) {
        return;
    }

    resourceState_.flags = (GetFlags() & 0xFFFFu) |
        fates::decomp_detail::kResourceSetupFlag;
    fates::decomp_detail::PrepareResourceStorage(resourceState_, data, GetFlags());

    if (!fates::decomp_detail::TryInitializeResourceBinary(resourceState_, data)) {
        return;
    }

    Link(resourceState_.resourceBinary);
    fates::decomp_detail::FinalizeResourcePlacement(resourceState_, data);

    if ((resourceState_.flags &
         fates::decomp_detail::kResourceRelocationRequestedFlag) != 0) {
        if (GetRelocatableSize() != 0) {
            DelayManager::Entry(
                &fates::decomp_detail::FinishResourceRelocation,
                this);
        }
        resourceState_.flags |=
            fates::decomp_detail::kResourceDelayedRelocationFlag;
    }
}

void ResObject::Cleanup() {
    if (!IsResourceSetup()) {
        return;
    }

    if ((resourceState_.flags &
         fates::decomp_detail::kResourceDelayedRelocationFlag) == 0) {
        fates::decomp_detail::TryUninitializeResourceBinary(resourceState_);
    }
    fates::decomp_detail::ReleaseResourceStorage(resourceState_);
    resourceState_.flags &= ~fates::decomp_detail::kResourceSetupFlag;
}

IAllocator* ResObject::GetAllocator() const {
    return fates::decomp_detail::GetResourceAllocator();
}

unsigned int ResObject::GetRelocatableSize() const {
    if ((resourceState_.flags &
         fates::decomp_detail::kResourceRelocationRequestedFlag) == 0) {
        return 0;
    }
    return fates::decomp_detail::GetRelocatableResourceSize(
        resourceState_,
        GetData());
}

bool ResObject::IsDelay() const {
    return true;
}

unsigned int ResObject::GetAlign() const {
    return 0x80;
}

bool ResObject::IsResourceSetup() const {
    return (resourceState_.flags & fates::decomp_detail::kResourceSetupFlag) != 0;
}

const fates::decomp_detail::ResourceObjectState& ResObject::ResourceState() const {
    return resourceState_;
}

fates::decomp_detail::ResourceObjectState& ResObject::ResourceState() {
    return resourceState_;
}
