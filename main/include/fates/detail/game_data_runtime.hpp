#pragma once

#include "fates/detail/game_data_archive.hpp"
#include "fates/game/belong.hpp"
#include "fates/game/chapter.hpp"
#include "fates/game/equip_skill.hpp"
#include "fates/game/experience.hpp"
#include "fates/game/item.hpp"
#include "fates/game/item_kinds.hpp"
#include "fates/game/job.hpp"
#include "fates/game/person.hpp"
#include "fates/game/personality.hpp"
#include "fates/game/terrain_cost.hpp"
#include "fates/game/weapon_data.hpp"

class ArchiveFile;

class GeoAttr {
public:
    static void Initialize();
    static void Finalize();
};

class CameraData {
public:
    static void Initialize();
    static void Finalize();
};

class EffectData {
public:
    static void Initialize();
    static void Finalize();
};

class Tutorial {
public:
    static void Initialize(const void*, int);
    static void Finalize();
};

class WeaponLevel {
public:
    static void Initialize(const void*);
    static void Finalize();
};

class AchieveBonus {
public:
    static void Initialize(const void*);
};

class Arena {
public:
    static void Initialize(const void*);
};

namespace fates::decomp_detail {

// Provisional readable name for the ArchiveFile* stored through retail global
// 0x006D3868 by GameData::Initialize.
extern ArchiveFile* gGameDataArchive;

// These helpers isolate ArchiveFile's not-yet-promoted constructor/destructor
// layer. The file/cache ownership machinery remains deliberately outside
// durable source until its intrusive-list and eviction semantics are named.
ArchiveFile* CreateArchiveFile(const char* path);
const GameDataArchiveRoot32* GetGameDataArchiveRoot(const ArchiveFile* archive);
void DestroyGameDataArchive();

} // namespace fates::decomp_detail
