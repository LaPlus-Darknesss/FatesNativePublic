#include "fates/graphics/field_asset_backend.hpp"

namespace fates::graphics::portable {
field::FieldResourceRead FieldAssetBackend::ReadResource(std::string_view path,unsigned flags) {
    // Transfer flags select original upload placement, not source identity. The
    // portable mount stores immutable CPU data; GPU upload is a later backend.
    (void)flags;
    const auto result=store_->Read(path);
    if(!result){issues_.push_back(std::string(path)+": "+result.detail);return {};}
    auto binding=std::make_shared<FieldBchResource>();binding->decoded=result.resource;return {std::move(binding),true};
}
void FieldAssetBackend::LinkResources(const field::FieldResourceRef& primary,const field::FieldResourceRef& texture) {
    auto a=std::dynamic_pointer_cast<FieldBchResource>(primary),b=std::dynamic_pointer_cast<FieldBchResource>(texture);
    if(!a||!b||!a->decoded||!b->decoded){issues_.push_back("Link requires two ready portable resources");return;}
    a->linked=b->decoded;
}
bool FieldAssetBackend::LoadModel(unsigned slot,const field::FieldResourceRef& resource,std::string_view name) {
    auto& actor=actors_.at(slot);actor={};
    const auto binding=std::dynamic_pointer_cast<FieldBchResource>(resource);
    if(!binding||!binding->decoded){actor.issue="Model resource is not a ready portable binding";return false;}
    const auto found=store_->Model(binding->decoded,name);
    // The setup service retains its original LoadModel/FreeModel responsibility.
    // It sees actual decoded resource metadata even on a missing-name result.
    const bool initialized=services_->LoadModel(slot,resource,name);
    if(!found){actor.issue=found.detail;return false;}
    if(!initialized){actor.issue="Explicit actor setup service refused the decoded model";return false;}
    actor.model=found.model;actor.resource=binding;return true;
}
field::FieldMatrix FieldAssetBackend::ActorModelMatrix(unsigned slot) const {
    const auto& actor=actors_.at(slot);
    return actor.model?field::FieldMatrix{actor.model->geometry.model.matrix}:services_->ActorModelMatrix(slot);
}
fates::presentation::portable::FieldActorAsset FieldAssetBackend::Capture(unsigned slot) const {
    const auto& actor=actors_.at(slot);
    return {actor.model,actor.resource?actor.resource->linked:nullptr,visibility_?visibility_(slot):std::nullopt,actor.issue};
}
}
