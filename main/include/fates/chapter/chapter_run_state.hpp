#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace fates::chapter {
using RunStateWord = std::intptr_t;
enum class RunStateFamily : std::uint8_t { ChapterLifecycle, GlobalUserState, GameProfile, GameConfig, PersistenceHelpers };
struct RunStateSpec { const char* retail_symbol; std::uint32_t retail_address; const char* retail_signature; RunStateFamily family; };
struct RunStateRuntime { using InvokeFn = RunStateWord (*)(void*, const RunStateSpec&, const RunStateWord*, std::size_t); void* user=nullptr; InvokeFn invoke=nullptr; };
const RunStateSpec* GetRunStateSpecs();
std::size_t GetRunStateSpecCount();
const RunStateSpec* FindRunStateSpec(std::uint32_t);
RunStateWord InvokeRunState(RunStateRuntime&, std::uint32_t, const RunStateWord*, std::size_t);

constexpr std::uint8_t kChapterFlowOpeningSkip = 4;
constexpr std::uint8_t kChapterFlowMapLoadSkip = 5;
constexpr std::uint32_t kChapterSaveTransientUserFlags = 0x07000EE2u;
constexpr std::uint8_t kGameUserGlobalDataVersion = 6;
constexpr std::uint8_t kGameUserGlobalSubstreamVersion = 3;
constexpr std::uint16_t kGameProfileSerializedBlockBytes = 0x01A8;
constexpr std::uint32_t kGameConfigResetFlags = 0x00011E5Eu;
constexpr std::uint32_t kConfigInfoCapabilityMode1Bit = 0x40000000u;
constexpr std::uint32_t kConfigInfoCapabilityMode2Bit = 0x20000000u;
constexpr std::uint8_t kUnitRecordVersion = 0;

bool IsMapLoadSkipped(std::uint8_t flow_state);
bool IsOpeningSkipped(std::uint8_t flow_state);
bool IsSortieSkipped(bool is_versus, bool context_has_sortie_flow, std::uint32_t chapter_flags, std::uint32_t user_flags, std::uint8_t flow_state);
std::uint32_t SanitizeUserFlagsBeforeChapterSave(std::uint32_t flags);
int ChapterSaveJumpState(std::uint32_t original_user_flags);
std::uint8_t SelectGlobalLastRoute(bool has_chapter, std::uint32_t chapter_flags, std::uint8_t current_route);
std::uint32_t SanitizeGlobalFlagsForSerialize(std::uint32_t flags);
std::uint8_t MapSettingEventType(bool resumed_or_backup, bool alternate_context);
int GetInfoCapabilityMode(std::uint32_t config_flags);
std::uint32_t SetInfoCapabilityMode(std::uint32_t config_flags, int mode);
bool IsDistancePairAllowed(std::uint8_t distance, std::uint8_t constraint);
std::uint8_t NormalizeDistance(std::uint8_t distance, std::uint8_t constraint);
std::uint8_t NextDistance(std::uint8_t distance, std::uint8_t constraint);
bool ProfileRelianceDiffers(std::uint16_t a, std::uint16_t b);
bool ProfileCastleAddressValid(std::uint32_t address);
bool ProfileRouteOwned(std::uint16_t profile_flags, std::uint16_t route_mask);

struct UnitRecordV0 { std::uint16_t a=0,b=0,c=0; std::uint8_t d=0,e=0; };
std::array<std::uint8_t,9> SerializeUnitRecordV0(const UnitRecordV0&);
UnitRecordV0 ClearUnitRecord();
struct EndChapterUnitBits { std::uint32_t flags8=0; std::uint32_t flagsC=0; std::array<std::uint16_t,5> status{}; };
EndChapterUnitBits NormalizeEndChapterUnitBits(EndChapterUnitBits in);
} // namespace fates::chapter
