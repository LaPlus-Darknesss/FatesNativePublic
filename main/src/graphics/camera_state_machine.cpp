#include "fates/graphics/camera_state_machine.hpp"
#include <algorithm>
void CameraStateMachine::Tick(){ for(auto* s:states_) if(s&&s->exiting) s->Leave(); if(states_.empty()) return; auto* s=states_.front(); if(!s) return; if(!s->entered){s->entered=true;s->Enter();} if(!s->exiting) s->Tick(); if(s->exiting){s->Leave();states_.erase(states_.begin());} }
void CameraStateMachine::ChangeState(CameraState* s){ if(!s) return; if(!states_.empty()) states_.front()->exiting=true; states_.push_back(s); }
void CameraStateMachine::ClearState(){ states_.clear(); }
CameraState* CameraStateMachine::GetCurrentState(){ for(auto* s:states_) if(s&&!s->exiting) return s; return nullptr; }
const CameraState* CameraStateMachine::GetCurrentState() const { for(auto* s:states_) if(s&&!s->exiting) return s; return nullptr; }
bool CameraStateMachine::IsAllowedToEnd() const { auto* s=GetCurrentState(); return s==nullptr||s->allowEnd; }
void CameraStateMachine::ChangeInput(){if(auto*s=GetCurrentState())s->ChangeInput();} void CameraStateMachine::ChangeToNormal(){if(auto*s=GetCurrentState())s->ChangeToNormal();} void CameraStateMachine::ChangeToWinCut(int i){if(auto*s=GetCurrentState())s->ChangeToWinCut(i);} void CameraStateMachine::ChangeToDeathCut(int i){if(auto*s=GetCurrentState())s->ChangeToDeathCut(i);} void CameraStateMachine::ChangeCut(){if(auto*s=GetCurrentState())s->ChangeCut();}
