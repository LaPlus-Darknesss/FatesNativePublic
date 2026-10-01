#pragma once
#include <memory>
#include <string>
#include <vector>
#include "fates/graphics/effect_package.hpp"
#include "fates/graphics/game_effect_object.hpp"
class GameEffectManager {
public:
    struct CacheNode {
        CacheNode()=default;
        ~CacheNode();
        std::string label{};
        EffectPackage package{};
    };
    GameEffectManager();
    ~GameEffectManager()=default; // deleting-destructor retail entry remains ABI evidence only
    void Persistent();
    GameEffectObject* CreateObject();
    void Tick();
    CacheNode* FindCache(const char* label);
    CacheNode& EnsureCache(const char* label);
private:
    std::vector<std::unique_ptr<GameEffectObject>> objects_{};
    std::vector<std::unique_ptr<CacheNode>> cache_{};
};
