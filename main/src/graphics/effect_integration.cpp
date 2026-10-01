#include "fates/graphics/effect_integration.hpp"
#include "fates/detail/camera_execution_runtime.hpp"
#include <algorithm>
void ProcEffect::Tick(){ if(!effect.IsAlive()) fates::decomp_detail::ProcAdvance(proc); }
void ProcEffectDelayFree::Persistent(){ const float denom=lifetime>0.0f?lifetime:1.0f; const float alpha=remain/denom; Color8 c{255,255,255,static_cast<unsigned char>(std::clamp(alpha,0.0f,1.0f)*255.0f)}; effect.SetColor(c); remain-=fates::decomp_detail::GetWorldTimeRate(); if(remain<0.0f){effect.Delete();fates::decomp_detail::ProcDelete(proc);} }
void EffectTracking::OnUpdate(GameEffect e){ if(void* o=fates::decomp_detail::ResolveTrackingObject(tracked)) fates::decomp_detail::ApplyTrackingEffect(e,o); else fates::decomp_detail::CleanupTrackingEffect(e); }
void BattleEffectCallback::OnUpdate(GameEffect e){ if(void* o=fates::decomp_detail::ResolveBattleObject(object)) fates::decomp_detail::ApplyBattleEffect(e,o,flags,&extra); else if(lostDelay<=0.0f) e.Delete(); else lostDelay=std::max(0.0f,lostDelay-fates::decomp_detail::GetWorldTimeRate()); }
PrimPostEffectCallback::PrimPostEffectCallback(PostEffectContext* c):context_(c){} PrimPostEffectCallback::~PrimPostEffectCallback()=default; void PrimPostEffectCallback::Draw(){fates::decomp_detail::PrimPostEffectDraw(this);} void PrimPostEffectCallback::OnBegin(){if(context_)fates::decomp_detail::PrimPostEffectBegin(*context_);} void PrimPostEffectCallback::OnEnd(){fates::decomp_detail::PrimPostEffectEnd();}
