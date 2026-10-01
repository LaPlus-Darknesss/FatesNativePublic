#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
namespace fates::campaign {
using PersistenceWord=std::intptr_t;
enum class PersistenceFamily:std::uint8_t{Backup,DynamicState,WorldState};
struct PersistenceSpec{const char* retail_symbol;std::uint32_t retail_address;PersistenceFamily family;};
struct PersistenceRuntime{using InvokeFn=PersistenceWord(*)(void*,const PersistenceSpec&,const PersistenceWord*,std::size_t);void* user=nullptr;InvokeFn invoke=nullptr;};
const PersistenceSpec* GetPersistenceSpecs();std::size_t GetPersistenceSpecCount();const PersistenceSpec* FindPersistenceSpec(std::uint32_t);PersistenceWord InvokePersistence(PersistenceRuntime&,std::uint32_t,const PersistenceWord*,std::size_t);
constexpr std::size_t kBackupHeaderBytes=0xC0;constexpr std::uint8_t kBackupHeaderVersion=1;constexpr std::uint8_t kDynamicStateVersion=0;
int BackupFileCount(int type_code);const char16_t* BackupFixedFileName(int type_code);std::size_t PackedFlagByteCount(std::uint32_t bits);bool PackedFlagGet(const std::uint8_t*,std::size_t,std::uint32_t);void PackedFlagSet(std::uint8_t*,std::size_t,std::uint32_t);
struct Color8{std::uint8_t r{},g{},b{},a{};};Color8 ColorTableGet(const Color8*,std::size_t,std::size_t);std::size_t ColorTableByteCount(std::uint32_t count);
struct SpotMobPortable{std::uint8_t kind{};std::uint8_t active{};std::array<std::uint32_t,5> words{};std::int32_t tail{-1};};void ResetSpotMob(SpotMobPortable&);bool IsSpotMobEmpty(const SpotMobPortable&);unsigned CountSpotMobs(const SpotMobPortable&,const SpotMobPortable&);
}
