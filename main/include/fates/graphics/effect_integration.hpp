#pragma once
#include "fates/graphics/game_effect.hpp"
#include "fates/graphics/post_effect_context.hpp"
class ProcEffect { public: void Tick(); virtual ~ProcEffect()=default; GameEffect effect{}; void* proc{}; };
class ProcEffectDelayFree { public: void Persistent(); virtual ~ProcEffectDelayFree()=default; GameEffect effect{}; float lifetime{1.0f}; float remain{1.0f}; void* proc{}; };
class EffectTracking : public EffectCallback { public: void OnUpdate(GameEffect effect) override; ~EffectTracking() override=default; void* tracked{}; };
class BattleEffectCallback : public EffectCallback { public: void OnUpdate(GameEffect effect) override; ~BattleEffectCallback() override=default; void* object{}; unsigned int flags{}; nn::math::VEC3 extra{}; float lostDelay{}; };
class PrimPostEffectCallback { public: explicit PrimPostEffectCallback(PostEffectContext* context); ~PrimPostEffectCallback(); void Draw(); void OnBegin(); void OnEnd(); private: PostEffectContext* context_{}; };
