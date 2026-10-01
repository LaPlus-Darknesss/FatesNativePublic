#pragma once
#include <cstdint>
#include "fates/graphics/effect_node.hpp"
#include "fates/graphics/game_effect.hpp"
class SceneSystem;
class ProcInst;
class GameEffectObject {
public:
    GameEffectObject();
    ~GameEffectObject();
    void UpdateMatrix();
    bool Init(float deltaFrame);
    bool Tick(float deltaFrame);
    bool Update(float deltaFrame);
    bool SkipTick();
    EffectNode& Node(){ return node_; }
    const EffectNode& Node() const { return node_; }
    void MarkDelete(){ flags_|=DeletePending; }
    bool DeletePendingNow() const { return (flags_&DeletePending)!=0; }
    std::uint32_t handle{};
    SceneSystem* scene{};
    const void* definition{};
    EffectCallback* callback{};
    ProcInst* boundProc{};
    nn::math::MTX34 transform{};
    nn::math::VEC3 translate{};
    Color8 color{255,255,255,255};
    float frame{};
    float stepFrame{1.0f};
    float elapsed{};
    float delay{};
    float emitterRatio{1.0f};
    int group{};
    int priority{};
    bool visible{true};
    bool eternal{};
private:
    enum Flag : std::uint32_t { Active=1u<<0, Initialized=1u<<1, Started=1u<<2, DeletePending=1u<<31 };
    enum class Phase : std::uint8_t { Initialize, WaitForResources, Running, Finished };
    EffectNode node_{};
    Phase phase_{Phase::Initialize};
    std::uint32_t flags_{Active};
};
