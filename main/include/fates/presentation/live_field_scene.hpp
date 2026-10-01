#pragma once
#include "fates/map/native_field_scene.hpp"
#include "fates/graphics/portable_assets.hpp"
#include <functional>

namespace fates::presentation::portable {
namespace field=fates::map::native;
namespace assets=fates::graphics::portable;
// Captured host-resource result. Default visibility is supplied by the actor's
// backend, never guessed by the consumer. Owner-issued visibility takes priority.
struct FieldActorAsset {
    std::shared_ptr<const assets::PortableModelAsset> model;
    std::shared_ptr<const assets::PortableBchResource> linked_textures;
    std::optional<bool> backend_visibility;
    std::string issue;
};
using FieldActorResolver=std::function<FieldActorAsset(const field::FieldObjectSceneState&,unsigned)>;
struct FieldObjectAssets {field::FieldObjectKey key;std::array<FieldActorAsset,4> actors;};
struct FieldSceneAssets {
    std::shared_ptr<const field::FieldSceneSnapshot> snapshot;
    std::vector<FieldObjectAssets> objects;
};
// Capture while the published scene/backend relationship is current. Its result
// and each underlying asset are immutable and may outlive the world. The callback
// supplies resources; it cannot change membership, LOD, state or transforms.
bool CaptureFieldSceneAssets(std::shared_ptr<const field::FieldSceneSnapshot>,const FieldActorResolver&,
    std::shared_ptr<const FieldSceneAssets>& output,std::string& error);
struct FieldDrawActor {
    field::FieldObjectKey key;
    unsigned slot{};
    int state{},level{};
    field::FieldMatrix world_matrix;
    FieldActorAsset asset;
};
struct FieldPresentationIssue {field::FieldObjectKey key;unsigned slot{};std::string detail;};
struct FieldPresentationFrame {
    std::shared_ptr<const FieldSceneAssets> assets;
    std::vector<FieldDrawActor> actors;
    std::vector<FieldPresentationIssue> issues;
};
// Consumes the actual NativeGameState current binding. Missing/unsettled/invalid
// current input clears the current draw set; it never leaves an old world on
// screen. A separately retained old frame remains valid for in-flight rendering.
class LiveFieldSceneConsumer {
public:
    bool Update(const field::NativeFieldSceneState&,std::shared_ptr<const FieldSceneAssets>,std::string& error);
    const std::shared_ptr<const FieldPresentationFrame>& Current() const noexcept{return current_;}
private:
    std::shared_ptr<const field::FieldWorldIdentity> world_;
    std::vector<std::weak_ptr<const field::FieldWorldIdentity>> retired_;
    std::uint64_t highest_revision_{};
    std::shared_ptr<const FieldPresentationFrame> current_;
};
}
