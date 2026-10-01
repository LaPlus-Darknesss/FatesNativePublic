#pragma once
#include "fates/map/native_map_camera.hpp"
#include "fates/runtime/native_game_skip.hpp"

namespace fates::map::native {
enum class CameraWaitStatus:std::uint8_t {
    Ready,NullScheduler,MismatchedDomain,DuplicateBinding,UnknownState,
    InvalidParent,CameraUnavailable,Retired,Busy
};
struct CameraWaitOutcome {
    bool yielded{};
    // Normal and absent-map branches return the final integer in r0. Instant's
    // incidental FieldConfig pointer is opaque; bytecode may discard it but the
    // VM must not invent an integer value for semantic use.
    std::optional<std::int32_t> value;
};
// Real ProcWaitCameraProlixity lifecycle over the common process scheduler.
// Draw presence is explicit carried map::Draw ownership, not whether a PC
// renderer is attached. No graphics sink supplies the completion predicate.
class NativeCameraWait final:public runtime::native::ProcessCallbacks {
public:
    static CameraWaitStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<NativeMapCamera>,
        std::shared_ptr<runtime::native::NativeGameSkip>,
        std::shared_ptr<presentation::native::NativeFadeSystem>,std::shared_ptr<NativeCameraWait>&);
    CameraWaitStatus RestoreCarriedDrawPresence(bool);
    CameraWaitStatus Bind(runtime::native::ProcessHandle parent,runtime::native::ProcessHandle& child);
    CameraWaitStatus Bind(runtime::native::ProcessAccess&,runtime::native::ProcessHandle parent,runtime::native::ProcessHandle& child);
    // CameraWait's caller must supply the real current ProcEvent. The typed
    // VM bridge owns that admission; arbitrary script callback registration is
    // not supplied by this scheduler module.
    CameraWaitStatus EventWait(runtime::native::ProcessAccess&,runtime::native::ProcessHandle current_event,CameraWaitOutcome&);
    bool UsesScheduler(const runtime::native::NativeProcessScheduler&) const noexcept;
    const std::shared_ptr<runtime::native::NativeRuntime>& runtime() const noexcept;
    std::vector<runtime::native::ProcessHandle> Processes() const;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    struct State;
    struct Continuation;
    explicit NativeCameraWait(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
