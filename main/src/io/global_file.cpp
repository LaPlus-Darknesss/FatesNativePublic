#include "fates/io/global_file.hpp"

#include "fates/detail/file_registry_runtime.hpp"
#include "fates/engine/ident_hash.hpp"
#include "fates/io/file_base.hpp"

#include <string>

namespace {

struct SharedFileRegistry;

struct SharedFileEntry {
    SharedFileEntry* previous{};
    SharedFileEntry* next{};
    SharedFileRegistry* owner{};
    unsigned int refCount{};
    FileBase* file{};
    std::string key{};
};

struct SharedFileRegistry {
    SharedFileEntry* head{};
    SharedFileEntry* tail{};
    int count{};
    IdentHash index{0x7F};

    SharedFileEntry* Find(const char* path) {
        if (path == nullptr) {
            return nullptr;
        }
        return static_cast<SharedFileEntry*>(index.GetSurely(path));
    }

    void* Load(const char* path, unsigned int priority, bool archiveMode) {
        if (path == nullptr) {
            return nullptr;
        }

        SharedFileEntry* entry = Find(path);
        if (entry == nullptr) {
            entry = new SharedFileEntry{};
            entry->owner = this;
            entry->key = path;
            entry->file = fates::decomp_detail::OpenSharedFileBase(
                path,
                priority,
                archiveMode);

            entry->previous = tail;
            if (tail != nullptr) {
                tail->next = entry;
            }
            if (head == nullptr) {
                head = entry;
            }
            tail = entry;
            ++count;
            index.Set(entry->key.c_str(), entry);
        }

        ++entry->refCount;
        return entry->file != nullptr ? entry->file->GetFileData() : nullptr;
    }

    void Free(const char* path) {
        SharedFileEntry* const entry = Find(path);
        if (entry == nullptr || entry->refCount == 0) {
            return;
        }

        --entry->refCount;
        if (entry->refCount != 0) {
            return;
        }

        index.Delete(entry->key.c_str());
        if (entry->previous != nullptr) {
            entry->previous->next = entry->next;
        } else {
            head = entry->next;
        }
        if (entry->next != nullptr) {
            entry->next->previous = entry->previous;
        } else {
            tail = entry->previous;
        }
        --count;

        fates::decomp_detail::DestroySharedFileBase(entry->file);
        delete entry;
    }
};

SharedFileRegistry* gFileRegistry{};
SharedFileRegistry* gArchiveRegistry{};
SharedFileRegistry* gUnknownRegistry{};

} // namespace

void GlobalFile::Initialize() {
    gFileRegistry = new SharedFileRegistry{};
    gArchiveRegistry = new SharedFileRegistry{};
    gUnknownRegistry = new SharedFileRegistry{};
}

void* GlobalFile::FileLoad(const char* path, unsigned int priority) {
    return gFileRegistry != nullptr
        ? gFileRegistry->Load(path, priority, false)
        : nullptr;
}

void GlobalFile::FileFree(const char* path) {
    if (gFileRegistry != nullptr) {
        gFileRegistry->Free(path);
    }
}

void* GlobalFile::ArchiveLoad(const char* path, unsigned int priority) {
    return gArchiveRegistry != nullptr
        ? gArchiveRegistry->Load(path, priority, true)
        : nullptr;
}

void GlobalFile::ArchiveFree(const char* path) {
    if (gArchiveRegistry != nullptr) {
        gArchiveRegistry->Free(path);
    }
}

bool GlobalFile::IsFileExist(const char* path) {
    return gFileRegistry != nullptr && gFileRegistry->Find(path) != nullptr;
}
