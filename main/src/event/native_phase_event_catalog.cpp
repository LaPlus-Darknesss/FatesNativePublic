#include "fates/event/native_phase_event_catalog.hpp"
#include <set>

namespace fates::event::native {
std::shared_ptr<const PhaseEventArchive> PhaseEventSelection::archive() const noexcept {
    return catalog_ ? catalog_->archives()[archive_index_] : nullptr;
}
const PhaseEventDeclaration* PhaseEventSelection::declaration() const noexcept {
    const auto owner = archive();
    return owner ? &owner->declarations()[declaration_index_] : nullptr;
}
PhaseSearchStatus PhaseEventCatalog::Create(
    std::span<const std::shared_ptr<const PhaseEventArchive>> archives,
    std::shared_ptr<const PhaseEventCatalog>& out) {
    if (archives.size() > 1024) return PhaseSearchStatus::LimitExceeded;
    std::set<const PhaseEventArchive*> seen;
    for (const auto& archive : archives) {
        if (!archive) return PhaseSearchStatus::NullArchive;
        if (!seen.insert(archive.get()).second) return PhaseSearchStatus::DuplicateArchive;
    }
    auto next = std::shared_ptr<PhaseEventCatalog>(new PhaseEventCatalog);
    next->archives_.assign(archives.begin(),archives.end());
    out = std::move(next);
    return PhaseSearchStatus::Ok;
}
PhaseSearchStatus PhaseEventCatalog::FindNext(PhaseEventKind kind, std::uint16_t turn,
    std::uint8_t force, const PhaseEventSelection* previous, PhaseEventSelection& out) const {
    if (static_cast<unsigned>(kind)<16 || static_cast<unsigned>(kind)>19)
        return PhaseSearchStatus::InvalidKind;
    std::size_t archive_index = 0, declaration_index = 0;
    if (previous) {
        if (previous->catalog_.get()!=this || !previous->declaration() || previous->declaration()->kind!=kind)
            return PhaseSearchStatus::ForeignSelection;
        archive_index = previous->archive_index_;
        declaration_index = previous->declaration_index_+1;
    }
    for (; archive_index<archives_.size(); ++archive_index, declaration_index=0) {
        const auto declarations = archives_[archive_index]->declarations();
        for (; declaration_index<declarations.size(); ++declaration_index) {
            const auto& declaration = declarations[declaration_index];
            if (declaration.kind!=kind || !PhaseEventMatchesExact(declaration.filter,turn,force)) continue;
            PhaseEventSelection next;
            next.catalog_ = shared_from_this();
            next.archive_index_ = archive_index;
            next.declaration_index_ = declaration_index;
            out = std::move(next);
            return PhaseSearchStatus::Found;
        }
    }
    out = {};
    return PhaseSearchStatus::NoMatchInSnapshot;
}
}
