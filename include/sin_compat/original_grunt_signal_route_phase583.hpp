#pragma once
// Independent authored host model for original SiN 2006 server.dll gamecode.
// Does NOT implement original engine action IDs, movement, physics or scheduling.
#include <cstdint>
namespace sin_compat { namespace original_grunt_signal_route_phase583 {
struct State {
    int32_t raw_0x878_current = 0;
    int32_t raw_0x880_request = -1;
    int32_t raw_0x884_schedule = -1;
    int32_t raw_0x888_mapped = -1;
    int32_t raw_0x88C_context = 0;
};
struct Hooks {
    virtual ~Hooks() = default;
    virtual int32_t translateAction(int32_t request,int32_t context) = 0;
    virtual int32_t lookupSchedule(int32_t mapped) = 0;
    virtual int32_t special15() = 0;
    // Native logging is conditioned on original global time, owner and strings.
    virtual void onUnmapped(int32_t mapped) = 0;
    virtual bool isActionRoutingAvailable() = 0;
    virtual void onTransition(int32_t request,int32_t schedule,
                              int32_t mapped,int32_t context) = 0;
};
// 0x22049A60. Original +0x884 receives the schedule, +0x888 receives
// the mapped numeric code. +0x88C is passed to the first translator.
// The native diagnostic time gate is not mapped: onUnmapped is an opaque
// callback to let the host express it without claiming its exact logger ABI.
inline void mapAction(State& state,int32_t request,Hooks& hooks) {
    state.raw_0x884_schedule=-1;
    state.raw_0x888_mapped=hooks.translateAction(request,state.raw_0x88C_context);
    if (request==15) {
        state.raw_0x884_schedule=hooks.special15();
        return;
    }
    state.raw_0x884_schedule=hooks.lookupSchedule(state.raw_0x888_mapped);
    if (state.raw_0x884_schedule != -1) return;
    hooks.onUnmapped(state.raw_0x888_mapped);
    if (state.raw_0x888_mapped==10) {
        state.raw_0x888_mapped=6;
        state.raw_0x884_schedule=hooks.lookupSchedule(6);
    }
    if (state.raw_0x884_schedule==-1) state.raw_0x884_schedule=0;
}
// 0x22049B50. A separate member +0x878 represents the native current
// code. The original explicitly permits request 0 through the special
// current==2 gate, and permits nonzero when previous +0x880==0x9A.
inline void applyRoutedAction(State& state,int32_t requested,Hooks& hooks) {
    if (!hooks.isActionRoutingAvailable()) return;
    if (state.raw_0x878_current==requested) return;
    if (requested!=0 && state.raw_0x878_current==2 && state.raw_0x880_request!=0x9A)
        return;
    state.raw_0x880_request=requested;
    mapAction(state,requested,hooks);
    hooks.onTransition(requested,state.raw_0x884_schedule,
                       state.raw_0x888_mapped,state.raw_0x88C_context);
}
}} // namespace sin_compat::original_grunt_signal_route_phase583