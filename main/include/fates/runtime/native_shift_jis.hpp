#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace fates::runtime::native {
enum class ShiftJisStatus:std::uint8_t {Ready,TablesUnavailable,InvalidTableImage,Unavailable,DestinationUnavailable,ReadLimit};
struct ShiftJisResult {
    ShiftJisStatus status{ShiftJisStatus::Ready};
    std::uint32_t code{}; // Original STD: 0 normal/limit/NUL, 2 null source, 3 invalid sequence.
    std::uint32_t consumed{},produced{};
    bool counts_written{}; // Original null-source branch leaves both optional count pointers untouched.
};
// Immutable shared conversion tables read from the user's original code image.
// No OS locale, wchar_t width, CP932/Unicode library substitution or duplicate
// text/token buffer. Input readers retain their actual source/lifetime semantics.
class NativeShiftJis final {
public:
    using WordReader=std::function<std::optional<char16_t>(std::size_t)>;
    using ByteReader=std::function<std::optional<std::uint8_t>(std::size_t)>;
    // Exact original ranges and table extents are checked before copying the two
    // tables. The author/Windows comparison separately binds the full code SHA.
    static ShiftJisStatus FromOriginalCode(std::span<const std::uint8_t>,std::shared_ptr<const NativeShiftJis>&);
    // Same known subset admitted by existing ASCII-only controls. Higher mapping
    // data is explicitly unavailable until FromOriginalCode is supplied.
    static std::shared_ptr<const NativeShiftJis> AsciiSubset();
    bool HasTables() const noexcept{return !to_sjis_.empty();}
    // Nullopt lengths correspond to null original int* arguments; negative values
    // mean unlimited INT_MAX. Null destination is count-only and ignores its input
    // capacity. A nonnull empty span is NOT treated as a null original pointer.
    ShiftJisResult ToSjis(const WordReader&,std::optional<std::span<std::uint8_t>>,
        std::optional<std::int32_t> source_limit={},std::optional<std::int32_t> destination_limit={},std::size_t read_budget=65536) const;
    ShiftJisResult ToUtf16(const ByteReader&,std::optional<std::span<char16_t>>,
        std::optional<std::int32_t> source_limit={},std::optional<std::int32_t> destination_limit={},std::size_t read_budget=65536) const;
    // sut wrappers reserve one element and append NUL to the converted prefix,
    // including STD error3. Native unavailable input/table never becomes success.
    // Zero/negative original capacities can write out of bounds and are refused.
    ShiftJisResult TerminatedSjis(const WordReader&,std::span<std::uint8_t>,std::size_t read_budget=65536) const;
    ShiftJisResult TerminatedUtf16(const ByteReader&,std::span<char16_t>,std::size_t read_budget=65536) const;
private:
    std::vector<std::uint8_t> to_sjis_;
    std::vector<std::uint16_t> to_unicode_;
};
}
