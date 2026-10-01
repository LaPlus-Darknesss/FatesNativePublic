#pragma once
#include "fates/map/native_height_archive.hpp"
#include "fates/runtime/native_identifier.hpp"
#include <functional>
#include <optional>

namespace fates::map::native {
struct FieldReferData {
    std::string name,part_name; // opaque original CP932 bytes
    FieldPose pose;
};
struct FieldSourceArchive {
    // Missing labels differ from present, empty lists. A descriptor needs Files
    // and References; a registered resource needs Parts. No path interpretation.
    std::optional<FieldHeightArchive> parts;
    std::optional<std::vector<std::string>> files;
    std::optional<std::vector<FieldReferData>> references;
};
bool DecodeFieldSourceArchive(std::span<const std::uint8_t>,FieldSourceArchive&);

struct FieldSourceResource {
    std::string name;
    std::shared_ptr<const FieldSourceArchive> archive;
};
struct FieldSourcePart {
    std::shared_ptr<const FieldSourceResource> resource;
    std::shared_ptr<const FieldHeightPart> part;
};
struct FieldSourcePlacement {
    FieldSourcePart source;
    std::string name;
    FieldPose pose;
};
enum class FieldSourceEventKind {
    ReadField,ReadResource,DestroyFailedResource,ConstructAndLoadObject,
    SetDisposition,ReleaseField,DestroyObject,ReleaseResource,
    ClearFileRegistry,ClearPartRegistry
};
struct FieldSourceEvent {
    FieldSourceEventKind kind;
    std::string identifier;
    std::shared_ptr<const FieldSourceResource> resource;
    std::shared_ptr<const FieldSourcePlacement> placement;
};
struct FieldSourceLoadResult {bool loaded{};std::vector<FieldSourceEvent> events;};
using FieldSourceProvider=std::function<std::shared_ptr<const FieldSourceArchive>(std::string_view)>;

// Synchronous service points, not a replay of the returned diagnostic batch.
// Callbacks may inspect registry membership, but may not mutate/reenter it or
// throw. ConstructAndLoad precedes insertion; destruction follows removal.
// Clear-registry events occur at call entry, before the corresponding clear.
class FieldSourceObserver {
public:
    virtual ~FieldSourceObserver()=default;
    virtual void OnSourceEvent(const FieldSourceEvent&)=0;
    virtual void OnPlacementInserted(const std::shared_ptr<const FieldSourcePlacement>&)=0;
};

// Owns source registration and placement admission. Events expose the original
// ordered object-service calls, not completed FieldObject construction/Load/Free.
// No loaded flag, state, level, visual resource success or world binding inferred.
// Providers supply immutable archives, are synchronous and must not reenter this
// owner. Allocation failure/throwing services are outside the admitted domain.
class FieldSourceRegistry {
public:
    FieldSourceRegistry()=default;
    FieldSourceRegistry(const FieldSourceRegistry&)=delete;
    FieldSourceRegistry& operator=(const FieldSourceRegistry&)=delete;
    FieldSourceLoadResult LoadField(std::string_view,const FieldSourceProvider&,FieldSourceObserver* =nullptr);
    bool EntryData(std::string_view,const FieldSourceProvider&,std::vector<FieldSourceEvent>&,FieldSourceObserver* =nullptr);
    FieldSourcePart FindPart(const char*) const;
    // Dynamic creation does not set disposition; LoadField explicitly does so.
    std::shared_ptr<const FieldSourcePlacement> CreateObject(const FieldReferData&,
        std::vector<FieldSourceEvent>&,FieldSourceObserver* =nullptr);
    std::shared_ptr<const FieldSourcePlacement> CreateObject(const char* part_name,
        const FieldSourceProvider&,std::vector<FieldSourceEvent>&,FieldSourceObserver* =nullptr);
    bool DeleteObject(const std::shared_ptr<const FieldSourcePlacement>&,
        std::vector<FieldSourceEvent>&,FieldSourceObserver* =nullptr);
    std::vector<FieldSourceEvent> FreeField(FieldSourceObserver* =nullptr);
    const auto& Resources() const {return resources_;}
    const auto& Placements() const {return placements_;}
private:
    fates::runtime::native::IdentifierRegistry<std::shared_ptr<const FieldSourceResource>> files_{127};
    fates::runtime::native::IdentifierRegistry<FieldSourcePart> parts_{127};
    std::vector<std::shared_ptr<const FieldSourceResource>> resources_;
    std::vector<std::shared_ptr<const FieldSourcePlacement>> placements_;
};
}
