#pragma once

#include "fates/io/global_file.hpp"

class IdentHash;

namespace fates::decomp_detail {

// The retail message manager owns an IdentHolder used for file bindings and a
// separate identifier hash populated by loaded message archives. Their
// implementations remain behind this boundary until IdentHolder/GlobalFile are
// reconstructed as their own coherent ownership layer.
extern IdentHash* gMessageIdentHash;

void InitializeMessageArchiveRegistry();
void FinalizeMessageArchiveRegistry();
bool BindMessageArchive(const char* name);
bool UnbindMessageArchive(const char* name);
bool IsMessageArchiveBound(const char* name);
void SetMessageArchivePointer(const char* name, void* archive);

inline void* LoadMessageArchiveFile(const char* path) {
    return GlobalFile::ArchiveLoad(path, 0);
}
inline void FreeMessageArchiveFile(const char* path) {
    GlobalFile::ArchiveFree(path);
}

// Retail $Nu expansion uses the current player unit name when available.
const wchar_t* GetPlayerUnitName();

// Tutorial presentation chooses suffixes from retail gameplay/config flags.
bool IsTutorialCasualMode();
bool IsTutorialSimpleMode();
bool IsTutorialEntrustMode();

} // namespace fates::decomp_detail
