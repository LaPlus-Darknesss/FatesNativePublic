#pragma once
#include <cstdint>
#include <cstddef>
#include <span>
#include <vector>
#include <optional>
#include <string_view>
namespace fates::runtime::native {
enum class ArchiveLabelStatus { Found,Missing,Invalid };
struct ArchiveLabelResult { ArchiveLabelStatus status{ArchiveLabelStatus::Invalid};std::size_t offset{}; };
// Same bounded table walk as ReadArchiveLabeledOffset, with explicit absence.
ArchiveLabelResult FindArchiveLabel(std::span<const std::uint8_t>,std::string_view);
// Shared original FE14 wrapper/LZ11 decoder, extracted without algorithm changes
// from DefinitionStore. No file-system, renderer or platform ownership here.
// Read an unrelocated archive identifier value using ArchiveConstruct table rules.
// Missing, malformed, ambiguous or truncated labels have no resolved word.
std::optional<std::uint32_t> ReadArchiveLabeledWord(std::span<const std::uint8_t>,std::string_view);
// Absolute byte offset of a unique labeled record with at least one data word.
std::optional<std::size_t> ReadArchiveLabeledOffset(std::span<const std::uint8_t>,std::string_view);
bool DecompressFe14Archive(std::span<const std::uint8_t> input,
                          std::vector<std::uint8_t>& output);
}
