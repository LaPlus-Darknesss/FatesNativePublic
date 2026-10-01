#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace fates::progression {

using DependencyWord = std::intptr_t;
using JobHandle = std::intptr_t;

enum class DependencyFamily : std::uint8_t {
    ProgressionData,
    SupportState,
    SupportPacket,
    ProgressionPresentation,
    TalkFacePresentation
};

struct DependencySpec {
    const char* retail_symbol;
    std::uint32_t retail_address;
    const char* retail_signature;
    DependencyFamily family;
};

struct DependencyRuntime {
    using InvokeFn = DependencyWord (*)(void* user, const DependencySpec& spec, const DependencyWord* args, std::size_t argc);
    void* user = nullptr;
    InvokeFn invoke = nullptr;
};

const DependencySpec* GetDependencySpecs();
std::size_t GetDependencySpecCount();
const DependencySpec* FindDependencySpec(std::uint32_t retail_address);
DependencyWord InvokeDependency(DependencyRuntime&, std::uint32_t retail_address, const DependencyWord* args, std::size_t argc);

// Exact, layout-independent retail policies recovered in Pass 48.
int DecodeClassChangeType(std::int8_t raw_type_code);

struct LearnSkillCallbacks {
    void* user = nullptr;
    bool (*job_is_high)(void* user, JobHandle job) = nullptr;
    int (*job_get_equip_skill)(void* user, JobHandle job, int level) = nullptr;
    bool (*unit_has_equip_skill)(void* user, int skill_id) = nullptr;
};
int SelectLearnEquipSkill(const JobHandle* candidates, std::size_t candidate_count,
                          int resolved_unit_level, int current_job_limit, bool current_job_is_high,
                          LearnSkillCallbacks callbacks);

bool VariableSizeFlagGet(const std::uint8_t* bytes, std::size_t bit_count, std::size_t index);
bool PackedFlagGet(const std::uint8_t* bytes, std::size_t index);
int RelianceFlagIndexPlayerAqua(bool alternate_block, int person_group, int reliance_level);
bool IdentifierEqual(const std::array<std::uint32_t,4>& lhs, const std::array<std::uint32_t,4>& rhs);

struct RelianceTalkInputs {
    bool first_invalid = false;
    bool second_invalid = false;
    bool first_route_enabled = false;
    bool second_route_enabled = false;
    bool first_unit_blocked = false;   // retail Unit flags & 0x18
    bool second_unit_blocked = false;  // retail Unit flags & 0x18
    int current_level = 0;
    int max_level = 0;
    bool next_rank_talk_exists = false;
    bool allow_post_max_special = false;
    bool first_is_player = false;
    bool second_is_player = false;
    bool first_is_download = false;
    bool second_is_download = false;
    bool same_sex = false;
    bool is_family = false;
    bool first_secondary_person_id_is_zero = false; // retail Unit +0x126 halfword
};
bool IsRelianceTalkEligible(const RelianceTalkInputs&);

struct SkipExpResult {
    std::uint8_t level = 0;
    std::uint8_t exp = 0;
    bool level_advanced = false;
};
SkipExpResult ApplySkippedExperienceLane(std::uint8_t level, std::uint8_t exp, int gain);

constexpr std::size_t kPacketUnitBytes = 0xB0;
constexpr std::size_t kUnitEditBytes = 0x30;
constexpr std::size_t kIdentifierWords = 4;
constexpr std::size_t kEnhanceBytes = 23;
constexpr std::size_t kClothBytes = 8;
constexpr std::size_t kRecordBytes = 8;
constexpr std::uint8_t kPacketUnitSerializeVersion = 2;

std::array<std::uint16_t,4> EncodeGameFontIcon(std::uint16_t icon_id);
std::array<std::uint16_t,4> EncodeGameFontSkill(std::uint16_t skill_id);
std::int16_t WaitTicksFromMilliseconds(float milliseconds);
int TransitionTicksFromMilliseconds(float milliseconds);

} // namespace fates::progression
