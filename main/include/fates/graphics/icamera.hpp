#pragma once
#include <array>
#include "fates/graphics/basic_types.hpp"
namespace DispType { enum class Type : int; }
class ICamera {
public:
    static void Initialize();
    ICamera();
    virtual ~ICamera();
    void SetupOrtho(DispType::Type display);
    void SetupOrtho(int width,int height);
    void Reset();
    void SetParallax(float eyeX,float eyeY,float eyeZ,float fovy,float depth);
    void SetParallax(const nn::math::MTX34& view,const nn::math::MTX44& proj,float depth);
    float GetParallax(float depth) const;
    bool IsStereoMode() const;
    const nn::math::MTX44& GetViewProj(int eye) const;
    void SetFrustum(float nearClip,float farClip,float fovy,float aspect);
    void UpdateProj(); void UpdateView(); void UpdateWorld(); void UpdateStereo(); void UpdateFrustum(); void Update();
    void SetRotateDeg(const nn::math::VEC3& degrees); void SetRotateDeg(float x,float y,float z);
    nn::math::VEC3 GetRotateDeg() const;
    nn::math::VEC3 GetWorldToScreen(const nn::math::VEC3& world) const;
    bool IsReject(const AABB& box) const; bool IsReject(const nn::math::VEC3& center,float radius) const;
    float GetSqDist(const AABB& box) const; float GetSqDist(const nn::math::VEC3& point) const;
    void CopyFrom(const ICamera* other);
    const nn::math::VEC3& Eye() const { return eye_; }
    const nn::math::VEC3& At() const { return at_; }
private:
    friend void fates_camera_runtime_access(ICamera&);
    nn::math::VEC3 eye_{}; nn::math::VEC3 at_{}; nn::math::VEC3 up_{0.0f,1.0f,0.0f}; nn::math::VEC3 rotateRad_{};
    float nearClip_{1.0f}; float farClip_{10000.0f}; float fovyRad_{0.78539816339f}; float aspect_{1.6666667f};
    int projectionMode_{}; bool stereoEnabled_{}; float stereoDepth_{};
    nn::math::MTX34 view_{}; nn::math::MTX44 proj_{}; std::array<nn::math::MTX44,2> viewProj_{};
    std::array<std::array<float,4>,6> frustum_{};
};
