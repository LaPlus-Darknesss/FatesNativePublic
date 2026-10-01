#include "fates/presentation/live_field_scene.hpp"
#include <algorithm>
#include <cmath>
#include <set>

namespace fates::presentation::portable {
namespace {
bool Fail(std::string& error,const char* detail){error=detail;return false;}
bool MatchesAsset(const field::FieldObjectSceneState& object,unsigned slot,const FieldActorAsset& asset) {
    if(!asset.model)return true;
    if(!object.part||!object.part->bch_container||!asset.model->resource||!asset.model->resource->file)return false;
    if(asset.model->geometry.model.name!=object.actors[slot].model_name||
        asset.model->resource->path!=field::FieldResourcePath(*object.part->bch_container,false))return false;
    return !asset.linked_textures||(asset.linked_textures->file&&
        asset.linked_textures->path==field::FieldResourcePath(*object.part->bch_container,true));
}
bool ValidSnapshot(const field::FieldSceneSnapshot& snapshot,std::string& error) {
    if(!snapshot.world||snapshot.updates_pending)return Fail(error,"Field scene is absent or unsettled");
    std::set<std::uint64_t> instances;
    for(const auto& object:snapshot.objects) {
        if(!object.key||object.key.world!=snapshot.world||!instances.insert(object.key.instance.value).second)
            return Fail(error,"Invalid or repeated field object lifetime identity");
        for(const auto& actor:object.actors)if(actor.attached&&actor.model_loaded) {
            if(!actor.model_name)return Fail(error,"Loaded actor has no model name");
            for(float value:actor.world_matrix.values)if(!std::isfinite(value))return Fail(error,"Nonfinite owner-issued actor transform");
        }
    }
    return true;
}
}
bool CaptureFieldSceneAssets(std::shared_ptr<const field::FieldSceneSnapshot> snapshot,const FieldActorResolver& resolve,
    std::shared_ptr<const FieldSceneAssets>& output,std::string& error) {
    if(!snapshot||!ValidSnapshot(*snapshot,error))return snapshot?false:Fail(error,"No field scene snapshot");
    auto next=std::make_shared<FieldSceneAssets>();next->snapshot=std::move(snapshot);
    for(const auto& object:next->snapshot->objects) {
        FieldObjectAssets binding;binding.key=object.key;
        for(unsigned slot=0;slot<4;++slot) {
            const auto& actor=object.actors[slot];if(!actor.attached||!actor.model_loaded)continue;
            auto& asset=binding.actors[slot];
            if(!resolve){asset.issue="No field actor resource provider";continue;}
            asset=resolve(object,slot);
            if(!MatchesAsset(object,slot,asset))
                return Fail(error,"Captured model does not match native actor request");
            if(!asset.model&&asset.issue.empty())asset.issue="Native actor resource is unresolved";
        }
        next->objects.push_back(std::move(binding));
    }
    output=std::move(next);error.clear();return true;
}
bool LiveFieldSceneConsumer::Update(const field::NativeFieldSceneState& current,std::shared_ptr<const FieldSceneAssets> captured,std::string& error) {
    // Clear first: a refusal must not resurrect stale actors. Submitted frames
    // hold their own immutable owners and are unaffected by this current pointer.
    current_.reset();
    std::erase_if(retired_,[](const auto& weak){return weak.expired();});
    const bool changed_world=world_!=current.owner;
    if(changed_world&&current.owner)for(const auto& weak:retired_)if(weak.lock()==current.owner)return Fail(error,"Retired field world cannot become current again");
    auto accept=[&](std::shared_ptr<const FieldPresentationFrame> frame) {
        if(changed_world){if(world_)retired_.push_back(world_);world_=current.owner;}
        highest_revision_=current.owner?current.revision:0;current_=std::move(frame);error.clear();return true;
    };
    if(!current.owner) {
        if(current.snapshot||captured)return Fail(error,"Field resources without a current world");
        return accept({});
    }
    if(!changed_world&&current.revision<highest_revision_)return Fail(error,"Current field revision moved backwards");
    if(!current.snapshot) {
        if(captured)return Fail(error,"Captured resources without a published current scene");
        return accept({});
    }
    const auto& snapshot=*current.snapshot;
    if(snapshot.world!=current.owner||snapshot.revision!=current.revision||snapshot.terrain_revision!=current.terrain_revision)
        return Fail(error,"Current field publication identity/revision mismatch");
    if(!ValidSnapshot(snapshot,error))return false;
    if(!captured||captured->snapshot!=current.snapshot||captured->objects.size()!=snapshot.objects.size())
        return Fail(error,"Captured resources do not belong to this immutable publication");
    auto next=std::make_shared<FieldPresentationFrame>();next->assets=std::move(captured);
    for(std::size_t i=0;i<snapshot.objects.size();++i) {
        const auto& object=snapshot.objects[i];const auto& binding=next->assets->objects[i];
        if(binding.key!=object.key)return Fail(error,"Captured object lifetime mismatch");
        for(unsigned slot=0;slot<4;++slot) {
            const auto& actor=object.actors[slot];if(!actor.attached||!actor.model_loaded)continue;
            const auto& asset=binding.actors[slot];
            if(!asset.model){next->issues.push_back({object.key,slot,asset.issue.empty()?"Actor resource unavailable":asset.issue});continue;}
            if(!MatchesAsset(object,slot,asset))return Fail(error,"Actor resource changed after capture");
            const auto visible=actor.visibility_request?actor.visibility_request:asset.backend_visibility;
            if(!visible){next->issues.push_back({object.key,slot,"Actor visibility is unresolved"});continue;}
            if(!*visible)continue;
            next->actors.push_back({object.key,slot,object.state,object.level,actor.world_matrix,asset});
            if(!asset.issue.empty())next->issues.push_back({object.key,slot,asset.issue});
        }
    }
    return accept(std::move(next));
}
}
