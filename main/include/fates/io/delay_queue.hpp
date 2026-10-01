#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace fates::io {
// Shared list kernel. The caller saves Next before invoking a callback, then
// recycles that node after it returns. Appends can affect later traversal.
template<class Value> class DelayQueue final {
public:
    using Index=std::uint16_t;
    static constexpr Index None=0xffff;
    static constexpr std::size_t Capacity=0x2000;
    DelayQueue() {
        for(std::size_t i=0;i<Capacity;++i) {
            nodes_[i].previous=i?static_cast<Index>(i-1):None;
            nodes_[i].next=i+1<Capacity?static_cast<Index>(i+1):None;
        }
    }
    bool Push(Value value) {
        if(free_tail_==None)return false;
        const auto id=free_tail_;auto& n=nodes_[id];free_tail_=n.previous;
        if(free_tail_!=None)nodes_[free_tail_].next=None;else free_head_=None;
        --free_count_;n.previous=tail_;n.next=None;
        if(tail_!=None)nodes_[tail_].next=id;else head_=id;
        tail_=id;++count_;n.value=std::move(value);n.ticks=2;return true;
    }
    Index Head() const noexcept {return head_;}
    Index Next(Index i) const noexcept {return nodes_[i].next;}
    std::int32_t Ticks(Index i) const noexcept {return nodes_[i].ticks;}
    const Value& Get(Index i) const noexcept {return nodes_[i].value;}
    void WaitOne(Index i) noexcept {--nodes_[i].ticks;}
    std::size_t size() const noexcept {return count_;}
    std::size_t free_size() const noexcept {return free_count_;}
    void Recycle(Index i) {
        auto& n=nodes_[i];
        if(n.next!=None)nodes_[n.next].previous=n.previous;else tail_=n.previous;
        if(n.previous!=None)nodes_[n.previous].next=n.next;else head_=n.next;
        --count_;n.previous=free_tail_;n.next=None;
        if(free_tail_!=None)nodes_[free_tail_].next=i;else free_head_=i;
        free_tail_=i;++free_count_;
        // Retail leaves callback/value/tick fields untouched in the free list.
    }
private:
    struct Node {Index previous{None},next{None};Value value{};std::int32_t ticks{};};
    std::array<Node,Capacity> nodes_;
    Index head_{None},tail_{None},free_head_{0},free_tail_{Capacity-1};
    std::size_t count_{},free_count_{Capacity};
};
}
