#pragma once
#include "fates/event/native_phase_event_archive.hpp"
#include <cstddef>

namespace fates::cmvm::native { class ScriptAttachmentSession; }

namespace fates::event::native {
class PhaseEventCatalog;
class PhaseEventSelection final {
public:
    PhaseEventSelection() = default;
    explicit operator bool() const noexcept { return bool(catalog_); }
    std::shared_ptr<const PhaseEventArchive> archive() const noexcept;
    const PhaseEventDeclaration* declaration() const noexcept;
    std::size_t archive_index() const noexcept { return archive_index_; }
private:
    friend class PhaseEventCatalog;
    friend class fates::cmvm::native::ScriptAttachmentSession;
    std::shared_ptr<const PhaseEventCatalog> catalog_;
    std::size_t archive_index_{}, declaration_index_{};
};
enum class PhaseSearchStatus : std::uint8_t {
    Ok, Found, NoMatchInSnapshot, NullArchive, DuplicateArchive, LimitExceeded,
    InvalidKind, ForeignSelection
};

// Immutable ordered archive SNAPSHOT. It neither performs retail attachment nor
// certifies that a caller supplied every attached archive. NoMatchInSnapshot is
// deliberately not a certificate of completed gameplay/event work.
// A retained selection owns this snapshot and its original archive bytes.
class PhaseEventCatalog final : public std::enable_shared_from_this<PhaseEventCatalog> {
public:
    PhaseEventCatalog(const PhaseEventCatalog&) = delete;
    PhaseEventCatalog& operator=(const PhaseEventCatalog&) = delete;
    static PhaseSearchStatus Create(std::span<const std::shared_ptr<const PhaseEventArchive>>,
        std::shared_ptr<const PhaseEventCatalog>&);
    // CmGetNextFunctionTyped order: remaining table entries, then linked archive
    // order. Re-evaluate inspectors using THIS call's live turn/force, rather than
    // retaining a batch of matches from before preceding script side effects.
    PhaseSearchStatus FindNext(PhaseEventKind, std::uint16_t turn, std::uint8_t force,
        const PhaseEventSelection* previous, PhaseEventSelection& out) const;
    std::span<const std::shared_ptr<const PhaseEventArchive>> archives() const noexcept { return archives_; }
private:
    PhaseEventCatalog() = default;
    std::vector<std::shared_ptr<const PhaseEventArchive>> archives_;
};
}
