#pragma once
#include "fates/presentation/native_presentation.hpp"
#include "fates/runtime/native_runtime.hpp"

namespace fates::presentation::native {

// Host-facing ownership seam. Gameplay remains authoritative in NativeRuntime;
// presentation can observe semantic snapshots/events but cannot mutate gameplay
// through this interface.
class NativeHostSession {
public:
    NativeHostSession(runtime::native::NativeRuntime& runtime,
                      PresentationSink& presentation)
        : runtime_(runtime), bridge_(presentation) {}

    const PresentationSnapshot& PublishPresentation() {
        return bridge_.Publish(runtime_);
    }

    runtime::native::NativeRuntime& runtime() noexcept { return runtime_; }
    const runtime::native::NativeRuntime& runtime() const noexcept { return runtime_; }

private:
    runtime::native::NativeRuntime& runtime_;
    PresentationBridge bridge_;
};

} // namespace fates::presentation::native
