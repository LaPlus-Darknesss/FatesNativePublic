#include "fates/event/native_phase_event_archive.hpp"
#include <bit>
#include <set>

namespace fates::event::native {
namespace {
bool Fits(std::size_t at, std::size_t length, std::size_t size) noexcept {
    return at <= size && length <= size - at;
}
std::uint32_t U32(std::span<const std::uint8_t> bytes, std::size_t at) noexcept {
    return std::uint32_t(bytes[at]) | (std::uint32_t(bytes[at+1])<<8u) |
        (std::uint32_t(bytes[at+2])<<16u) | (std::uint32_t(bytes[at+3])<<24u);
}
bool IsPhaseKind(std::uint8_t kind) noexcept { return kind >= 16 && kind <= 19; }
}
PhaseArchiveStatus PhaseEventArchive::Read(std::span<const std::uint8_t> bytes,
    std::shared_ptr<const PhaseEventArchive>& out) {
    using S = PhaseArchiveStatus;
    if (bytes.size() < 0x28 || bytes.size() > 16u*1024u*1024u ||
        U32(bytes,0) != 0x00626d63u || U32(bytes,4) != 0x20110819u) return S::InvalidHeader;
    if (U32(bytes,0x10) || bytes[0x24] || bytes[0x25]) return S::RelocatedOrAttached;
    if (bytes[0x26]) return S::InitializerRequired;
    const auto table = std::size_t(U32(bytes,0x1c));
    if (table < 0x28 || (table & 3u) || !Fits(table,4,bytes.size()) ||
        U32(bytes,0x20) > bytes.size()) return S::InvalidTable;
    auto next = std::shared_ptr<PhaseEventArchive>(new PhaseEventArchive);
    std::set<std::uint32_t> seen;
    bool terminated = false;
    for (std::uint32_t index = 0; index < 32768u; ++index) {
        const auto entry = table + std::size_t(index)*4u;
        if (!Fits(entry,4,bytes.size())) return S::InvalidTable;
        const auto record = U32(bytes,entry);
        if (!record) { terminated = true; break; }
        if (record < 0x28 || (record & 3u) || !Fits(record,0x18,bytes.size()) ||
            !seen.insert(record).second) return S::InvalidRecord;
        ++next->function_count_;
        const auto kind = bytes[record+8];
        if (!IsPhaseKind(kind)) continue;
        const auto arguments = U32(bytes,record+0x14);
        if (bytes[record+9] != 3 || arguments < 0x28 || (arguments & 3u) ||
            !Fits(arguments,12,bytes.size())) return S::InvalidArguments;
        const auto code = U32(bytes,record+4);
        if (code < 0x28 || !Fits(code,1,bytes.size())) return S::InvalidCode;
        PhaseEventDeclaration declaration{};
        declaration.function_index = static_cast<std::uint16_t>(index);
        declaration.local_words = static_cast<std::uint16_t>(unsigned(bytes[record+10]) |
            (unsigned(bytes[record+11])<<8u));
        declaration.kind = static_cast<PhaseEventKind>(kind);
        declaration.filter = {std::bit_cast<std::int32_t>(U32(bytes,arguments)),
            std::bit_cast<std::int32_t>(U32(bytes,arguments+4)),
            std::bit_cast<std::int32_t>(U32(bytes,arguments+8))};
        declaration.record_offset = record; declaration.code_offset = code;
        next->declarations_.push_back(declaration);
        if (next->declarations_.size() > 4096u) return S::LimitExceeded;
    }
    if (!terminated) return S::LimitExceeded;
    next->bytes_.assign(bytes.begin(),bytes.end());
    out = std::move(next);
    return S::Ok;
}
PhaseArchiveStatus PhaseEventArchive::Select(PhaseEventKind kind, std::uint16_t turn,
    std::uint8_t force, std::vector<std::uint16_t>& out) const {
    if (!IsPhaseKind(static_cast<std::uint8_t>(kind))) return PhaseArchiveStatus::InvalidEventKind;
    std::vector<std::uint16_t> selected;
    for (const auto& declaration : declarations_)
        if (declaration.kind == kind && PhaseEventMatchesExact(declaration.filter,turn,force))
            selected.push_back(declaration.function_index);
    out = std::move(selected);
    return PhaseArchiveStatus::Ok;
}
}
