#include "fates/graphics/scene_node.hpp"
#include "fates/graphics/scene_system.hpp"
#include "fates/detail/scene_runtime.hpp"
#include <algorithm>
#include <limits>

namespace {
AABB invalidBox(){ AABB b{}; b.min={1.0f,1.0f,1.0f}; b.max={-1.0f,-1.0f,-1.0f}; return b; }
bool valid(const AABB& b){ return b.min.x<=b.max.x && b.min.y<=b.max.y && b.min.z<=b.max.z; }
}
SceneNode::SceneNode():sortDistance_(0.0f),localBox_(invalidBox()),worldBox_(invalidBox()){
    world_.m[0][0]=world_.m[1][1]=world_.m[2][2]=1.0f;
}
SceneNode::~SceneNode(){ Detach(); }
void SceneNode::SetVisible(int slot,bool visible){ const auto bit=static_cast<std::uint8_t>(1u<<(slot&7)); if(visible) flags_&=static_cast<std::uint8_t>(~bit); else flags_|=bit; }
bool SceneNode::IsVisible(int slot) const { return (flags_ & static_cast<std::uint8_t>(1u<<(slot&7)))==0; }
void SceneNode::SetLocalBox(const AABB& box){ localBox_=box; flags_|=kLocalBoxDirty; }
void SceneNode::SetLocalBox(const nn::math::VEC3& center,const nn::math::VEC3& size){
    const nn::math::VEC3 h{size.x*0.5f,size.y*0.5f,size.z*0.5f};
    localBox_.min={center.x-h.x,center.y-h.y,center.z-h.z}; localBox_.max={center.x+h.x,center.y+h.y,center.z+h.z}; flags_|=kLocalBoxDirty;
}
void SceneNode::ResetLocalBox(){ localBox_=invalidBox(); flags_|=kLocalBoxDirty; }
void SceneNode::SetWorldTranslate(const nn::math::VEC3& p){ world_.m[0][3]=p.x; world_.m[1][3]=p.y; world_.m[2][3]=p.z; flags_|=kWorldDirty; }
void SceneNode::UpdateCamera(const ICamera* camera){
    if(!valid(localBox_)){ flags_&=static_cast<std::uint8_t>(~kRejected); sortDistance_=0.0f; return; }
    if((flags_&(kLocalBoxDirty|kWorldDirty))!=0){ worldBox_=fates::decomp_detail::TransformAABB(localBox_,world_); flags_&=static_cast<std::uint8_t>(~(kLocalBoxDirty|kWorldDirty)); }
    sortDistance_=fates::decomp_detail::SceneNodeSquaredDistance(camera,worldBox_);
    if(fates::decomp_detail::SceneNodeRejected(camera,worldBox_)) flags_|=kRejected; else flags_&=static_cast<std::uint8_t>(~kRejected);
}
void SceneNode::Attach(SceneSystem* scene){ if(scene_==scene) return; Detach(); if(scene==nullptr) return; scene_=scene; prev_=scene->last_; next_=nullptr; if(prev_) prev_->next_=this; else scene->first_=this; scene->last_=this; ++scene->nodeCount_; }
void SceneNode::Detach(){ if(scene_==nullptr) return; if(prev_) prev_->next_=next_; else scene_->first_=next_; if(next_) next_->prev_=prev_; else scene_->last_=prev_; --scene_->nodeCount_; prev_=next_=nullptr; scene_=nullptr; }
