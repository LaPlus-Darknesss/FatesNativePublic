#pragma once
#include <cstdint>

namespace fates::detail::cmvm {
inline constexpr std::uint32_t kRetailContextDebugStorageBytes = 0x144;
inline constexpr std::uint32_t kRetailDirectCallStackWords = 0x20;

inline constexpr std::uint8_t kOpcodeLocalLoadI8 = 0x01;
inline constexpr std::uint8_t kOpcodeLocalAddressI8 = 0x07;
inline constexpr std::uint8_t kOpcodePushI8 = 0x19;
inline constexpr std::uint8_t kOpcodePushI16Be = 0x1a;
inline constexpr std::uint8_t kOpcodePushWord32Be = 0x1b;
inline constexpr std::uint8_t kOpcodePushArchiveAddressI8 = 0x1c;
inline constexpr std::uint8_t kOpcodePushArchiveAddressI16Be = 0x1d;
inline constexpr std::uint8_t kOpcodePushWord32BeAlt = 0x1f;
inline constexpr std::uint8_t kOpcodeLoadIndirectKeepAddress = 0x20;
// Historical misnomer retained as a numeric alias. This dereferences the top
// address and pushes its word; it does not duplicate the address itself.
inline constexpr std::uint8_t kOpcodeDuplicate = kOpcodeLoadIndirectKeepAddress;
inline constexpr std::uint8_t kOpcodeDrop = 0x21;
inline constexpr std::uint8_t kOpcodeStoreIndirect = 0x23;
inline constexpr std::uint8_t kOpcodeSubtractInt = 0x28;
inline constexpr std::uint8_t kOpcodeDivideInt = 0x2c;
inline constexpr std::uint8_t kOpcodeRemainderInt = 0x2e;
inline constexpr std::uint8_t kOpcodeLogicalNot = 0x32;
inline constexpr std::uint8_t kOpcodeCompareEqualWord = 0x38;
inline constexpr std::uint8_t kOpcodeCompareLessEqualInt = 0x40;
inline constexpr std::uint8_t kOpcodeCompareGreaterInt = 0x42;
inline constexpr std::uint8_t kOpcodeCompareGreaterEqualInt = 0x44;
inline constexpr std::uint8_t kOpcodeCallFunctionIndex = 0x46;
inline constexpr std::uint8_t kOpcodeCallIdent = 0x47;
inline constexpr std::uint8_t kOpcodeReturnTop = 0x48;
inline constexpr std::uint8_t kOpcodeBranchI16Be = 0x49;
inline constexpr std::uint8_t kOpcodeBranchIfZeroI16Be = 0x4c;
inline constexpr std::uint8_t kOpcodeYield = 0x4e;
// Historical source name retained for compatibility. Retail sets context+21,
// leaves the next instruction intact and clears the flag on the next tick.
inline constexpr std::uint8_t kOpcodeStop = kOpcodeYield;
// Retail 0x4F advances over an inline four-byte payload. Earlier durable source called
// this a relative jump; Pass62 corrects that label directly from DoInstruction evidence.
inline constexpr std::uint8_t kOpcodeInlineWordSkip = 0x4f;
inline constexpr std::uint8_t kOpcodeDuplicateTopWord = 0x53;
inline constexpr std::uint8_t kOpcodeReturnFalse = 0x54;
inline constexpr std::uint8_t kOpcodeReturnTrue = 0x55;
}
