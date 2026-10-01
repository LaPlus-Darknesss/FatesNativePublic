#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>
namespace fates::cmvm {
struct ScriptTrickDeclaration {
    std::uint16_t function_index{};
    std::uint8_t event_type{};
    std::array<std::uint32_t,13> arguments{};
    std::string label;
};
enum class TrickArchiveStatus : std::uint8_t {
    Ok, InvalidHeader, RelocatedOrAttached, InitializerRequired,
    InvalidTable, InvalidRecord, InvalidArguments, InvalidString, LimitExceeded
};
// Reads ORIGINAL, unrelocated CMB bytes. It does not attach/execute a VM or
// reinterpret type-21/22 declarations as the existing named-function library.
// The accepted header version is the supplied FE14 format; initializer-bearing
// archives require their real execution owner before this initial snapshot route.
TrickArchiveStatus ReadScriptTrickDeclarations(std::span<const std::uint8_t>,
    std::vector<ScriptTrickDeclaration>&);
} // namespace fates::cmvm
