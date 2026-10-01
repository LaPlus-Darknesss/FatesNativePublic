#include "fates/cmvm/cmvm.hpp"
#include <cstring>

namespace fates::cmvm {
namespace {
std::uint16_t ReadU16Le(const std::byte* p) noexcept {
    return static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(p[0])) |
           static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(p[1]) << 8U);
}
std::uint32_t ReadU32Le(const std::byte* p) noexcept {
    return static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(p[0])) |
           (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(p[1])) << 8U) |
           (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(p[2])) << 16U) |
           (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(p[3])) << 24U);
}
bool OffsetFits(std::size_t offset, std::size_t need, std::size_t size) noexcept {
    return offset <= size && need <= size - offset;
}
}

bool certify_archive(const std::byte* archive, std::size_t size) {
    // Retail CmArchive::Certify requires the literal "cmb" magic and a version/size
    // floor. The exact version-floor constant remains runtime-owned until typed.
    return archive && size >= 0x24 &&
        std::to_integer<unsigned char>(archive[0])=='c' &&
        std::to_integer<unsigned char>(archive[1])=='m' &&
        std::to_integer<unsigned char>(archive[2])=='b';
}

bool open_named_script_library(const std::byte* archive, const std::size_t size, CmNamedScriptLibraryView& out) {
    // The shipped Scripts/Command.cmb uses ordinary type-0 CmFunction records whose
    // +0x10 field is an archive-relative NUL-terminated identifier pointer.  Keep this
    // parser intentionally scoped to that proven named-library form; chapter archives
    // may contain other function record types and are not reinterpreted here.
    if (!certify_archive(archive, size)) return false;
    const auto table_offset = static_cast<std::size_t>(ReadU32Le(archive + 0x1c));
    const auto string_offset = static_cast<std::size_t>(ReadU32Le(archive + 0x20));
    if (!OffsetFits(table_offset, 4, size) || string_offset >= size) return false;
    std::size_t count = 0;
    while (OffsetFits(table_offset + count * 4, 4, size)) {
        const auto record_offset = static_cast<std::size_t>(ReadU32Le(archive + table_offset + count * 4));
        if (record_offset == 0) break;
        if (!OffsetFits(record_offset, 0x14, size)) return false;
        ++count;
    }
    out.archive = {archive, size, archive + string_offset};
    out.function_table = archive + table_offset;
    out.function_count = count;
    return count != 0;
}

bool find_script_function_by_index(const CmNamedScriptLibraryView& lib, const std::size_t index, CmFunctionView& out) {
    if (!lib.archive.base || !lib.function_table || index >= lib.function_count) return false;
    const auto record_offset = static_cast<std::size_t>(ReadU32Le(lib.function_table + index * 4));
    if (!OffsetFits(record_offset, 0x14, lib.archive.size)) return false;
    const auto* record = lib.archive.base + record_offset;
    const auto code_offset = static_cast<std::size_t>(ReadU32Le(record + 4));
    const auto name_offset = static_cast<std::size_t>(ReadU32Le(record + 0x10));
    if (!OffsetFits(code_offset, 1, lib.archive.size) || !OffsetFits(name_offset, 1, lib.archive.size)) return false;
    if (std::to_integer<std::uint8_t>(record[8]) != 0) return false;
    const void* terminator = std::memchr(lib.archive.base + name_offset, 0, lib.archive.size - name_offset);
    if (!terminator) return false;
    out.address = lib.archive.base + code_offset;
    out.type = std::to_integer<std::uint8_t>(record[8]);
    out.argc = std::to_integer<std::uint8_t>(record[9]);
    out.local_words = ReadU16Le(record + 10);
    out.archive = &lib.archive;
    out.native = false;
    return true;
}

bool find_named_script_function(const CmNamedScriptLibraryView& lib, const std::string_view name, CmFunctionView& out) {
    for (std::size_t i = 0; i < lib.function_count; ++i) {
        const auto record_offset = static_cast<std::size_t>(ReadU32Le(lib.function_table + i * 4));
        if (!OffsetFits(record_offset, 0x14, lib.archive.size)) return false;
        const auto* record = lib.archive.base + record_offset;
        if (std::to_integer<std::uint8_t>(record[8]) != 0) continue;
        const auto name_offset = static_cast<std::size_t>(ReadU32Le(record + 0x10));
        if (!OffsetFits(name_offset, 1, lib.archive.size)) continue;
        const char* id = reinterpret_cast<const char*>(lib.archive.base + name_offset);
        const void* terminator = std::memchr(id, 0, lib.archive.size - name_offset);
        if (!terminator) continue;
        const auto length = static_cast<std::size_t>(static_cast<const char*>(terminator) - id);
        if (name == std::string_view(id, length)) return find_script_function_by_index(lib, i, out);
    }
    return false;
}

// Retail ownership notes:
// - CmAttachScript links certified archives into a global intrusive list.
// - first attach performs Relocate -> Initialize -> optional archive direct-call.
// - repeated attaches increment a 16-bit attach count.
// - CmFunction::Relocate and CmArchive::Relocate turn archive-relative offsets into pointers.
// - CmCFunction stores native callback + argc/type metadata and registers its Ident name.
// These remain adapters until the complete archive lifecycle is needed by a real causal transcript.
}
