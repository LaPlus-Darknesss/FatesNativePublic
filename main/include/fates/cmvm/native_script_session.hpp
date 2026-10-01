#pragma once
#include "fates/cmvm/native_script_functions.hpp"
#include "fates/event/native_phase_event_catalog.hpp"
#include <map>

namespace fates::cmvm::native {
class ScriptAttachmentSession;
class ScriptAttachment final {
public:
    ScriptAttachment()=default;
    explicit operator bool() const noexcept {return bool(state_);}
    bool attached() const noexcept;
    std::shared_ptr<const ScriptFunctionTable> table() const noexcept;
private:
    friend class ScriptAttachmentSession;
    struct State {
        std::shared_ptr<const ScriptFunctionTable> table;
        std::shared_ptr<const char> session_identity;
        std::shared_ptr<const void> storage_identity;
        bool attached{true};
    };
    std::shared_ptr<State> state_;
};
class AttachedPhaseSelection final {
public:
    explicit operator bool() const noexcept {return bool(selection_);}
    const event::native::PhaseEventSelection& selection() const noexcept {return selection_;}
    const ScriptAttachment& attachment() const noexcept {return attachment_;}
private:
    friend class ScriptAttachmentSession;
    event::native::PhaseEventSelection selection_;
    ScriptAttachment attachment_;
};
// A function bound to one particular attachment lifetime, not just archive bytes.
// Only the owning session can construct it. Reattachment cannot revive it.
class AttachedScriptFunction final {
public:
    explicit operator bool() const noexcept {return bool(function_);}
    const ScriptFunctionRef& function() const noexcept {return function_;}
    const ScriptAttachment& attachment() const noexcept {return attachment_;}
private:
    friend class ScriptAttachmentSession;
    ScriptFunctionRef function_;
    ScriptAttachment attachment_;
};
enum class ScriptSessionStatus : std::uint8_t {
    Ok, Retired, InvalidBucketCount, InvalidIdentifier, NullArchive,
    UnsupportedStorage, DirectEntryRequired, InvalidFunctions, AlreadyAttached,
    DetachedLinkRequired, ForeignAttachment, DetachedAttachment, LimitExceeded,
    RevisionExhausted, Found, NoMatchInSession, InvalidKind, ForeignSelection,
    InvalidArchiveName
};
enum class LiveScriptLookupStatus : std::uint8_t {
    FoundScript, NativeOwnerRequired, Missing, InvalidIdentifier, Retired, DetachedFunction
};
struct LiveScriptLookup {
    LiveScriptLookupStatus status{LiveScriptLookupStatus::Missing};
    std::string registered_identifier;
    ScriptFunctionRef function;
    ScriptAttachment attachment;
};

// Mutable native owner of an explicitly supplied script-attachment chronology.
// It does NOT infer the retail chapter loader's complete inputs or bucket count.
// First admission supports unrelocated zero-static/no-direct-entry archives,
// covering the supplied original corpus. Other headers are explicit refusals.
// Single-threaded with VM ticks: mutate between Run calls, never concurrently.
class ScriptAttachmentSession final {
public:
    static ScriptSessionStatus Create(std::uint32_t bucket_count,
        std::span<const std::string> native_names,std::shared_ptr<ScriptAttachmentSession>&);
    ~ScriptAttachmentSession();
    ScriptAttachmentSession(const ScriptAttachmentSession&)=delete;
    ScriptAttachmentSession& operator=(const ScriptAttachmentSession&)=delete;
    // File loaders supply the retained image identity so two loaders cannot
    // turn one already-linked image into independent parsed attachments.
    ScriptSessionStatus Attach(std::shared_ptr<const event::native::PhaseEventArchive>,ScriptAttachment& out,
        std::shared_ptr<const void> storage_identity={});
    ScriptSessionStatus Detach(const ScriptAttachment&);
    // CmGetArchiveByName: first exact embedded name in the CURRENT list. This
    // is byte-string comparison, unlike the named-function hash registry.
    ScriptSessionStatus FindArchiveByName(std::string_view,ScriptAttachment& out) const;
    void Retire() noexcept;
    bool retired() const noexcept {return retired_;}
    bool IsAttached(const ScriptAttachment&) const noexcept;
    LiveScriptLookup Find(std::string_view) const;
    ScriptSessionStatus SelectFunction(const ScriptAttachment&,std::size_t index,AttachedScriptFunction& out) const;
    // Original typed iteration only. No phase/area/flag inspector is implied.
    // Every call traverses the current list, anchored to a still-live function.
    ScriptSessionStatus FindNextTyped(std::uint8_t type,const AttachedScriptFunction* previous,
        AttachedScriptFunction& out) const;
    ScriptSessionStatus FindNext(event::native::PhaseEventKind,std::uint16_t turn,std::uint8_t force,
        const AttachedPhaseSelection* previous,AttachedPhaseSelection& out) const;
    std::span<const ScriptAttachment> attachments() const noexcept {return attachments_;}
    std::size_t symbol_count() const noexcept {return registry_.Size();}
    std::uint32_t accounting_count() const noexcept {return registry_.AccountingCount();}
    std::uint64_t revision() const noexcept {return revision_;}
private:
    explicit ScriptAttachmentSession(std::uint32_t count):registry_(count),identity_(std::make_shared<const char>(char{})) {}
    struct History {
        std::weak_ptr<const void> storage;
        bool detached_with_next{};
    };
    runtime::native::IdentifierRegistry<ScriptSymbolBinding> registry_;
    std::shared_ptr<const char> identity_;
    std::vector<ScriptAttachment> attachments_;
    std::shared_ptr<const event::native::PhaseEventCatalog> catalog_;
    std::map<const void*,History> history_;
    std::uint64_t revision_{};
    bool retired_{};
};
}
