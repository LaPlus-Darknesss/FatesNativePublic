#pragma once

#include <array>
#include <cstdint>

class FileBase;
class FileController;
class IAllocator;

class FileObject {
public:
    FileObject();
    virtual ~FileObject();

    // The retail FileObject vtable supplies these five subtype policies.
    // Modeling them as real virtual source methods removes the provisional
    // decomp_detail dispatch helpers introduced before the subtype evidence
    // was available.
    virtual void Setup() = 0;
    virtual void Cleanup() = 0;
    virtual IAllocator* GetAllocator() const = 0;
    virtual bool IsDelay() const = 0;
    virtual unsigned int GetAlign() const = 0;

    void Sweep();
    void Touch();
    bool ReadBinary(const char* path, const void* source, unsigned int size);
    void Resize(unsigned int size);

    const char* GetPath() const { return path_.data(); }
    void* GetData() const { return data_; }
    unsigned int GetSize() const { return size_; }
    unsigned int GetPriority() const { return priority_; }
    unsigned int GetCacheLevel() const { return cacheLevel_; }
    unsigned int GetBindCount() const { return bindCount_; }
    std::uint32_t GetFlags() const { return flags_; }

private:
    friend class FileBase;
    friend class FileController;

    void SetPath(const char* path);

    FileObject* cachePrevious_{};
    FileObject* cacheNext_{};
    FileController* cacheOwner_{};

    FileObject* asyncPrevious_{};
    FileObject* asyncNext_{};
    FileController* asyncOwner_{};

    std::array<char, 0x50> path_{};
    IAllocator* dataAllocator_{};
    void* data_{};
    unsigned int size_{};
    std::uint16_t bindCount_{};
    std::uint8_t cacheLevel_{};
    std::uint8_t priority_{1};
    std::uint32_t flags_{};
};
