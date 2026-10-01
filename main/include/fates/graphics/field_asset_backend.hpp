#pragma once
#include "fates/presentation/live_field_scene.hpp"
#include <stdexcept>

namespace fates::graphics::portable {
namespace field=fates::map::native;
// A per-object ResFile replacement binding. Linking changes this binding only;
// captured frames retain the immutable linked source that existed at capture.
struct FieldBchResource final:field::FieldResource {
    std::shared_ptr<const PortableBchResource> decoded,linked;
};
// Real transport/model ownership over the existing platform-service seam. The
// caller must supply actor setup/bounds, animation/effect, collision and default
// visibility services. This adapter does not pretend those services are owned.
// LoadModel services receive a FieldBchResource and can inspect its real content.
// Static serialized model matrices come from the retained decoded model; world
// transforms still belong entirely to FieldObjectInstance.
class FieldAssetBackend final:public field::FieldObjectBackend {
public:
    using VisibilityReader=std::function<std::optional<bool>(unsigned)>;
    FieldAssetBackend(std::shared_ptr<PortableAssetStore> store,std::shared_ptr<field::FieldObjectBackend> services,VisibilityReader visibility)
        :store_(std::move(store)),services_(std::move(services)),visibility_(std::move(visibility)) {
        if(!store_||!services_)throw std::invalid_argument("Field assets require a store and explicit platform services");
    }
    fates::presentation::portable::FieldActorAsset Capture(unsigned) const;
    const std::vector<std::string>& ResourceIssues() const noexcept{return issues_;}
    unsigned DefaultLevelSelector() const override{return services_->DefaultLevelSelector();}
    int TransferMode(bool texture) override{return services_->TransferMode(texture);}
    field::FieldResourceRead ReadResource(std::string_view,unsigned) override;
    void ReleaseResource(field::FieldResourceRef& value,bool) override{value.reset();}
    void LinkResources(const field::FieldResourceRef&,const field::FieldResourceRef&) override;
    bool LoadModel(unsigned,const field::FieldResourceRef&,std::string_view) override;
    field::FieldMatrix ActorModelMatrix(unsigned) const override;
    void ConstructActor(unsigned s) override{actors_.at(s)={};services_->ConstructActor(s);}
    void DestroyActor(unsigned s) override{services_->DestroyActor(s);actors_.at(s)={};}
    void FreeActor(unsigned s) override{services_->FreeActor(s);actors_.at(s)={};}
    void SetupActor(unsigned s,std::int16_t v) override{services_->SetupActor(s,v);}
    void CleanupActor(unsigned s) override{services_->CleanupActor(s);}
    void AttachActor(unsigned s) override{services_->AttachActor(s);}
    void DetachActor(unsigned s) override{services_->DetachActor(s);}
    void SetActorModelFlags(unsigned s,std::uint16_t v) override{services_->SetActorModelFlags(s,v);}
    void ResetActorLocalBounds(unsigned s) override{services_->ResetActorLocalBounds(s);}
    field::FieldBounds ActorLocalBounds(unsigned s) const override{return services_->ActorLocalBounds(s);}
    void SetActorTransform(unsigned s,const field::FieldMatrix& m) override{services_->SetActorTransform(s,m);}
    void SetActorVisible(unsigned s,bool v) override{services_->SetActorVisible(s,v);}
    void SetAnimationMode(unsigned s,std::uint8_t v) override{services_->SetAnimationMode(s,v);}
    void StopAnimations(unsigned s) override{services_->StopAnimations(s);}
    void ResetAnimations(unsigned s) override{services_->ResetAnimations(s);}
    bool HasAnimation(const field::FieldResourceRef& r,std::string_view n) override{return services_->HasAnimation(r,n);}
    bool TryPlayAnimations(unsigned s,const field::FieldResourceRef& r,std::string_view n) override{return services_->TryPlayAnimations(s,r,n);}
    void SetAnimationLoop(unsigned s,bool v) override{services_->SetAnimationLoop(s,v);}
    void RandomizeAnimation(unsigned s) override{services_->RandomizeAnimation(s);}
    void SetAnimationStep(unsigned s,float v) override{services_->SetAnimationStep(s,v);}
    void SetAnimationToEnd(unsigned s) override{services_->SetAnimationToEnd(s);}
    void UpdateActor(unsigned s) override{services_->UpdateActor(s);}
    bool HasEffectDefinition(const std::optional<std::string>& n) override{return services_->HasEffectDefinition(n);}
    void MarkWorldPending() override{services_->MarkWorldPending();}
    void GeometryEvent(field::FieldGeometryEvent e,const field::FieldGeometrySnapshot& s) override{services_->GeometryEvent(e,s);}
    field::FieldEffectHandle CreateEffect(const std::optional<std::string>& n) override{return services_->CreateEffect(n);}
    std::optional<std::string> SecondaryEffectLabel(field::FieldEffectHandle h) override{return services_->SecondaryEffectLabel(h);}
    void DeleteEffect(field::FieldEffectHandle h) override{services_->DeleteEffect(h);}
    void SetEffectGroup(field::FieldEffectHandle h,unsigned v) override{services_->SetEffectGroup(h,v);}
    void SetEffectVisible(field::FieldEffectHandle h,bool v) override{services_->SetEffectVisible(h,v);}
    void SetEffectStep(field::FieldEffectHandle h,float v) override{services_->SetEffectStep(h,v);}
    void SetEffectLocation(field::FieldEffectHandle h,unsigned v) override{services_->SetEffectLocation(h,v);}
    void SetEffectTransform(field::FieldEffectHandle h,const field::FieldMatrix& m) override{services_->SetEffectTransform(h,m);}
private:
    struct Actor {std::shared_ptr<const PortableModelAsset> model;std::shared_ptr<FieldBchResource> resource;std::string issue;};
    std::shared_ptr<PortableAssetStore> store_;
    std::shared_ptr<field::FieldObjectBackend> services_;
    VisibilityReader visibility_;
    std::array<Actor,4> actors_;
    std::vector<std::string> issues_;
};
}
