#include "fates/graphics/game_effect_manager.hpp"
#include "fates/detail/effect_runtime.hpp"
#include <algorithm>
#include <cstring>

GameEffectManager::CacheNode::~CacheNode(){ package.Free(); }
GameEffectManager::GameEffectManager(){ fates::decomp_detail::SetGameEffectManagerSingleton(this); }
void GameEffectManager::Persistent(){}
GameEffectObject* GameEffectManager::CreateObject(){ auto p=std::make_unique<GameEffectObject>(); auto* raw=p.get(); objects_.push_back(std::move(p)); return raw; }
void GameEffectManager::Tick(){
    for(auto& p:objects_) if(p) (void)p->Update(1.0f);
    objects_.erase(std::remove_if(objects_.begin(),objects_.end(),[](const auto& p){ return !p || p->DeletePendingNow(); }),objects_.end());
}
GameEffectManager::CacheNode* GameEffectManager::FindCache(const char* label){
    for(auto& n:cache_){
        if(n && n->label==label) return n.get();
    }
    return nullptr;
}
GameEffectManager::CacheNode& GameEffectManager::EnsureCache(const char* label){
    if(auto* n=FindCache(label)) return *n;
    auto p=std::make_unique<CacheNode>(); p->label=label?label:""; auto* raw=p.get(); cache_.push_back(std::move(p)); return *raw;
}
