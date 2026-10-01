#pragma once
#include "fates/graphics/portable_bch.hpp"
#include "fates/graphics/portable_geometry.hpp"
#include "fates/graphics/portable_material.hpp"
#include "fates/graphics/portable_texture.hpp"
#include <functional>

namespace fates::graphics::portable {
// One immutable mount supplies exact relative resource names. Transport failure
// is distinct from a readable but malformed resource. A new mount needs a new
// store; changing a file underneath a retained resource is not hot reload.
using AssetReader=std::function<bool(std::string_view,std::vector<std::uint8_t>&,std::string&)>;
enum class AssetStatus {Ready,Missing,Invalid,Ambiguous,Unsupported};
struct PortableBchResource {
    std::string path;
    std::shared_ptr<const BchFile> file;
    std::vector<BchModelDescriptor> models;
    std::vector<BchTextureDescriptor> textures;
};
struct ResourceResult {
    AssetStatus status{AssetStatus::Missing};
    std::shared_ptr<const PortableBchResource> resource;
    std::string detail;
    explicit operator bool() const noexcept{return status==AssetStatus::Ready&&bool(resource);}
};
struct PortableModelAsset {
    std::shared_ptr<const PortableBchResource> resource;
    unsigned index{};
    BchModelGeometry geometry;
    std::vector<BchMaterial> materials;
};
struct ModelResult {
    AssetStatus status{AssetStatus::Missing};
    std::shared_ptr<const PortableModelAsset> model;
    std::string detail;
    explicit operator bool() const noexcept{return status==AssetStatus::Ready&&bool(model);}
};
struct PortableTextureAsset {
    std::shared_ptr<const PortableBchResource> resource;
    BchTextureDescriptor descriptor;
    TextureImage image;
};
struct TextureResult {
    AssetStatus status{AssetStatus::Missing};
    std::shared_ptr<const PortableTextureAsset> texture;
    std::string detail;
    explicit operator bool() const noexcept{return status==AssetStatus::Ready&&bool(texture);}
};
// Host-side immutable asset retention, not the original ResFile state machine.
// Weak cache entries never keep a dead scene alive. Returned assets do. Reads
// and failures are retryable after all owners release a resource. Names are raw
// bytes, case sensitive; no aliases, directory scans or chapter-specific lists.
class PortableAssetStore {
public:
    explicit PortableAssetStore(AssetReader reader):reader_(std::move(reader)){}
    ResourceResult Read(std::string_view path);
    ModelResult Model(const std::shared_ptr<const PortableBchResource>&,std::string_view name);
    // Until vendor linking precedence is owned, two same-name sources are an
    // explicit ambiguity. No unrelated texture or white fallback is installed.
    TextureResult Texture(const std::shared_ptr<const PortableBchResource>& primary,
        const std::shared_ptr<const PortableBchResource>& linked,std::string_view name,
        bool color_2d_context);
    void CollectExpired();
private:
    AssetReader reader_;
    std::map<std::string,std::weak_ptr<const PortableBchResource>> resources_;
    std::vector<std::weak_ptr<const PortableModelAsset>> models_;
    std::vector<std::weak_ptr<const PortableTextureAsset>> textures_;
};
}
