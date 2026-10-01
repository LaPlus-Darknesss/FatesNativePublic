#include "fates/io/delay_manager.hpp"
#include "fates/io/delay_queue.hpp"
#include <memory>
#include <new>
#include <vector>

namespace {
struct Deferred {DelayManager::Callback callback{};void* pointer{};};
using Queue=fates::io::DelayQueue<Deferred>;
Queue* gDelayState{};
// Initialize publishes without releasing the previous allocation. Old ticks
// retain their queue; host teardown is separate from game resource callbacks.
std::vector<std::unique_ptr<Queue>> queues;
void DeletePointer(void* pointer) {::operator delete(pointer);}
}
void DelayManager::Initialize() {
    auto queue=std::make_unique<Queue>();gDelayState=queue.get();queues.push_back(std::move(queue));
}
void DelayManager::Entry(Callback callback,void* pointer) {
    if(!callback)return; // Legacy null guard; native entry refuses it.
    if(!gDelayState || !gDelayState->Push({callback,pointer}))callback(pointer);
}
void DelayManager::Free(void* pointer) {if(pointer)Entry(&DeletePointer,pointer);}
void DelayManager::Tick() {
    auto* const queue=gDelayState;if(!queue)return;
    auto node=queue->Head();
    while(node!=Queue::None) {
        const auto next=queue->Next(node);
        if(queue->Ticks(node)>0)queue->WaitOne(node);
        else {const auto value=queue->Get(node);value.callback(value.pointer);queue->Recycle(node);}
        node=next;
    }
}
void DelayManager::Dump() {
    if(gDelayState)for(auto node=gDelayState->Head();node!=Queue::None;node=gDelayState->Next(node)){}
}
bool DelayManager::IsActive() {return gDelayState && gDelayState->size()>0;}
