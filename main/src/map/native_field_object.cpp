#include "fates/map/native_field_object.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace fates::map::native {
namespace {
std::string_view CString(std::string_view text) {return text.substr(0,text.find('\0'));}
std::string Bounded64(std::string text) {text.resize(std::min<std::size_t>(text.size(),63));return text;}
constexpr std::array<std::string_view,5> ModelSuffix{"none","map","low","high","near"};
void CheckModel(int index) {if(index < -1 || index > 3)throw std::invalid_argument("Field model index");}
void CheckState(int state) {if(state<0||state>2)throw std::invalid_argument("Field state");}
bool Finite(const FieldMatrix& matrix) {return std::all_of(matrix.values.begin(),matrix.values.end(),[](float x){return std::isfinite(x);});}
void CheckBounds(const FieldBounds& bounds) {
    if(!std::all_of(bounds.begin(),bounds.end(),[](float x){return std::isfinite(x);}))throw std::invalid_argument("Field bounds");
}
std::string LowerPartName(std::string_view name) {
    // sut::ToLower writes at most capacity-1 visible bytes. The native admission
    // currently covers ASCII identifiers; it does not guess the SDK locale table.
    auto text=CString(name);std::string lower(text.substr(0,31));
    for(auto& c:lower){const auto byte=static_cast<unsigned char>(c);if(byte>=128)throw std::invalid_argument("Unvalidated field identifier lowercase domain");if(c>='A'&&c<='Z')c=char(c+('a'-'A'));}
    return lower;
}
}
FieldBounds EmptyFieldBounds() noexcept {
    constexpr auto largest=std::numeric_limits<float>::max();return {largest,largest,largest,-largest,-largest,-largest};
}
bool IsEmptyFieldBounds(const FieldBounds& b) noexcept {return b[3]<b[0]||b[4]<b[1]||b[5]<b[2];}
std::string FieldModelName(const FieldHeightPart& part,int index) {
    CheckModel(index);return Bounded64(std::string(CString(part.name))+"_"+std::string(ModelSuffix[std::size_t(index+1)]));
}
std::string FieldAnimationName(const FieldHeightPart& part,int index,int state,bool access) {
    CheckModel(index);CheckState(state);if(index!=-1)index=part.model_indices[std::size_t(index)];CheckModel(index);
    constexpr std::array<std::string_view,3> idle{"idle","idle1","idle2"},use{"access","access1","access2"};
    // Do not call FieldModelName: snprintf truncates the final joined string,
    // rather than separately formatting/truncating an intermediate model name.
    return Bounded64(std::string(CString(part.name))+"_"+std::string(ModelSuffix[std::size_t(index+1)])+"_"+std::string((access?use:idle)[std::size_t(state)]));
}
std::string FieldResourcePath(std::string_view container,bool texture) {
    return Bounded64("field/"+std::string(CString(container))+(texture?"_t.bch.lz":".bch.lz"));
}
unsigned FieldTransferFlags(int mode) noexcept {return mode==0?0x42u:mode==1?0x44u:mode==2?0x49u:0u;}
int DefaultFieldLevel(unsigned selector) {if(selector>1)throw std::invalid_argument("Unvalidated default field level selector");return selector==0?0:2;}

void FreeFieldEffect(FieldEffectState& node,FieldEffectBackend& backend) {
    for(auto& handle:node.handles){backend.DeleteEffect(handle);handle={};}
}
void LoadFieldEffect(FieldEffectState& node,FieldEffectBackend& backend,const FieldEffectPlacement& placement) {
    FreeFieldEffect(node,backend);
    node.handles[0]=backend.CreateEffect(placement.label);
    if(const auto second=backend.SecondaryEffectLabel(node.handles[0]))node.handles[1]=backend.CreateEffect(second);
    for(const auto handle:node.handles)backend.SetEffectGroup(handle,1);
    node.pose=placement.pose;
}
void SetFieldEffectLevel(FieldEffectState& node,FieldEffectBackend& backend,int level) {
    if(node.level==level)return;
    if(node.handles[1]) {
        backend.SetEffectVisible(node.handles[0],level==0);backend.SetEffectStep(node.handles[0],level==0?1.0f:0.0f);
        backend.SetEffectVisible(node.handles[1],level!=0);backend.SetEffectStep(node.handles[1],level==0?0.0f:1.0f);
    }
    backend.SetEffectLocation(node.handles[0],level==0?1u:0u);node.level=level;
}
bool TransformFieldEffect(const FieldEffectState& node,FieldEffectBackend& backend,const FieldMatrix& parent) {
    if(!node.handles[0]&&!node.handles[1])return true;
    FieldMatrix local;if(!MakeFieldPoseMatrixExact(node.pose,local))return false;
    const auto world=MultiplyFieldMatrixExact(parent,local);if(!Finite(world))return false;
    for(const auto handle:node.handles)if(handle)backend.SetEffectTransform(handle,world);
    return true;
}

FieldObjectInstance::FieldObjectInstance(std::shared_ptr<const FieldHeightPart> part,
    std::optional<std::string_view> name,const FieldPose& pose,std::shared_ptr<FieldObjectBackend> backend,
    std::shared_ptr<HeightMapRange> pending,std::function<void()> world_pending):part_(std::move(part)),backend_(std::move(backend)),pending_(std::move(pending)),geometry_(part_),world_pending_(std::move(world_pending)) {
    if(!backend_||!pending_)throw std::invalid_argument("FieldObject requires owned backend and pending range");
    DefaultFieldLevel(backend_->DefaultLevelSelector());
    if(part_) {
        if(part_->type>6)throw std::invalid_argument("Unvalidated field part type");
        CheckBounds(part_->bounds);for(const auto index:part_->model_indices)CheckModel(index);
        for(int level=0;level<4;++level)ValidatePose(pose,level);
        const auto lower=LowerPartName(part_->name);
        if(lower.find("cas_")!=std::string::npos)state_.flags|=0x40;
        for(const auto token:{"tree","kusa","fort","pole","firewall","forest","forest","pillar"})
            if(lower.find(token)!=std::string::npos)state_.flags|=0x20;
        if(part_->model_indices[3]==-1)state_.flags|=0x20;
    }
    ValidatePose(pose,-1);state_.pose=pose;
    if(name) {
        const auto hashes=fates::runtime::native::HashIdentifierExact(*name);
        state_.display_name=std::string(CString(*name).substr(0,63));state_.name_hash=hashes.nameHash;state_.bucket=hashes.bucketHash%127;
    }
    for(unsigned slot=0;slot<4;++slot)backend_->ConstructActor(slot);
}
FieldObjectInstance::~FieldObjectInstance() {
    Free();ClearEffects();
    // __aeabi_vec_dtor traverses members in reverse construction order.
    for(unsigned slot=4;slot>0;--slot)backend_->DestroyActor(slot-1);
    backend_->ReleaseResource(state_.texture,true);backend_->ReleaseResource(state_.primary,false);
}
FieldMatrix FieldObjectInstance::ComposedMatrix(const FieldPose& pose,int level) const {
    CheckModel(level);FieldMatrix matrix;if(!MakeFieldPoseMatrixExact(pose,matrix))throw std::invalid_argument("Field pose");
    if(part_&&level!=-1) {
        FieldMatrix component;if(!MakeFieldPoseMatrixExact(part_->level_poses[std::size_t(level)],component))throw std::invalid_argument("Field component pose");
        matrix=MultiplyFieldMatrixExact(matrix,component);if(!Finite(matrix))throw std::invalid_argument("Field composed transform overflow");
    }
    return matrix;
}
void FieldObjectInstance::ValidatePose(const FieldPose& pose,int level) const {
    const auto matrix=ComposedMatrix(pose,level);
    if(part_) {
        FieldBounds bounds;
        if(!IsEmptyFieldBounds(part_->bounds)&&!TransformFieldBoundsExact(matrix,part_->bounds,bounds))throw std::invalid_argument("Field bounds transform");
        for(const auto& list:part_->effects)if(list)for(const auto& effect:list->records) {
            FieldMatrix local;if(!MakeFieldPoseMatrixExact(effect.pose,local)||!Finite(MultiplyFieldMatrixExact(matrix,local)))throw std::invalid_argument("Field effect pose");
        }
    }
}
void FieldObjectInstance::MarkDirty() {state_.flags|=2;if(world_pending_)world_pending_();backend_->MarkWorldPending();}
void FieldObjectInstance::ResetAccess() noexcept {state_.access_state=-1;state_.access_level=-1;state_.access_handle=0;state_.access_progress=0;}
void FieldObjectInstance::ClearEffects() {
    // Explicit head order rather than relying on a container's destruction order.
    for(auto& effect:state_.effects)FreeFieldEffect(effect,*backend_);state_.effects.clear();
}
void FieldObjectInstance::UpdateEffects(int state) {
    ClearEffects();const auto& list=part_->effects[std::size_t(state)];if(!list)return;
    const auto matrix=ComposedMatrix(state_.pose,state_.level);
    for(const auto& placement:list->records)if(backend_->HasEffectDefinition(placement.label)) {
        FieldEffectState node;LoadFieldEffect(node,*backend_,placement);
        SetFieldEffectLevel(node,*backend_,state_.level==-1?DefaultFieldLevel(backend_->DefaultLevelSelector()):state_.level);
        if(!TransformFieldEffect(node,*backend_,matrix))throw std::logic_error("Previously validated effect transform");
        state_.effects.push_back(std::move(node));
    }
}
void FieldObjectInstance::Load() {
    if(state_.flags&1)return;
    if(!part_){state_.flags|=1;return;}
    if(part_->type==1)return;
    if(!part_->bch_container)throw std::invalid_argument("Null field BCH format argument");
    const auto primary=backend_->ReadResource(FieldResourcePath(*part_->bch_container,false),FieldTransferFlags(backend_->TransferMode(false)));
    state_.primary=primary.resource;
    if(!primary.ready) {backend_->ReleaseResource(state_.primary,false);state_.primary.reset();state_.flags|=1;return;}
    const auto texture=backend_->ReadResource(FieldResourcePath(*part_->bch_container,true),FieldTransferFlags(backend_->TransferMode(true)));
    state_.texture=texture.resource;
    if(texture.ready)backend_->LinkResources(state_.primary,state_.texture);
    if(state_.primary) {
        constexpr std::array<std::int16_t,7> selectors{-11,-13,-8,-12,-11,0,-8};
        for(unsigned slot=0;slot<4;++slot)if(part_->model_indices[slot]!=-1) {
            auto& actor=state_.actors[slot];actor.model_name=FieldModelName(*part_,part_->model_indices[slot]);
            actor.model_loaded=backend_->LoadModel(slot,state_.primary,*actor.model_name);
            actor.selector_request=selectors[part_->type];backend_->SetupActor(slot,*actor.selector_request);
            if(part_->type==3){actor.added_model_flags|=0x800;backend_->SetActorModelFlags(slot,0x800);}
        }
        if(part_->type==4)for(unsigned slot=0;slot<4;++slot)backend_->ResetActorLocalBounds(slot);
    }
    const int state=state_.state,level=state_.level;state_.state=-1;state_.level=-1;
    SetState(state);SetLevel(level);state_.flags|=1;
}
void FieldObjectInstance::SetState(int value) {
    CheckState(value);if(state_.state==value)return;
    if(state_.primary) {
        if(!part_)throw std::logic_error("Resource without field part");
        for(unsigned slot=0;slot<4;++slot) {
            const auto model=part_->model_indices[slot];if(model==-1)continue;
            auto& actor=state_.actors[slot];actor.animation_mode_request=std::uint8_t(1);backend_->SetAnimationMode(slot,1);
            backend_->StopAnimations(slot);backend_->ResetAnimations(slot);
            const auto idle=FieldAnimationName(*part_,model,value,false);
            if(backend_->HasAnimation(state_.primary,idle)) {
                actor.animation_mode_request=std::uint8_t(value==0?0:1);backend_->SetAnimationMode(slot,*actor.animation_mode_request);
                if(backend_->TryPlayAnimations(slot,state_.primary,idle)) {
                    backend_->SetAnimationLoop(slot,true);if(value==0)backend_->RandomizeAnimation(slot);
                }
            }
            const auto access=FieldAnimationName(*part_,model,value,true);
            const bool has_access=backend_->HasAnimation(state_.primary,access);
            actor.animation_mode_request=std::uint8_t(2);backend_->SetAnimationMode(slot,2);
            if(has_access) {
                if(backend_->TryPlayAnimations(slot,state_.primary,access)) {
                    backend_->SetAnimationLoop(slot,false);backend_->SetAnimationStep(slot,1.0f);backend_->SetAnimationToEnd(slot);
                }
            } else backend_->StopAnimations(slot);
            backend_->UpdateActor(slot);
        }
        UpdateEffects(value);
    }
    state_.state=value;ResetAccess();MarkDirty();
}
void FieldObjectInstance::SetLevel(int value) {
    if(value==-1)value=DefaultFieldLevel(backend_->DefaultLevelSelector());CheckModel(value);
    if(state_.level==value)return;
    if(!part_)throw std::invalid_argument("SetLevel requires field part");
    ValidatePose(state_.pose,value);
    const int previous=state_.level==-1?-1:part_->model_indices[std::size_t(state_.level)];
    const int selected=value==-1?-1:part_->model_indices[std::size_t(value)];
    if(previous!=selected) {
        for(unsigned slot=0;slot<4;++slot){backend_->DetachActor(slot);state_.actors[slot].attached=false;}
        if(selected!=-1){backend_->AttachActor(unsigned(selected));state_.actors[std::size_t(selected)].attached=true;}
    }
    for(auto& effect:state_.effects)SetFieldEffectLevel(effect,*backend_,value);
    state_.level=value;UpdateTransform();
}
void FieldObjectInstance::SetPose(const FieldPose& pose) {ValidatePose(pose,state_.level);state_.pose=pose;MarkDirty();UpdateTransform();}
void FieldObjectInstance::SetTranslate(const std::array<float,3>& position) {auto pose=state_.pose;pose.position=position;SetPose(pose);}
void FieldObjectInstance::SetFlag(std::uint16_t flag,bool value) {
    if(bool(state_.flags&flag)==value)return;
    state_.flags=value?std::uint16_t(state_.flags|flag):std::uint16_t(state_.flags&~flag);MarkDirty();
}
void FieldObjectInstance::SetDispos(bool value) {SetFlag(8,value);}
void FieldObjectInstance::SetEscape(bool value) {SetFlag(16,value);}
void FieldObjectInstance::SetVisible(bool value) {for(unsigned slot=0;slot<4;++slot){backend_->SetActorVisible(slot,value);state_.actors[slot].visibility_request=value;}}
void FieldObjectInstance::UpdateTransform() {
    if(!part_)return;const auto matrix=ComposedMatrix(state_.pose,state_.level);
    if(IsEmptyFieldBounds(part_->bounds))state_.part_bounds=EmptyFieldBounds();
    else if(!TransformFieldBoundsExact(matrix,part_->bounds,state_.part_bounds))throw std::logic_error("Previously validated field bounds");
    for(unsigned slot=0;slot<4;++slot) {
        const auto model=backend_->ActorModelMatrix(slot);if(!Finite(model))throw std::logic_error("Backend supplied invalid model matrix");
        auto world=MultiplyFieldMatrixExact(matrix,model);if(!Finite(world))throw std::logic_error("Backend model transform overflow");
        state_.actors[slot].world_matrix=world;backend_->SetActorTransform(slot,world);
    }
    for(const auto& effect:state_.effects)if(!TransformFieldEffect(effect,*backend_,matrix))throw std::logic_error("Previously validated effect transform");
    state_.actor_bounds=EmptyFieldBounds();
    for(unsigned slot=4;slot>0;--slot) {
        const auto bounds=backend_->ActorLocalBounds(slot-1);CheckBounds(bounds);if(IsEmptyFieldBounds(bounds))continue;
        if(!TransformFieldBoundsExact(state_.actors[slot-1].world_matrix,bounds,state_.actor_bounds))throw std::logic_error("Backend supplied invalid actor bounds");
        break; // highest valid index wins; this is not a bounds union
    }
}
FieldGeometryUpdate FieldObjectInstance::UpdateDispos() {
    FieldGeometryRequest request{state_.pose,state_.state,state_.level,state_.flags};
    auto update=geometry_.Update(request,*pending_);if(update.status!=FieldGeometryStatus::Ok||!update.changed)return update;
    for(const auto event:update.events) {
        if(event==FieldGeometryEvent::UpdateVisualTransform)UpdateTransform();
        else {const bool remove=event==FieldGeometryEvent::RemoveHeight||event==FieldGeometryEvent::RemoveGeometry||event==FieldGeometryEvent::RemoveCollision;
            backend_->GeometryEvent(event,remove?update.before:update.after);}
    }
    state_.flags=request.flags;return update;
}
void FieldObjectInstance::Free() {
    if(!(state_.flags&1))return;
    for(unsigned slot=0;slot<4;++slot) {
        backend_->CleanupActor(slot);backend_->DetachActor(slot);state_.actors[slot].attached=false;
        backend_->FreeActor(slot);state_.actors[slot].model_name.reset();state_.actors[slot].model_loaded=false;
    }
    if(!geometry_.QueueActiveRemoval(*pending_))throw std::logic_error("Invalid active field height bounds");
    for(const auto event:{FieldGeometryEvent::RemoveHeight,FieldGeometryEvent::RemoveGeometry,FieldGeometryEvent::RemoveCollision})backend_->GeometryEvent(event,geometry_.Active());
    ClearEffects();backend_->ReleaseResource(state_.primary,false);state_.primary.reset();backend_->ReleaseResource(state_.texture,true);state_.texture.reset();
    geometry_.ClearActive();ResetAccess();geometry_.ClearCaches();state_.flags=std::uint16_t(state_.flags&~1u);
}
}
