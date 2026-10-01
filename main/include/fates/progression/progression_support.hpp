#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace fates::progression {

using ProgressionWord = std::intptr_t;

enum class ProgressionFamily : std::uint8_t { GrowthRuntime, LevelUpPresentation, SupportState, RelianceCore, DualSupport };

struct ProgressionSpec {
    const char* retail_symbol;
    std::uint32_t retail_address;
    const char* retail_signature;
    ProgressionFamily family;
};

struct ProgressionRuntime {
    using InvokeFn = ProgressionWord (*)(void* user, const ProgressionSpec& spec, const ProgressionWord* args, std::size_t argc);
    void* user = nullptr;
    InvokeFn invoke = nullptr;
};

const ProgressionSpec* GetProgressionSpecs();
std::size_t GetProgressionSpecCount();
const ProgressionSpec* FindProgressionSpec(std::uint32_t retail_address);
ProgressionWord InvokeProgression(ProgressionRuntime&, std::uint32_t retail_address, const ProgressionWord* args, std::size_t argc);

// Layout-independent retail contracts proven by Pass 47.
constexpr std::size_t kLevelStatCount = 8;
struct GrowthCallbacks {
    void* user = nullptr;
    int (*next_percent_0_99)(void* user) = nullptr;
    bool (*can_raise)(void* user, std::size_t stat_index) = nullptr;
    void (*raise)(void* user, std::size_t stat_index) = nullptr;
};
struct GrowthResolution {
    std::array<std::uint8_t, kLevelStatCount> applied_points{};
    unsigned attempts = 0;
    bool qualifying_growth_event = false;
};
GrowthResolution ResolveRetailLevelGrowth(const std::array<int, kLevelStatCount>& growth_percent, GrowthCallbacks callbacks);

bool SupportKeyMatches(std::uint16_t stored_person_id, std::uint16_t query_person_id,
                       bool query_person_is_download, bool identifier_equal);
std::array<std::uint8_t,2> SupportPoolSerializedHeader(std::size_t node_count);

struct DualSupportState {
    std::int32_t word0 = 0;
    std::int32_t word1 = 0;
    std::int32_t primary_index = -1;
    std::int32_t secondary_index = -1;
    std::array<std::int16_t,4> addends{};
};
DualSupportState ClearedDualSupportState();

bool HasNewLevel(std::uint8_t calculated_level, std::uint8_t committed_level);
bool HasNewWeaponRank(int before_rank, int after_rank);
bool ShouldBranchClassChangeSkill(bool has_class_change, int class_change_type,
                                  unsigned branch_index, unsigned branch_count);

// One source-facing function per meaningful retail responsibility. Exact ABI signatures remain in evidence/registry.
ProgressionWord GameProfile__UpdateReliance(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord RelianceObj__RelianceObj(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord SupportNode__SupportNode(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord SupportPool__Deserialize(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord SupportPool__Entry(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord SupportPool__GetEmpty(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord GrowSequence__BranchSkill(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord GrowSequence__ClassChange(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord GrowSequence__DualLevelUp(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord GrowSequence__ForgetSkill(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord GrowSequence__LevelUpShow(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord GrowSequence__DualGainSkill(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord GrowSequence__GainWeaponExp(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord GrowSequence__WeaponLevelUp(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord GrowSequence__ClassChangeShow(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord GrowSequence__DualForgetSkill(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord GrowSequence__DualLevelUpShow(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord GrowSequence__DualGainWeaponExp(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord GrowSequence__ClassChangeReflect(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord GrowSequence__DualLevelUpReflect(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord GrowSequence__ClassChangeCalculate(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord GrowSequence__DualLevelUpCalculate(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord GrowSequence__Create(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord GrowSequence__GainExp(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord GrowSequence__LevelUp(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord GrowSequence__Calculate(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord GrowSequence__GainSkill(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord GrowSequence__GrowSequence(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord Live2DDefine__GetSupportPoint(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord GrowSequence__ResumeGameInfo(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord LevelUpSequence__anonymous_namespace__ProcGrowUp__Persistent(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord LevelUpSequence__anonymous_namespace__ProcGrowUp__TickParams(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord LevelUpSequence__anonymous_namespace__ProcGrowUp__WindowOpen(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord LevelUpSequence__anonymous_namespace__ProcGrowUp__WindowClose(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord LevelUpSequence__anonymous_namespace__ProcGrowUp__TickWaitLoad(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord LevelUpSequence__anonymous_namespace__ProcGrowUp__ResultMessage(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord LevelUpSequence__anonymous_namespace__ProcGrowUp__CreateLightEffect(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord LevelUpSequence__anonymous_namespace__ProcGrowUp__Telop(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord LevelUpSequence__anonymous_namespace__ProcGrowUp__LevelTo(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord LevelUpSequence__anonymous_namespace__ProcGrowUp__LevelTo2(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord LevelUpSequence__anonymous_namespace__ProcGrowUp__TickFace(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord LevelUpSequence__anonymous_namespace__ProcGrowUp__ProcGrowUp(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord LevelUpSequence__anonymous_namespace__ProcGrowMessage__Persistent(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord LevelUpSequence__anonymous_namespace__ProcGrowMessage__TickFadeOut(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord LevelUpSequence__anonymous_namespace__ProcGrowMessage__TickScrollIn(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord LevelUpSequence__anonymous_namespace__ProcGrowMessage__TickScrollUp(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord LevelUpSequence__anonymous_namespace__ProcGrowMessage__TickScrollOut(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord LevelUpSequence__Create(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord RelianceTalkSequence__Create(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord RelianceTalkSequence__CanTalk(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord PersonEnumerator_Reliance__IsExclusion(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord PersonEnumerator_Reliance__RelianceObjVectorList__RelianceObjVectorList(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord PersonEnumerator_Reliance__PersonEnumerator_Reliance(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord map__DualSupportCalculator__Clear(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord map__DualSupportCalculator__Calculate(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord GrowSequence__LevelUpReflect(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord GrowSequence__LevelUpCalculate(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord SupportNode__GetName(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord SupportPool__GetLockUnitNum(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord SupportPool__Dump(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord SupportPool__Search(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord SupportPool__Search_2(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord SupportPool__IsLocked(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);
ProgressionWord SupportPool__Serialize(ProgressionRuntime&, const ProgressionWord* args, std::size_t argc);

} // namespace fates::progression
