#pragma once
#include <array>
#include <cstddef>

namespace util {
template <class T, std::size_t Capacity>
class FixedSizeArray {
public:
    bool PushBack(const T& value) {
        if (size_ >= Capacity) return false;
        values_[size_++] = value;
        return true;
    }
    void Clear() { size_ = 0; }
    std::size_t Size() const { return size_; }
    bool Empty() const { return size_ == 0; }
    T* Begin() { return values_.data(); }
    T* End() { return values_.data() + size_; }
    const T* Begin() const { return values_.data(); }
    const T* End() const { return values_.data() + size_; }
    T& operator[](std::size_t i) { return values_[i]; }
    const T& operator[](std::size_t i) const { return values_[i]; }
private:
    std::array<T,Capacity> values_{};
    std::size_t size_{};
};
}
