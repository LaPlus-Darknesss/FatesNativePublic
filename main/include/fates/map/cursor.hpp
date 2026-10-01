#pragma once
class Stream;
class Terrain;
class ICamera;

namespace map {

// Tactical cursor semantics. Retail's exact argument type for IsHook is
// game::graphics::GameCursor::FreeMode; the enum values remain opaque here so
// we do not duplicate a vendor/presentation-owned declaration.
class Cursor {
public:
    static ICamera* GetCamera();
    static const Terrain* GetTerrain(int x, int y);
    static void Initialize();
    static void Deserialize(Stream* stream);
    static void TickDistance();
    static Cursor* Get();
    bool IsHook(int freeMode, int x, int y, int previousX, int previousY);
    static void Finalize();
    void TickType();
    void Serialize(Stream* stream) const;
};

} // namespace map
