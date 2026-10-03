#pragma once
#include "fates/presentation/native_font_metrics.hpp"
#include "fates/presentation/native_talk_window.hpp"
namespace fates::presentation::native {
// One concrete color-change consumer of Font metrics. The window font/type and
// selected current font remain separate. This does not implement glyph drawing,
// FontObject sheet setup, font loading, text-width controls or ProcTalkManager.
class NativeTalkFontEffects final:public runtime::native::ProcessCallbacks {
public:
    static FontMetricStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<NativeTalkWindow>,
        std::shared_ptr<NativeFontMetrics>,std::shared_ptr<NativeTalkFontEffects>&);
    static runtime::native::ProcessCall ChangeColorCall(runtime::native::ProcessHandle,TalkWindowHandle,TalkColorBytes);
    bool UsesWindow(const NativeTalkWindow&) const noexcept;
    bool UsesScheduler(const runtime::native::NativeProcessScheduler&) const noexcept;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    struct Continuation;
    std::weak_ptr<runtime::native::NativeProcessScheduler> scheduler_;
    std::shared_ptr<NativeTalkWindow> windows_;
    std::shared_ptr<NativeFontMetrics> fonts_;
};
}
