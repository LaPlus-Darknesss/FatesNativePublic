#pragma once
#include <array>
#include <cstdint>
#include <cstddef>

namespace fates::runtime::native {
// Original GameHandle encoding. Presence tests generation, not raw nonzero.
// Stored generations are 16-bit; comparisons retain all 20 incoming high bits.
struct ObjectHandle {
    std::uint32_t value{};
    std::uint32_t Generation() const noexcept {return value>>12;}
    std::uint16_t Index() const noexcept {return std::uint16_t(value&0xfffu);}
    explicit operator bool() const noexcept {return Generation()!=0;}
    bool operator==(const ObjectHandle&) const=default;
};
// Opaque host identity supplied by the object owner, never an ARM/host address.
// The owner must keep live identities unique; zero denotes no object.
struct ObjectIdentity {
    std::uint64_t value{};
    explicit operator bool() const noexcept {return value!=0;}
    bool operator==(const ObjectIdentity&) const=default;
};
class ObjectHandleRegistry {
public:
    static constexpr std::size_t Capacity=2048;
    ObjectHandleRegistry();
    ObjectHandleRegistry(const ObjectHandleRegistry&)=delete;
    ObjectHandleRegistry& operator=(const ObjectHandleRegistry&)=delete;
    // EntryHandle: present owner handle is a no-op; exhaustion writes zero.
    // The generation counter increments per successful entry and skips zero.
    void Entry(ObjectHandle& owner,ObjectIdentity identity);
    // RemoveHandle clears a present owner even when its generation is stale.
    // A zero-generation value is left untouched. Released slots are LIFO.
    void Remove(ObjectHandle& owner);
    ObjectIdentity Get(ObjectHandle) const;
    // Host lifetime identity, separate from the finite retail generation. Never
    // reused within this registry; exhaustion refuses instead of wrapping.
    ObjectIdentity NewIdentity();
    std::size_t Size() const noexcept {return used_;}
    std::uint16_t LastGeneration() const noexcept {return generation_;}
private:
    struct EntryState {ObjectHandle handle;ObjectIdentity identity;};
    std::array<EntryState,Capacity> entries_{};
    std::array<std::uint16_t,Capacity> free_{};
    std::size_t used_{};
    std::uint16_t generation_{};
    std::uint64_t last_identity_{};
};
}
