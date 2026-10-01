#pragma once
class Terrain {
public:
    static void Initialize(const char* archiveName);
    static const char* GetFieldName();
    static const void* GetImageData();
    static const void* TryGetImageData();
    static const Terrain* Get(const char* identifier);
    static const Terrain* Get(int terrainId);
    static const Terrain* TryGet(const char* identifier);
    static void Finalize();

    bool IsCastleUnitDispos() const;
    const wchar_t* GetName() const;
};
