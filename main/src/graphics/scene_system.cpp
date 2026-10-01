#include "fates/graphics/scene_system.hpp"
#include "fates/graphics/scene_node.hpp"
#include "fates/graphics/render_state.hpp"
#include <algorithm>

namespace {
std::array<float,4> gTimeRate{1.0f,1.0f,1.0f,1.0f};
util::FixedSizeArray<SceneNode*,1024> gRenderNodes;
}
void SceneSystem::Initialize(){ gRenderNodes.Clear(); gTimeRate.fill(1.0f); }
void SceneSystem::SetTimeRate(float rate){ gTimeRate.fill(rate); }
void SceneSystem::SetTimeRate(int index,float rate){ if(index>=0 && index<4) gTimeRate[static_cast<std::size_t>(index)]=rate; }
float SceneSystem::GetDeltaTime(int index){ if(index<0||index>=4) return 0.0f; return static_cast<float>(fates::decomp_detail::GetSceneDeltaTick())*gTimeRate[static_cast<std::size_t>(index)]; }
SceneSystem::SceneSystem(){ fates::decomp_detail::InitializeSceneRuntime(runtimeState_); }
SceneSystem::~SceneSystem(){ Free(); while(first_) first_->Detach(); fates::decomp_detail::DestroySceneRuntime(runtimeState_); }
void SceneSystem::SetFrame(float frame){ frame_=frame; }
void SceneSystem::UpdateFrame(){ if(runtimeState_.sceneContent) fates::decomp_detail::UpdateSceneFrame(runtimeState_,resources_,frame_); }
void SceneSystem::UpdateCamera(){ fates::decomp_detail::UpdateSceneCameraRuntime(runtimeState_); }
void SceneSystem::Draw(){ fates::decomp_detail::SubmitSceneDrawCallback(*this); }
void SceneSystem::Free(){ fates::decomp_detail::ResetSceneLights(runtimeState_); fates::decomp_detail::ClearSelectedScene(runtimeState_); resources_.Free(); runtimeState_.sceneContent=nullptr; frame_=0.0f; }
bool SceneSystem::Load(const char* path,const char* sceneName){ ResFile temporary; if(path==nullptr || !fates::decomp_detail::LoadSceneResourceFromPath(temporary,path)) return false; return Load(temporary,sceneName); }
bool SceneSystem::Load(const ResFile& resources,const char* sceneName){
    Free(); if(!resources.IsDone() || !fates::decomp_detail::SelectScene(runtimeState_,resources,sceneName)) return false;
    resources_.ReplaceHandle(resources); LoadLight(); SetFrame(0.0f); UpdateFrame(); return runtimeState_.sceneContent!=nullptr;
}
bool SceneSystem::Load(){ const ResFile* r=fates::decomp_detail::GetDefaultSceneResource(); return r!=nullptr && Load(*r,nullptr); }
void SceneSystem::Sort(util::FixedSizeArray<SceneNode*,1024>* nodes,bool reverse){ if(nodes==nullptr) return; auto less=[](SceneNode* a,SceneNode* b){ return a->sortDistance_<b->sortDistance_; }; if(reverse) std::stable_sort(nodes->Begin(),nodes->End(),[&](SceneNode* a,SceneNode* b){return less(b,a);}); else std::stable_sort(nodes->Begin(),nodes->End(),less); }
void SceneSystem::Render(util::FixedSizeArray<SceneNode*,1024>* nodes){
    if(nodes==nullptr || !fates::decomp_detail::IsSceneRenderable(runtimeState_)) return;
    UpdateCamera(); fates::decomp_detail::CommitSceneFogAndLights(runtimeState_,resources_); RenderState state(this); state.BeginCommand();
    Sort(nodes,false); for(auto p=nodes->Begin();p!=nodes->End();++p) if(*p) fates::decomp_detail::DrawSceneNodeOpaqueVirtual(**p,state);
    Sort(nodes,true); for(auto p=nodes->Begin();p!=nodes->End();++p) if(*p) fates::decomp_detail::DrawSceneNodeTranslucentVirtual(**p,state);
    state.EndCommand();
}
void SceneSystem::Update(){ if(!fates::decomp_detail::IsSceneRenderable(runtimeState_)) return; RenderState state(this); for(SceneNode* n=first_;n;){ SceneNode* next=n->next_; fates::decomp_detail::UpdateSceneNodeVirtual(*n,state); n=next; } }
void SceneSystem::OnBegin(){ if(!fates::decomp_detail::IsSceneRenderable(runtimeState_)) return; gRenderNodes.Clear(); for(SceneNode* n=first_;n;n=n->next_) if((n->flags_&0x1f)==0) gRenderNodes.PushBack(n); Render(&gRenderNodes); gRenderNodes.Clear(); }
void SceneSystem::OnEnd(){ /* exact retail entry is a four-byte no-op */ }
void SceneSystem::LoadLight(){ fates::decomp_detail::LoadSceneLights(runtimeState_,resources_); }
