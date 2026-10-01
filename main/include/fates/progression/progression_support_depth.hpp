#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace fates::progression {

using DepthWord = std::intptr_t;

enum class DepthFamily : std::uint8_t {
    ClassChangeEnumeration,
    SupportPacket,
    ExperienceViewer,
    TalkFace,
    Presentation
};

struct DepthSpec {
    const char* retail_symbol;
    std::uint32_t retail_address;
    const char* retail_signature;
    DepthFamily family;
};
struct DepthRuntime {
    using InvokeFn = DepthWord (*)(void* user, const DepthSpec& spec, const DepthWord* args, std::size_t argc);
    void* user = nullptr;
    InvokeFn invoke = nullptr;
};
const DepthSpec* GetDepthSpecs();
std::size_t GetDepthSpecCount();
const DepthSpec* FindDepthSpec(std::uint32_t retail_address);
DepthWord InvokeDepth(DepthRuntime&, std::uint32_t retail_address, const DepthWord* args, std::size_t argc);

// Layout-independent / explicitly-layout-bounded contracts proven by Pass 49.
std::array<std::uint32_t,4> ClearIdentifierWords();
struct IdentifierSerializedV0 {
    std::uint8_t version = 0;
    std::array<std::uint32_t,2> first_u64_words{};
    std::array<std::uint32_t,2> second_u64_words{};
};
IdentifierSerializedV0 DescribeIdentifierSerializationV0(const std::array<std::uint32_t,4>& words);
std::vector<std::uint8_t> SerializeUnitEditV4(const std::array<std::uint8_t,0x30>& edit);

bool BuddyInheritanceCandidateEligible(bool candidate_is_player, bool candidate_is_download,
                                       bool same_sex, bool is_family, bool is_romance,
                                       int reliance_level);
int AbsoluteNameWindowTextureWidth(std::int16_t signed_width);
constexpr std::size_t kFrameManagerTitleItemSlots = 5;
constexpr std::size_t kTalkExpandedMaxCodeUnits = 0x1fff;
constexpr std::size_t kTalkFaceExpressionBytes = 0x20;

} // namespace fates::progression
