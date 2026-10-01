#pragma once

class FileController;
class IAllocator;

namespace fates::decomp_detail {

// Platform-facing boundaries retained from the Nintendo CTR runtime.
// The promoted File/Compress source owns the game-side orchestration while
// these helpers isolate nn::fs / nn::cfg / nn::cx and retail-global plumbing.
void InitializePlatformFileSystem();
void InstallFileController(FileController* controller);
void ConfigurePlatformFileLatency();
void MountRomFileSystem();
IAllocator* GetDefaultFileAllocator();

bool TryGetPlatformFileSize(
    const wchar_t* path,
    unsigned int& size);
int ReadPlatformFileRange(
    const wchar_t* path,
    unsigned int offset,
    void* destination,
    unsigned int size);

void UncompressPlatformLz(const void* source, void* destination);

} // namespace fates::decomp_detail
