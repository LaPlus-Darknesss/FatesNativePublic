#pragma once
#include "fates/io/native_resource_reference.hpp"
#include "fates/runtime/native_process.hpp"
namespace fates::presentation::native {
struct FrameManagerIdentity final {const std::uint32_t serial;};
using FrameManagerHandle=std::shared_ptr<const FrameManagerIdentity>;
struct FrameLayoutState {
    std::optional<io::native::ResourceDeletionReference> title;
    std::array<std::optional<io::native::ResourceDeletionReference>,5> items;
    std::uint32_t count{},height{},word_28{},word_2c{},word_30{};
    std::uint8_t byte_24{},byte_25{},byte_26{},byte_27{};
    bool destructor_entered{};
};
// Actual Layout constructor fields. This does not construct/publish a manager,
// load textures, create its layout process, or infer an absent title/item cache.
FrameLayoutState ConstructFrameLayoutState(std::uint32_t side,std::uint32_t dirty) noexcept;
enum class FrameManagerLife:std::uint8_t {Live,Destroying,Destroyed};
struct CarriedFrameManager {
    std::uint32_t storage_word{};
    std::array<FrameLayoutState,2> layouts;
};
struct FrameManagerObservation {
    FrameManagerHandle identity;
    CarriedFrameManager fields;
    FrameManagerLife life{};
};
struct FrameIconTextObservation {
    std::uint32_t object_word{},text_storage_word{};
    bool destructor_entered{},destroyed{};
};
enum class FrameManagerStatus:std::uint8_t {
    Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,
    InvalidState,InvalidHandle,IdentityExhausted
};
class NativeFrameManager final:public runtime::native::ProcessCallbacks {
public:
    static FrameManagerStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<NativeFrameManager>&);
    ~NativeFrameManager();
    NativeFrameManager(const NativeFrameManager&)=delete;
    NativeFrameManager& operator=(const NativeFrameManager&)=delete;
    FrameManagerStatus Restore(const CarriedFrameManager&,std::optional<runtime::native::ProcessHandle> current_process,
        FrameManagerHandle&,runtime::native::ProcessAccess* =nullptr);
    FrameManagerStatus PublishAbsent(runtime::native::ProcessAccess* =nullptr);
    FrameManagerStatus WriteLayout(FrameManagerHandle,std::uint32_t,const FrameLayoutState&,runtime::native::ProcessAccess* =nullptr);
    FrameManagerStatus RestoreIconText(std::uint32_t object_word,std::uint32_t text_storage_word,runtime::native::ProcessAccess* =nullptr);
    // Unknown publication, known null, and a live/retiring allocation differ.
    std::optional<FrameManagerHandle> Current() const;
    std::optional<runtime::native::ProcessHandle> CurrentProcess() const;
    std::optional<FrameManagerObservation> Observe(FrameManagerHandle) const;
    std::optional<FrameIconTextObservation> ObserveIconText(std::uint32_t object_word) const;
    static runtime::native::ProcessCall DeleteCall(runtime::native::ProcessHandle);
    static runtime::native::ProcessCall FinalizeCall(runtime::native::ProcessHandle);
    std::optional<runtime::native::ProcessCall> DeleteLayoutCall(runtime::native::ProcessHandle,FrameManagerHandle,std::uint32_t,bool title=true) const;
    std::optional<runtime::native::ProcessCall> DestroyCall(runtime::native::ProcessHandle,FrameManagerHandle) const;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    struct State;struct Continuation;
    explicit NativeFrameManager(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
