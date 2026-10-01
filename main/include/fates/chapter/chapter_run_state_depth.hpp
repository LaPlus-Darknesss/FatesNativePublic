#pragma once
#include <cstddef>
#include <cstdint>
namespace fates::chapter {
using DepthWord=std::intptr_t;
enum class DepthFamily:std::uint8_t{BackupLifecycle,DurableState,Mode,Deployment,Gameplay};
struct DepthSpec{const char* retail_symbol;std::uint32_t retail_address;DepthFamily family;};
struct DepthRuntime{using InvokeFn=DepthWord(*)(void*,const DepthSpec&,const DepthWord*,std::size_t);void* user=nullptr;InvokeFn invoke=nullptr;};
const DepthSpec* GetDepthSpecs();std::size_t GetDepthSpecCount();const DepthSpec* FindDepthSpec(std::uint32_t);DepthWord InvokeDepth(DepthRuntime&,std::uint32_t,const DepthWord*,std::size_t);
constexpr std::uint8_t kVariableStateStreamVersion=0;constexpr std::uint8_t kGameRealElapseVersion=1;constexpr std::size_t kGameRealElapseSerializedBytes=26;constexpr std::size_t kGameBackupHeaderReadBytes=0xC0;constexpr std::size_t kUnitEditBytes=0x30;
std::size_t PackedFlagBytes(std::uint32_t bit_count);std::size_t Color8TableBytes(std::uint32_t count);bool TestPackedFlag(const std::uint8_t*,std::size_t,std::uint32_t);void SetPackedFlag(std::uint8_t*,std::size_t,std::uint32_t);int VersusTimeLimitSeconds(int);bool LinkFailureFromCode(int);bool MatchClientFromSessionMaster(bool);bool MatchMasterFromSlots(std::uint8_t,std::uint8_t);std::size_t UnitEditInitialDeserializeBytes(std::uint8_t version);
}
