#include "fates/graphics/vis_anim.hpp"
bool VisAnim::Play(const ResFile& r,int index){ return index!=0xffff && Alloc(r.GetVisAnim(index),r); }
void VisAnim::Stop(){ Free(); }
VisAnim::~VisAnim(){ Free(); }
