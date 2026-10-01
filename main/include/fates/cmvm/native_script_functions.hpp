#pragma once
#include "fates/event/native_phase_event_archive.hpp"
#include "fates/runtime/native_identifier.hpp"
#include <memory>
#include <optional>
#include <string>

namespace fates::cmvm::native {
struct ScriptFunctionInfo {
    std::uint16_t index{}, local_words{};
    std::uint8_t type{}, argument_count{};
    std::uint32_t record_offset{}, code_offset{};
    // Null record+10 is an unnamed function. An empty but present identifier
    // remains distinguishable. Bytes retain the original encoding.
    std::optional<std::string> name;
};
class ScriptFunctionTable;
class ScriptFunctionRef final {
public:
    ScriptFunctionRef()=default;
    explicit operator bool() const noexcept {return bool(table_);}
    const ScriptFunctionInfo* info() const noexcept;
    std::shared_ptr<const ScriptFunctionTable> table() const noexcept {return table_;}
private:
    friend class ScriptFunctionTable;
    std::shared_ptr<const ScriptFunctionTable> table_;
    std::uint16_t index_{};
};
enum class ScriptFunctionStatus : std::uint8_t {
    Ok, NullArchive, InvalidCode, InvalidName, LimitExceeded, InvalidIndex
};
// Retains all original table entries, including unnamed ordinary functions.
// This extends the existing bounded named-library reader without treating every
// indexed function as named or every nonzero type as an ordinary script helper.
class ScriptFunctionTable final : public std::enable_shared_from_this<ScriptFunctionTable> {
public:
    ScriptFunctionTable(const ScriptFunctionTable&)=delete;
    ScriptFunctionTable& operator=(const ScriptFunctionTable&)=delete;
    static ScriptFunctionStatus Read(std::shared_ptr<const event::native::PhaseEventArchive>,
        std::shared_ptr<const ScriptFunctionTable>&);
    ScriptFunctionStatus Get(std::size_t index, ScriptFunctionRef&) const;
    std::span<const ScriptFunctionInfo> functions() const noexcept {return functions_;}
    std::shared_ptr<const event::native::PhaseEventArchive> archive() const noexcept {return archive_;}
private:
    ScriptFunctionTable()=default;
    std::shared_ptr<const event::native::PhaseEventArchive> archive_;
    std::vector<ScriptFunctionInfo> functions_;
};

enum class ScriptSymbolKind : std::uint8_t { Script, Native, UnimplementedNative=Native };
struct ScriptSymbolBinding {
    std::string identifier;
    ScriptSymbolKind kind{ScriptSymbolKind::Script};
    ScriptFunctionRef function;
};
enum class ScriptLookupStatus : std::uint8_t {
    Ok, InvalidBucketCount, InvalidBinding, LimitExceeded, FoundScript,
    NativeOwnerRequired, MissingFromSnapshot, InvalidIdentifier
};
struct ScriptLookupResult {
    ScriptLookupStatus status{ScriptLookupStatus::MissingFromSnapshot};
    std::string registered_identifier;
    ScriptFunctionRef function;
};
// Immutable explicit registry snapshot. Bucket count and registration order are
// supplied facts; this is not full retail initialization/attachment. Native names
// can reserve their real positions without pretending their callbacks exist.
// Reuse exact IdentHash append/first-hash-match semantics, including collisions.
class ScriptFunctionRegistry final {
public:
    static ScriptLookupStatus ReadSnapshot(std::uint32_t bucket_count,
        std::span<const ScriptSymbolBinding>, std::shared_ptr<const ScriptFunctionRegistry>&);
    ScriptLookupResult Find(std::string_view identifier) const;
private:
    explicit ScriptFunctionRegistry(std::uint32_t count):registry_(count) {}
    runtime::native::IdentifierRegistry<ScriptSymbolBinding> registry_;
};
}
