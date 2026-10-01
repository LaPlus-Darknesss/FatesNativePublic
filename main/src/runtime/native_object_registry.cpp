#include "fates/runtime/native_object_registry.hpp"
#include <stdexcept>

namespace fates::runtime::native {
ObjectIdentity ObjectHandleRegistry::NewIdentity() {
    if(last_identity_==UINT64_MAX)throw std::overflow_error("Native object identity exhausted");
    return {++last_identity_};
}
namespace {
void CheckIndex(ObjectHandle handle) {
    if(handle.Index()>=ObjectHandleRegistry::Capacity)
        throw std::invalid_argument("Object handle index outside original pool");
}
}
ObjectHandleRegistry::ObjectHandleRegistry() {
    for(std::size_t i=0;i<Capacity;++i) {
        entries_[i].handle.value=static_cast<std::uint32_t>(i);
        free_[i]=static_cast<std::uint16_t>(i);
    }
}
void ObjectHandleRegistry::Entry(ObjectHandle& owner,ObjectIdentity identity) {
    if(owner)return;
    if(!identity)throw std::invalid_argument("Object identity must be present");
    if(used_==Capacity){owner={};return;}
    const auto index=free_[used_++];
    generation_=static_cast<std::uint16_t>(generation_+1u);
    if(generation_==0)generation_=1;
    auto& entry=entries_[index];
    entry.handle.value=std::uint32_t(index)|(std::uint32_t(generation_)<<12);
    entry.identity=identity;owner=entry.handle;
}
void ObjectHandleRegistry::Remove(ObjectHandle& owner) {
    if(!owner)return;
    CheckIndex(owner);auto& entry=entries_[owner.Index()];
    if(entry.handle.Generation()==owner.Generation()) {
        entry.identity={};entry.handle.value=owner.Index();
        if(used_)free_[--used_]=owner.Index();
    }
    owner={};
}
ObjectIdentity ObjectHandleRegistry::Get(ObjectHandle handle) const {
    CheckIndex(handle);const auto& entry=entries_[handle.Index()];
    return entry.handle.Generation()==handle.Generation()?entry.identity:ObjectIdentity{};
}
}
