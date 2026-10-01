#pragma once

class IdentHash;

class UniqueArchiveObject {
public:
    void Setup();
    void Cleanup();

    void* TryGet(const char* identifier) const;
    void* GetSurely(const char* identifier) const;

private:
    // Retail Setup/Cleanup dereference a pointer at object +0x70 and pass that
    // pointed-to archive buffer to ArchiveConstruct/ArchiveDestruct. The
    // readable source model keeps the storage as a pointer rather than
    // pretending those bytes are embedded archive data.
    void* archiveStorage_{};
    IdentHash* index_{};
};
