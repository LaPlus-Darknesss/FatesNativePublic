#pragma once
#include <cstdint>
namespace fates::io::native {
// A carried, nonnull resource identity resolved by its concrete callback owner.
// The word belongs to that owner's address domain; it is not a host pointer or
// an automatically generated slot number. A zero target is an unknown method.
struct ResourceDeletionReference {
    std::uint32_t object_word{},deleting_destructor{};
    bool operator==(const ResourceDeletionReference&) const = default;
};
}
