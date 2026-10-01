#pragma once
#include <cstdint>
#include "fates/graphics/basic_types.hpp"
class SceneSystem; class Model; class ICamera; class ProcInst;
namespace EffectGroup { enum class Type : int {}; }
namespace EffectDataLocation { enum class Type : int {}; }
namespace DispType { enum class Type : int {}; }
namespace prim::Target { enum class Type : std::uint8_t {}; }
class EffectCallback { public: virtual ~EffectCallback()=default; virtual void OnUpdate(class GameEffect)=0; };

class GameEffect {
public:
    GameEffect()=default;
    explicit GameEffect(std::uint32_t handle):handle_(handle){}
    static void Initialize();
    static void Finalize();
    static void EntryCache(const char* label);
    static void SweepCache();
    static GameEffect FindByGroup(EffectGroup::Type group);
    static GameEffect FindByLabel(const char* label);
    static void HideByGroup(int group);
    static void ShowByGroup(int group);
    static void DeleteByGroup(int group);
    static void CallbackByGroup(int group,EffectCallback* callback);
    static GameEffect DirectCreate(const char* label,SceneSystem* scene);
    static GameEffect Create(const char* label,SceneSystem* scene);
    bool IsValid() const { return handle_!=0; }
    void SetLayerId(int layer);
    void SetRotateY(float radians);
    void SetVisible(bool visible);
    void SetCallback(EffectCallback* callback);
    void SetLocation(EffectDataLocation::Type location);
    void SetPriority(int priority);
    void SetStepFrame(float step);
    void SetTranslate(const nn::math::VEC3& value);
    void SetTranslate(float x,float y);
    void SetTranslate(float x,float y,float z);
    void SetMapPosition(const nn::math::VEC3& value);
    void SetMapPosition(float x,float y,float z);
    void SetEmitterRatio(float ratio);
    static void SetupTelopCamera(ICamera* camera,DispType::Type display);
    void SetScreenPosition(float x,float y,float z);
    void Bind(ProcInst* proc);
    void Connect(ProcInst* proc);
    void FadeOut(int frames);
    void SetColor(const Color8& color);
    void SetDelay(int frames);
    void SetFrame(float frame);
    void SetGroup(EffectGroup::Type group);
    void SetIdling(int frames);
    void SetMatrix(const nn::math::MTX34& matrix);
    void SetRotate(const nn::math::MTX34& matrix);
    void SetStatus(unsigned int status);
    void SetTarget(prim::Target::Type target);
    void SetTimeCh(int channel);
    void Delete();
    void SetEternal(bool eternal);
    GameEffect FindByHandle() const;
    bool IsFinished() const;
    bool IsSeparated() const;
    nn::math::VEC3 GetTranslate() const;
    bool IsAsyncLoading() const;
    const void* GetData() const;
    bool IsAlive() const;
    float GetFrame() const;
    Model* GetModel() const;
    float GetElapse() const;
    float GetRemain() const;
    std::uint32_t Handle() const { return handle_; }
private:
    std::uint32_t handle_{};
};
