#pragma once
#include "fates/runtime/native_process.hpp"

namespace fates::runtime::native {
struct MapBinderIdentity final {const std::uint64_t serial;};
using MapBinderHandle=std::shared_ptr<const MapBinderIdentity>;
enum class MapBinderStatus:std::uint8_t {
    Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,
    UnknownCurrent,InvalidHandle,IdentityExhausted
};
struct MapBinderSnapshot {
    MapBinderHandle identity;
    std::array<std::int32_t,2> counts{};
    std::uint8_t state{};
};
// The shared map::Binder allocation/current pointer. Both ProcEvent and cursor
// descriptor callbacks use counter0. Counter1 and byte+8 are initialized but
// their later users are not yet admitted. This does not initialize the map.
class NativeMapBinder final:public ProcessCallbacks {
public:
    static MapBinderStatus Create(std::shared_ptr<NativeProcessScheduler>,
        std::shared_ptr<ProcessCallbackRegistry>,std::shared_ptr<NativeMapBinder>&);
    ~NativeMapBinder();
    NativeMapBinder(const NativeMapBinder&)=delete;
    NativeMapBinder& operator=(const NativeMapBinder&)=delete;
    // Successful original allocation/constructor/publication. Replacement does
    // not release an earlier allocation. Finalize releases only current storage.
    MapBinderStatus Initialize(MapBinderHandle&);
    MapBinderStatus Initialize(ProcessAccess&,MapBinderHandle&);
    MapBinderStatus Finalize();
    MapBinderStatus Finalize(ProcessAccess&);
    MapBinderStatus Bind(MapBinderHandle,bool& transitioned);
    MapBinderStatus Bind(ProcessAccess&,MapBinderHandle,bool& transitioned);
    MapBinderStatus Unbind(MapBinderHandle,bool& transitioned);
    MapBinderStatus Unbind(ProcessAccess&,MapBinderHandle,bool& transitioned);
    bool UsesScheduler(const NativeProcessScheduler&) const noexcept;
    std::optional<MapBinderHandle> Current() const;
    std::optional<MapBinderSnapshot> Observe(MapBinderHandle) const;
    // Cursor wrappers require a present binder, as the original dereferences
    // Get() unconditionally. Absence blocks; it is not a successful no-op.
    std::unique_ptr<ProcessContinuation> Begin(const ProcessCall&) override;
private:
    struct State;
    struct Continuation;
    explicit NativeMapBinder(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
