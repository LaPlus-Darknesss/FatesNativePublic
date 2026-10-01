#pragma once
#include <array>
#include "fates/detail/scene_runtime.hpp"
#include "fates/engine/fixed_size_array.hpp"
#include "fates/io/res_file.hpp"

class SceneNode;
class SceneSystem {
public:
    static void Initialize();
    static void SetTimeRate(float rate);
    static void SetTimeRate(int index,float rate);
    static float GetDeltaTime(int index);

    SceneSystem();
    ~SceneSystem();
    void SetFrame(float frame);
    void UpdateFrame();
    void UpdateCamera();
    void Draw();
    void Free();
    bool Load(const char* path,const char* sceneName=nullptr);
    bool Load(const ResFile& resources,const char* sceneName=nullptr);
    bool Load();
    void Sort(util::FixedSizeArray<SceneNode*,1024>* nodes,bool reverse);
    void Render(util::FixedSizeArray<SceneNode*,1024>* nodes);
    void Update();
    void OnBegin();
    void OnEnd();
    void LoadLight();
private:
    friend class SceneNode;
    SceneNode* first_{}; SceneNode* last_{}; int nodeCount_{};
    float frame_{};
    ResFile resources_{};
    fates::decomp_detail::SceneRuntimeState runtimeState_{};
};
