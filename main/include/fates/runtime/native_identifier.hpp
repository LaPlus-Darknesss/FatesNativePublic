#pragma once
#include <cstdint>
#include <cstddef>
#include <string_view>
#include <string>
#include <vector>
#include <utility>

namespace fates::runtime::native {
struct IdentifierHashes { std::uint32_t nameHash{}, bucketHash{}; };
// LDRSB supplies each contribution. Both polynomial accumulators wrap at 32 bits.
// Embedded NUL terminates just as it does for the original C string input.
constexpr IdentifierHashes HashIdentifierExact(std::string_view identifier) {
    IdentifierHashes result;
    for (unsigned char byte : identifier) {
        if (!byte) break;
        const auto value = static_cast<std::uint32_t>(byte < 128 ? int(byte) : int(byte)-256);
        result.nameHash = result.nameHash*31u + value;
        result.bucketHash = result.bucketHash*37u + value;
    }
    return result;
}
inline IdentifierHashes HashIdentifierExact(const char* identifier) {
    return identifier ? HashIdentifierExact(std::string_view(identifier)) : IdentifierHashes{};
}

// Portable storage for the original append/first-hash-match registry semantics.
// The shared retail entry pool and allocation failure remain service boundaries.
// A returned entry pointer is borrowed only until the next mutation.
template<class Value> class IdentifierRegistry {
public:
    struct Entry { std::uint32_t nameHash; std::string identifier; Value value; };
    explicit IdentifierRegistry(std::uint32_t bucket_count) : buckets_(bucket_count) {}
    const Entry* Find(const char* identifier) const {
        if (!identifier || buckets_.empty()) return nullptr;
        const auto hashes=HashIdentifierExact(identifier);
        for (const auto& entry:buckets_[hashes.bucketHash%buckets_.size()])
            if (entry.nameHash==hashes.nameHash) return &entry;
        return nullptr;
    }
    bool Append(const char* identifier,Value value) {
        if (!identifier || buckets_.empty()) return false;
        const auto hashes=HashIdentifierExact(identifier);
        buckets_[hashes.bucketHash%buckets_.size()].push_back({hashes.nameHash,identifier,std::move(value)});
        ++size_;++accounting_count_;return true;
    }
    // Retail Delete removes the first hash match, even if its spelling differs.
    // Its u32 accounting count decrements for a missing nonnull name as well.
    // Keep actual container size separate so underflow cannot control host memory.
    bool EraseFirst(const char* identifier) {
        if (!identifier || buckets_.empty()) return false;
        const auto hashes=HashIdentifierExact(identifier);
        auto& bucket=buckets_[hashes.bucketHash%buckets_.size()];
        --accounting_count_;
        for (auto it=bucket.begin();it!=bucket.end();++it) {
            if(it->nameHash!=hashes.nameHash)continue;
            bucket.erase(it);--size_;return true;
        }
        return false;
    }
    void Clear() { for(auto& bucket:buckets_)bucket.clear();size_=0;accounting_count_=0; }
    std::size_t Size() const {return size_;}
    std::uint32_t AccountingCount() const noexcept {return accounting_count_;}
private:
    std::vector<std::vector<Entry>> buckets_;
    std::size_t size_{};
    std::uint32_t accounting_count_{};
};
}
