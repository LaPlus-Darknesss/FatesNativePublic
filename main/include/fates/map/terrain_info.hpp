#pragma once
namespace map {
class TerrainInfo {
public:
    static void Initialize();
    static TerrainInfo* Get();
    void Draw();
    void HideForDesc();
    void Hide();
    void Show(int x,int y);
    static void Finalize();
    void DrawTransformAccessForTargetSelect(float alpha);
};
}
