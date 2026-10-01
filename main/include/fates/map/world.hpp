#pragma once

namespace map {

// Tier A/B boundary: map::World owns the tactical field presentation binding
// used by gameplay actors. Gameplay-visible visibility/lifecycle policy is
// preserved here; scene-node/vendor renderer storage intentionally remains OPAQUE.
class World {
public:
    static void Initialize();
    World();
    ~World();

    void HideIcon();
    void ShowIcon();
    void HideBalloon();
    void ShowBalloon();
    void UpdateHeight();
    float GetBalloonScale();
    void Bind();
    void Draw();
    void Tick();
    void Setup();
    void Unbind();
    void Cleanup();
    static void Finalize();
    void ShowUnit();
    void HideUnit();
};

} // namespace map
