#pragma once
#include "fates/presentation/native_talk_window.hpp"

namespace fates::presentation::native {
enum class TalkSpeakerStatus:std::uint8_t {Ready,NullScheduler,MismatchedDomain,Busy,Retired,InvalidWindow,UnknownIdentifier,NameUnavailable};
enum class TalkSpeakerOrigin:std::uint8_t {ExplicitName,FaceMessage,RetainedName,OriginalFallback};
struct TalkSpeakerResult {
    TalkSpeakerStatus status{TalkSpeakerStatus::NameUnavailable};
    TalkSpeakerOrigin origin{TalkSpeakerOrigin::OriginalFallback};
    runtime::native::UnitNameStatus name_status{runtime::native::UnitNameStatus::Ready};
    TalkWindowSource source{};
};
// Original GetTalkerName5079D8 over the CURRENT window identifier and shared
// NativeUnitNames/FaceData/Mess owners. No FaceInstance, copy cache or invented
// empty speaker when a face is unowned. Returned name/message sources retain
// original live allocation/buffer identity; explicit null differs from empty.
class NativeTalkSpeaker final {
public:
    static TalkSpeakerStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<NativeTalkWindow>,std::shared_ptr<runtime::native::NativeUnitNames>,
        std::shared_ptr<runtime::native::NativeMessageLookup>,std::shared_ptr<NativeTalkSpeaker>&);
    TalkSpeakerResult Get(TalkWindowHandle,runtime::native::ProcessAccess* =nullptr);
    bool UsesWindows(const NativeTalkWindow& windows) const noexcept {return windows_.get()==&windows;}
private:
    std::weak_ptr<runtime::native::NativeProcessScheduler> scheduler_;
    std::shared_ptr<NativeTalkWindow> windows_;
    std::shared_ptr<runtime::native::NativeUnitNames> names_;
    std::shared_ptr<runtime::native::NativeMessageLookup> messages_;
};
}
