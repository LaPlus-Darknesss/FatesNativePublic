#include "fates/io/archive_index.hpp"

#include "fates/detail/archive_index_runtime.hpp"
#include "fates/detail/arm32_address.hpp"
#include "fates/engine/ident_hash.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace {

constexpr std::size_t kPayloadOffset = 0x20;
constexpr std::size_t kRelocationEndOffset = 0x04;
constexpr std::size_t kRelocationCountOffset = 0x08;
constexpr std::size_t kIdentifierCountOffset = 0x0C;
constexpr std::size_t kAuxiliaryCountOffset = 0x10;
constexpr std::size_t kMagicOffset = 0x18;
constexpr std::size_t kConstructedFlagOffset = 0x1F;
constexpr std::array<char, 7> kArchiveMagic{{'H','S','D','A','r','c','\0'}};

std::uint32_t ReadU32(const std::byte* base, std::size_t offset) {
    std::uint32_t value{};
    std::memcpy(&value, base + offset, sizeof(value));
    return value;
}

void WriteU32(std::byte* base, std::size_t offset, std::uint32_t value) {
    std::memcpy(base + offset, &value, sizeof(value));
}

IdentHash* ResolveHash(IdentHash* hash) {
    return hash != nullptr ? hash : fates::decomp_detail::GetGlobalArchiveIdentHash();
}

} // namespace

void* ArchiveConstruct(void* archive, IdentHash* hash) {
    if (archive == nullptr) {
        return nullptr;
    }

    auto* const header = static_cast<std::byte*>(archive);
    auto* const payload = header + kPayloadOffset;

    const std::uint32_t relocationEnd = ReadU32(header, kRelocationEndOffset) & ~3u;
    const std::uint32_t relocationCount = ReadU32(header, kRelocationCountOffset);
    const std::uint32_t identifierCount = ReadU32(header, kIdentifierCountOffset);
    const std::uint32_t auxiliaryCount = ReadU32(header, kAuxiliaryCountOffset);

    auto* const relocationTable = payload + relocationEnd;
    auto* const identifierTable = relocationTable + relocationCount * sizeof(std::uint32_t);
    auto* const stringBase = identifierTable +
        identifierCount * 8u + auxiliaryCount * 8u;

    if (std::memcmp(header + kMagicOffset, kArchiveMagic.data(), kArchiveMagic.size()) != 0) {
        std::memcpy(header + kMagicOffset, kArchiveMagic.data(), kArchiveMagic.size());

        for (std::uint32_t index = 0; index < relocationCount; ++index) {
            const std::uint32_t fieldOffset =
                ReadU32(relocationTable, index * sizeof(std::uint32_t)) & ~3u;
            const std::uint32_t relative = ReadU32(payload, fieldOffset);
            const auto absolute = static_cast<std::uint32_t>(
                reinterpret_cast<std::uintptr_t>(payload + relative));
            WriteU32(payload, fieldOffset, absolute);
        }
    }

    if (static_cast<std::uint8_t>(header[kConstructedFlagOffset]) == 0) {
        IdentHash* const resolvedHash = ResolveHash(hash);
        header[kConstructedFlagOffset] = std::byte{1};

        if (resolvedHash != nullptr) {
            for (std::uint32_t index = 0; index < identifierCount; ++index) {
                const std::size_t entryOffset = index * 8u;
                const std::uint32_t valueOffset = ReadU32(identifierTable, entryOffset) & ~3u;
                const std::uint32_t nameOffset = ReadU32(identifierTable, entryOffset + 4u);

                const char* const name = reinterpret_cast<const char*>(stringBase + nameOffset);
                void* const value = payload + valueOffset;
                resolvedHash->Set(name, value);
            }
        }
    }

    return payload;
}

void ArchiveDestruct(void* archive, IdentHash* hash) {
    if (archive == nullptr) {
        return;
    }

    auto* const header = static_cast<std::byte*>(archive);
    if (static_cast<std::uint8_t>(header[kConstructedFlagOffset]) == 0) {
        return;
    }

    auto* const payload = header + kPayloadOffset;
    const std::uint32_t relocationEnd = ReadU32(header, kRelocationEndOffset) & ~3u;
    const std::uint32_t relocationCount = ReadU32(header, kRelocationCountOffset);
    const std::uint32_t identifierCount = ReadU32(header, kIdentifierCountOffset);
    const std::uint32_t auxiliaryCount = ReadU32(header, kAuxiliaryCountOffset);

    auto* const identifierTable = payload + relocationEnd +
        relocationCount * sizeof(std::uint32_t);
    auto* const stringBase = identifierTable +
        identifierCount * 8u + auxiliaryCount * 8u;

    IdentHash* const resolvedHash = ResolveHash(hash);
    if (resolvedHash != nullptr) {
        for (std::uint32_t index = identifierCount; index > 0; --index) {
            const std::size_t entryOffset = (index - 1u) * 8u;
            const std::uint32_t nameOffset = ReadU32(identifierTable, entryOffset + 4u);
            resolvedHash->Delete(reinterpret_cast<const char*>(stringBase + nameOffset));
        }
    }

    header[kConstructedFlagOffset] = std::byte{0};
}
