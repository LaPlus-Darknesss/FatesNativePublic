#pragma once
#include "fates/runtime/native_talk_initialize.hpp"
#include "fates/runtime/native_map_binder.hpp"

namespace fates::runtime::native {
struct TalkManagerStorageObservation {
    ProcessHandle manager;
    std::shared_ptr<const NativeTalkText> text;
    std::array<presentation::native::TalkWindowHandle,3> windows;
    presentation::native::TalkBackgroundHandle background;
    std::uint8_t windows_constructed{},windows_destroyed{};
    bool context_committed{},disposer_live{},constructor_complete{},destroying{};
};
// Original manager constructor body and deleting destructor over the existing
// owners. The base is supplied by the existing scheduler: either the earlier
// explicitly attached canonical base or the factory's constructing detached base.
// Event binding remains separate. FaceManager::CreateInstance is a real service boundary,
// not a provider that silently declares face loading complete.
class NativeTalkManagerStorage final:public ProcessCallbacks {
public:
    static TalkControlStatus Create(std::shared_ptr<NativeProcessScheduler>,std::shared_ptr<ProcessCallbackRegistry>,
        std::shared_ptr<NativeTalkControlContext>,std::shared_ptr<presentation::native::NativeTalkWindow>,
        std::shared_ptr<presentation::native::NativeTalkBackground>,std::shared_ptr<NativeTalkInitialize>,
        std::shared_ptr<NativeMapBinder>,std::shared_ptr<NativeGameSkip>,std::shared_ptr<presentation::native::NativeFadeSystem>,
        const NativeArchiveIdentifiers&,NativeMessageLookup&,NativeUnitNames&,NativeTalkPlayer&,NativeTalkTokens&,
        std::shared_ptr<NativeTalkManagerStorage>&);
    ~NativeTalkManagerStorage();
    NativeTalkManagerStorage(const NativeTalkManagerStorage&)=delete;
    NativeTalkManagerStorage& operator=(const NativeTalkManagerStorage&)=delete;
    // Identifiers, message lookup, unit names, player and tokens are borrowed from
    // the existing domain and must outlive this owner and its retained operations.
    // Constructor called only once for an otherwise-unowned canonical process.
    static ProcessCall ConstructCall(ProcessHandle);
    static ProcessCall IsExistCall(ProcessHandle);
    std::optional<TalkManagerStorageObservation> Observe(ProcessHandle)const;
    std::optional<ProcessHandle> Current()const;
    // Explicit startup/caller null store, not a guessed BSS or destructor run.
    TalkControlStatus PublishAbsent(ProcessAccess* =nullptr);
    // The original factory stores the completed attached result a second time.
    // Do not elide this store when constructor callbacks changed the global.
    TalkControlStatus PublishConstructed(ProcessHandle,ProcessAccess&);
    const NativeRuntime& runtime() const noexcept;
    bool UsesInitializer(const NativeTalkInitialize&) const noexcept;
    bool UsesContext(const NativeTalkControlContext&)const noexcept;
    bool UsesScheduler(const NativeProcessScheduler&)const noexcept;
    std::unique_ptr<ProcessContinuation> Begin(const ProcessCall&)override;
private:
    struct State;struct Continuation;explicit NativeTalkManagerStorage(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
