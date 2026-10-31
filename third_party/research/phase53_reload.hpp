#pragma once
// Phase 53: deliberately bounded clean-room reference for observed reload paths.
// NOT a drop-in Source SDK implementation or recovered Ritual source.
#include <algorithm>
#include <cstdint>
namespace sin53 {
constexpr int ACT_VM_RELOAD=166;
enum class Activity { Reload=ACT_VM_RELOAD, Start=-2, End=-3 };
struct Clip { int loaded=0, capacity=0, reserve=0; };
struct PlayerAmmo { Clip primary{},secondary{}; };
struct Transfer { int primary=0,secondary=0; };
inline int transfer_one(Clip &c,bool enabled=true) {
    if (!enabled || c.capacity<=0 || c.loaded<0 || c.reserve<=0) return 0;
    const int missing=std::max(0,c.capacity-c.loaded);
    const int moved=std::min(missing,c.reserve);
    c.loaded+=moved; c.reserve-=moved;
    return moved;
}
inline Transfer transfer_ammo(PlayerAmmo &a,bool usesPrimary=true,bool usesSecondary=false) {
    return {transfer_one(a.primary,usesPrimary),transfer_one(a.secondary,usesSecondary)};
}
struct ReloadState {
    int stage=0;            // observed +0x4A8: values 1, 2 and 3
    bool active=false;      // observed +0x4AC
    bool mode508=false;     // observed +0x508, exact per-gun setting not recovered
    float nextAttackTime=0; // observed +0x4A0
};
struct ActivityChoice { Activity selected; int requestedId; bool fallback; int newStage; };
// The binary selects START except when stage==3; then it selects END.
// Lookup failures fall back to the generic ACT_VM_RELOAD (166).
inline ActivityChoice select_reload_activity(ReloadState& state, int startLookup, int endLookup) {
    const bool ending=state.stage==3;
    const Activity preferred=ending?Activity::End:Activity::Start;
    if(!ending)state.stage=2;
    int id=ending?endLookup:startLookup;
    if(id<0) { state.stage=1;return {Activity::Reload,ACT_VM_RELOAD,true,state.stage}; }
    return {preferred,id,false,state.stage};
}
struct ShellStep { int inserted=0; bool interrupted=false; bool finished=false; };
// Normalized test abstraction for one observed mode-508 processing branch.
// This does not pretend to reproduce scheduling, animation or engine side effects.
inline ShellStep single_round_tick(Clip& c,ReloadState& state,bool timeReached,bool attackPressed) {
    if(!state.mode508 || !state.active || !timeReached)return {};
    if(attackPressed && c.loaded>0) {state.active=false;return {0,true,false};}
    if(c.loaded>=c.capacity || c.reserve<=0) {state.active=false;return {0,false,true};}
    const int moved=std::min(1,std::min(c.capacity-c.loaded,c.reserve));
    c.loaded+=moved;c.reserve-=moved;
    return {moved,false,false};
}
// Assault-rifle slot224 delegates to the shared chooser, but can restore the
// earlier +0x4A0 deadline in its own post-call condition.
inline ActivityChoice rifle_stage_wrapper(ReloadState& state,int startId,int endId,bool preserveOldDeadline) {
    float saved=state.nextAttackTime;
    auto result=select_reload_activity(state,startId,endId);
    if(preserveOldDeadline)state.nextAttackTime=saved;
    return result;
}
} // namespace sin53
