#pragma once
#include "fates/io/native_resource_reference.hpp"
#include "fates/runtime/native_process.hpp"
namespace fates::map::native {
struct UnitIconManagerIdentity final {const std::uint32_t serial;};
struct UnitIconCacheIdentity final {const std::uint32_t serial;};
using UnitIconManagerHandle=std::shared_ptr<const UnitIconManagerIdentity>;
using UnitIconCacheHandle=std::shared_ptr<const UnitIconCacheIdentity>;
struct CarriedUnitIconCacheEntry {
    io::native::ResourceDeletionReference resource;
    std::string name;
    std::uint16_t references{};
};
struct CarriedUnitIconCache {
    std::vector<CarriedUnitIconCacheEntry> order;
    std::vector<std::size_t> index_order;
};
enum class UnitIconCacheLife:std::uint8_t {Linked,Destroying,Destroyed};
struct UnitIconCacheObservation {
    UnitIconCacheHandle identity;
    UnitIconManagerHandle allocation;
    CarriedUnitIconCacheEntry entry;
    UnitIconCacheLife life{};
};
struct UnitIconManagerObservation {
    bool present{};
    UnitIconManagerHandle identity;
    std::vector<UnitIconCacheHandle> order;
    std::uint32_t index_accounting_count{};
    std::size_t index_size{};
};
enum class UnitIconManagerStatus:std::uint8_t {
    Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,
    InvalidState,InvalidHandle,IdentityExhausted
};
// Owns the resource cache, not the separate UnitIcon animation fields. Current
// publication and complete cache membership must come from real carried state.
// No constructor, empty-cache inference or substitute destructor is installed.
class NativeUnitIconManager final:public runtime::native::ProcessCallbacks {
public:
    static UnitIconManagerStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<NativeUnitIconManager>&);
    ~NativeUnitIconManager();
    NativeUnitIconManager(const NativeUnitIconManager&)=delete;
    NativeUnitIconManager& operator=(const NativeUnitIconManager&)=delete;
    UnitIconManagerStatus Restore(const CarriedUnitIconCache&,UnitIconManagerHandle&,
        std::vector<UnitIconCacheHandle>&,runtime::native::ProcessAccess* =nullptr);
    UnitIconManagerStatus PublishAbsent(runtime::native::ProcessAccess* =nullptr);
    // Extend carried live membership. Resource construction remains its owner.
    UnitIconManagerStatus AppendCarried(UnitIconManagerHandle,const CarriedUnitIconCacheEntry&,
        UnitIconCacheHandle&,runtime::native::ProcessAccess* =nullptr);
    UnitIconManagerStatus SetReferences(UnitIconCacheHandle,std::uint16_t,runtime::native::ProcessAccess* =nullptr);
    std::optional<UnitIconManagerObservation> Observe() const;
    std::optional<UnitIconManagerObservation> Observe(UnitIconManagerHandle) const;
    std::optional<UnitIconCacheObservation> Observe(UnitIconCacheHandle) const;
    UnitIconCacheHandle Find(UnitIconManagerHandle,std::string_view) const;
    static runtime::native::ProcessCall SweepCall(runtime::native::ProcessHandle);
    std::optional<runtime::native::ProcessCall> SweepCall(runtime::native::ProcessHandle,UnitIconManagerHandle) const;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    struct State;struct Continuation;
    explicit NativeUnitIconManager(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
