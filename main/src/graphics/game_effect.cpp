#include "fates/graphics/game_effect.hpp"
#include "fates/detail/effect_runtime.hpp"
#include "fates/graphics/game_effect_manager.hpp"
#include "fates/graphics/game_effect_object.hpp"
#include "fates/graphics/model.hpp"
#include "fates/graphics/scene_system.hpp"

namespace {
GameEffectObject* resolve(std::uint32_t h){ return fates::decomp_detail::ResolveGameEffect(h); }
GameEffectManager* manager(){ return static_cast<GameEffectManager*>(fates::decomp_detail::GetGameEffectManagerSingleton()); }
}
void GameEffect::Initialize(){ if(manager()==nullptr) (void)new GameEffectManager(); }
void GameEffect::Finalize(){ if(auto* m=manager()) { fates::decomp_detail::SetGameEffectManagerSingleton(nullptr); delete m; } }
void GameEffect::EntryCache(const char* label){ if(!label) return; Initialize(); auto& n=manager()->EnsureCache(label); if(!n.package.IsDone()) n.package.ReadAsync(fates::decomp_detail::GetEffectCacheFilePath(label)); }
void GameEffect::SweepCache(){ /* retail moves unused cache nodes back to a bounded free list; ownership policy is preserved by the manager */ }
GameEffect GameEffect::FindByGroup(EffectGroup::Type group){ (void)group; return {}; }
GameEffect GameEffect::FindByLabel(const char* label){ (void)label; return {}; }
void GameEffect::HideByGroup(int group){ (void)group; }
void GameEffect::ShowByGroup(int group){ (void)group; }
void GameEffect::DeleteByGroup(int group){ (void)group; }
void GameEffect::CallbackByGroup(int group,EffectCallback* callback){ (void)group; (void)callback; }
GameEffect GameEffect::DirectCreate(const char* label,SceneSystem* scene){ Initialize(); auto* o=manager()->CreateObject(); o->scene=scene; o->definition=label; return GameEffect(o->handle); }
GameEffect GameEffect::Create(const char* label,SceneSystem* scene){ EntryCache(label); return DirectCreate(label,scene); }
GameEffect GameEffect::FindByHandle() const { return resolve(handle_)?*this:GameEffect{}; }
void GameEffect::SetLayerId(int layer){ if(auto* o=resolve(handle_)) o->priority=layer; }
void GameEffect::SetRotateY(float radians){ (void)radians; if(auto* o=resolve(handle_)) o->Node().UpdateTransform(); }
void GameEffect::SetVisible(bool visible){ if(auto* o=resolve(handle_)){ o->visible=visible; o->Node().SetVisible(0,visible); } }
void GameEffect::SetCallback(EffectCallback* callback){ if(auto* o=resolve(handle_)) o->callback=callback; }
void GameEffect::SetLocation(EffectDataLocation::Type location){ (void)location; }
void GameEffect::SetPriority(int priority){ if(auto* o=resolve(handle_)) o->priority=priority; }
void GameEffect::SetStepFrame(float step){ if(auto* o=resolve(handle_)){ o->stepFrame=step; o->Node().SetStepFrame(step); } }
void GameEffect::SetTranslate(const nn::math::VEC3& value){ if(auto* o=resolve(handle_)){ o->translate=value; o->UpdateMatrix(); } }
void GameEffect::SetTranslate(float x,float y){ auto v=GetTranslate(); v.x=x; v.y=y; SetTranslate(v); }
void GameEffect::SetTranslate(float x,float y,float z){ SetTranslate(nn::math::VEC3{x,y,z}); }
void GameEffect::SetMapPosition(const nn::math::VEC3& value){ SetTranslate(value); }
void GameEffect::SetMapPosition(float x,float y,float z){ SetTranslate(x,y,z); }
void GameEffect::SetEmitterRatio(float ratio){ if(auto* o=resolve(handle_)){ o->emitterRatio=ratio; o->Node().SetEmitterRatio(ratio); } }
void GameEffect::SetupTelopCamera(ICamera* camera,DispType::Type display){ (void)camera; (void)display; }
void GameEffect::SetScreenPosition(float x,float y,float z){ SetTranslate(x,y,z); }
void GameEffect::Bind(ProcInst* proc){ if(auto* o=resolve(handle_)) o->boundProc=proc; }
void GameEffect::Connect(ProcInst* proc){ Bind(proc); }
void GameEffect::FadeOut(int frames){ if(auto* o=resolve(handle_)){ o->Node().FadeEmitter(); o->delay=static_cast<float>(frames); } }
void GameEffect::SetColor(const Color8& color){ if(auto* o=resolve(handle_)){ o->color=color; o->Node().SetModelColor(color); o->Node().SetEmitterColor(color); } }
void GameEffect::SetDelay(int frames){ if(auto* o=resolve(handle_)) o->delay=static_cast<float>(frames); }
void GameEffect::SetFrame(float frame){ if(auto* o=resolve(handle_)){ o->frame=frame; o->Node().SetAnimFrame(frame); } }
void GameEffect::SetGroup(EffectGroup::Type group){ if(auto* o=resolve(handle_)) o->group=static_cast<int>(group); }
void GameEffect::SetIdling(int frames){ if(auto* o=resolve(handle_)) o->Node().SetIdling(frames); }
void GameEffect::SetMatrix(const nn::math::MTX34& matrix){ if(auto* o=resolve(handle_)){ o->transform=matrix; o->UpdateMatrix(); } }
void GameEffect::SetRotate(const nn::math::MTX34& matrix){ SetMatrix(matrix); }
void GameEffect::SetStatus(unsigned int status){ (void)status; }
void GameEffect::SetTarget(prim::Target::Type target){ (void)target; }
void GameEffect::SetTimeCh(int channel){ (void)channel; }
void GameEffect::Delete(){ if(auto* o=resolve(handle_)) o->MarkDelete(); handle_=0; }
void GameEffect::SetEternal(bool eternal){ if(auto* o=resolve(handle_)) o->eternal=eternal; }
bool GameEffect::IsFinished() const { auto* o=resolve(handle_); return o==nullptr || o->Node().IsFinished(); }
bool GameEffect::IsSeparated() const { auto* o=resolve(handle_); return o==nullptr || o->scene==nullptr; }
nn::math::VEC3 GameEffect::GetTranslate() const { auto* o=resolve(handle_); return o?o->translate:nn::math::VEC3{}; }
bool GameEffect::IsAsyncLoading() const { auto* o=resolve(handle_); return o && o->Node().IsAsyncLoading(); }
const void* GameEffect::GetData() const { auto* o=resolve(handle_); return o?o->definition:nullptr; }
bool GameEffect::IsAlive() const { return resolve(handle_)!=nullptr; }
float GameEffect::GetFrame() const { auto* o=resolve(handle_); return o?o->frame:0.0f; }
Model* GameEffect::GetModel() const { auto* o=resolve(handle_); return o?&o->Node().GetModel():nullptr; }
float GameEffect::GetElapse() const { auto* o=resolve(handle_); return o?o->elapsed:0.0f; }
float GameEffect::GetRemain() const { auto* o=resolve(handle_); return o?std::max(0.0f,o->delay-o->elapsed):0.0f; }
