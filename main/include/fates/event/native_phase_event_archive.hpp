#pragma once
#include "fates/event/native_phase_event_filter.hpp"
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace fates::event::native {
enum class PhaseEventKind : std::uint8_t { Turn = 16, Terrain = 17, After = 18, Reinforce = 19 };
enum class PhaseArchiveStatus : std::uint8_t {
    Ok, InvalidHeader, RelocatedOrAttached, InitializerRequired, InvalidTable,
    InvalidRecord, InvalidArguments, InvalidCode, LimitExceeded, InvalidEventKind
};
struct PhaseEventDeclaration {
    std::uint16_t function_index{}, local_words{};
    PhaseEventKind kind{PhaseEventKind::Turn};
    PhaseEventFilter filter{};
    std::uint32_t record_offset{}, code_offset{};
};

// Owns immutable ORIGINAL, unrelocated FE14 CMB bytes. Keep this shared owner
// with any selected function through a future event yield. This is a declaration
// reader, not archive attachment, initialization, bytecode validation or execution.
// An empty accepted archive differs from an unbound (null) archive. A caller must
// still establish the full attached archive set before asserting no event exists.
class PhaseEventArchive final {
public:
    static PhaseArchiveStatus Read(std::span<const std::uint8_t>,
        std::shared_ptr<const PhaseEventArchive>&);
    std::span<const std::uint8_t> bytes() const noexcept { return bytes_; }
    std::span<const PhaseEventDeclaration> declarations() const noexcept { return declarations_; }
    std::uint32_t function_count() const noexcept { return function_count_; }
    PhaseArchiveStatus Select(PhaseEventKind, std::uint16_t turn, std::uint8_t force,
        std::vector<std::uint16_t>& function_indices) const;
private:
    PhaseEventArchive() = default;
    std::vector<std::uint8_t> bytes_;
    std::vector<PhaseEventDeclaration> declarations_;
    std::uint32_t function_count_{};
};
}
