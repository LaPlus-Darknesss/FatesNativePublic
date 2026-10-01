#include "fates/graphics/effect_package.hpp"

EffectPackage::~EffectPackage(){ Free(); }
void EffectPackage::Free(){ FileBase::Free(); }
bool EffectPackage::ReadSync(const char* path){
    const auto type=OpenFile(path,ReadType::TryImmediate);
    if(type==FileType::Missing) return false;
    FinishAsync();
    if(GetFileData()==nullptr){ Free(); return false; }
    return true;
}
void EffectPackage::ReadAsync(const char* path){
    const auto type=OpenFile(path,ReadType::Asynchronous);
    CloseFile(type,ReadType::Asynchronous);
}
