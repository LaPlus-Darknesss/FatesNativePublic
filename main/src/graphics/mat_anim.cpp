#include "fates/graphics/mat_anim.hpp"
bool MatAnim::Play(const ResFile& r,int index){ return index!=0xffff && Alloc(r.GetMatAnim(index),r); }
void MatAnim::Stop(){ /* exact retail entry 0x004D76B4 is a four-byte no-op */ }
MatAnim::~MatAnim(){ Free(); }
