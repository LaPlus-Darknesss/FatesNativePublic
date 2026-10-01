#pragma once
#include "fates/map/native_field_object.hpp"
#include "fates/map/native_field_configuration.hpp"
#include "fates/runtime/native_object_registry.hpp"

namespace fates::map::native {
// Host lifetime token. Retained snapshots keep it alive, preventing pointer
// reuse from making a replacement world look like a prior world.
struct FieldWorldIdentity {};
struct FieldObjectKey {
    std::shared_ptr<const FieldWorldIdentity> world;
    fates::runtime::native::ObjectIdentity instance;
    explicit operator bool() const noexcept {return world&&bool(instance);}
    bool operator==(const FieldObjectKey&) const=default;
};
struct FieldObjectSceneState {
    FieldObjectKey key;
    fates::runtime::native::ObjectHandle handle; // finite retail handle, not visual identity
    std::string source_resource;
    std::shared_ptr<const FieldHeightPart> part;
    std::optional<std::string> name;
    FieldPose pose;
    int state{},level{-1};
    std::uint16_t flags{};
    FieldBounds part_bounds,actor_bounds;
    std::array<FieldActorState,4> actors;
};
// One-way semantic data. No display/LCD coordinates, renderer-owned decisions,
// borrowed mutable object pointers or independently usable effect handles.
struct FieldSceneSnapshot {
    std::shared_ptr<const FieldWorldIdentity> world;
    std::uint64_t revision{},terrain_revision{};
    std::uint8_t chapter{};
    bool map_active{},load_succeeded{},updates_pending{};
    std::string requested_name;
    std::optional<std::string> field_name,environment_name;
    std::optional<std::string> scene_resource_path; // actual read or original Dummy fallback
    std::shared_ptr<const FieldConfiguration> configuration;
    std::vector<std::string> source_resources;
    std::vector<FieldObjectSceneState> objects;
};
// Current-world binding in NativeGameState. Only the native world writes this;
// consumers receive the immutable snapshot, never a second tactical model.
struct NativeFieldSceneState {
    std::shared_ptr<const FieldWorldIdentity> owner;
    std::uint64_t revision{},terrain_revision{};
    std::shared_ptr<const FieldSceneSnapshot> snapshot;
};
}
