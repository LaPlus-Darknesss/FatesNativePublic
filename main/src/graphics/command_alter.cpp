#include "fates/graphics/command_alter.hpp"
#include "fates/graphics/command_buffer.hpp"
#include "fates/detail/command_runtime.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>

namespace {
enum class AlterKind : std::uint16_t { Mtx34=1, Mtx44=2, OtherMatrix=3, RawCommand=4, Callback=5 };
struct AlterEntry {
    AlterKind kind{};
    std::uint16_t byteCount{};
    std::uint32_t commandOffset{};
    const void* payload{};
};
struct AlterState {
    std::array<AlterEntry,512> entries{}; // retail record arena is 0x1800 bytes / 512 records
    std::array<std::byte,0x3000> scratch{};
    std::size_t entryCount{};
    std::size_t scratchUsed{};
    std::uint32_t commandOffset{};
};
std::unique_ptr<AlterState> gState;
AlterState& S(){ if(!gState) gState=std::make_unique<AlterState>(); return *gState; }
void addBuffer(AlterKind kind,const void* data,std::size_t bytes){
    if(data==nullptr || bytes==0) return;
    auto& s=S(); if(s.entryCount>=s.entries.size()) return;
    void* dst=CommandAlter::Alloc(static_cast<unsigned int>(bytes)); if(dst==nullptr) return;
    std::memcpy(dst,data,bytes);
    s.entries[s.entryCount++]={kind,static_cast<std::uint16_t>(bytes),CommandBuffer::GetUsedBufferOffset(),dst};
}
}

void CommandAlter::Initialize(){ gState=std::make_unique<AlterState>(); }
void CommandAlter::Clear(){ auto& s=S(); s.entryCount=0; s.scratchUsed=0; s.commandOffset=0; }
void* CommandAlter::Alloc(unsigned int size){ auto& s=S(); if(s.scratchUsed+size>s.scratch.size()) return nullptr; void* p=s.scratch.data()+s.scratchUsed; s.scratchUsed+=size; return p; }
void CommandAlter::AddMtx44(const float* matrix){ addBuffer(AlterKind::Mtx44,matrix,16*sizeof(float)); }
void CommandAlter::AddMtx34(const float* matrix){ addBuffer(AlterKind::Mtx34,matrix,12*sizeof(float)); }
void CommandAlter::AddCommand(const unsigned int* words,int wordCount){ if(wordCount>0) addBuffer(AlterKind::RawCommand,words,static_cast<std::size_t>(wordCount)*4); }
void CommandAlter::AddCallback(Callback* callback){ auto& s=S(); if(callback==nullptr || s.entryCount>=s.entries.size()) return; s.entries[s.entryCount++]={AlterKind::Callback,0,CommandBuffer::GetUsedBufferOffset(),callback}; }
unsigned int CommandAlter::GetCommandOffset(){ return S().commandOffset; }
void CommandAlter::Execute(unsigned int destinationBase,unsigned int sourceBase,unsigned int commandOffset){
    auto& s=S(); s.commandOffset=commandOffset;
    for(std::size_t i=0;i<s.entryCount;i++){
        const auto& e=s.entries[i]; const std::uintptr_t dst=static_cast<std::uintptr_t>(destinationBase)+(e.commandOffset-sourceBase);
        switch(e.kind){
        case AlterKind::Mtx34: fates::decomp_detail::ReplacePicaCommand(dst,0,e.payload); break;
        case AlterKind::Mtx44: fates::decomp_detail::ReplacePicaCommand(dst,1,e.payload); break;
        case AlterKind::OtherMatrix: fates::decomp_detail::ReplacePicaCommand(dst,2,e.payload); break;
        case AlterKind::RawCommand: fates::decomp_detail::ReplaceRawCommand(dst,e.payload,e.byteCount); break;
        case AlterKind::Callback: s.commandOffset=static_cast<std::uint32_t>(dst); static_cast<Callback*>(const_cast<void*>(e.payload))->OnReplace(); break;
        }
    }
    Clear();
}
