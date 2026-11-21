#pragma once
// Authored host reference bounded by original 2006 SiN server.dll VAs.
// Not original commercial source, a native x87 emulator, or engine scheduler.
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
namespace sin_compat { namespace original_grunt_event_signals_phase582 {
struct State {
    int32_t raw_0x880 = -1;
    uint8_t raw_0xA9 = 0;
    uint8_t raw_0xC3C = 1;
    float raw_0xC20 = 0.0f;
    float raw_0xC38 = 0.0f;
};
struct SignalHooks {
    virtual ~SignalHooks() = default;
    virtual void debugSignal(int32_t code) = 0;
    virtual void ownerVirtual4DC(int32_t arg) = 0;
    virtual bool canRouteAction() = 0;
    virtual void routeAction(int32_t code) = 0;
};
// Original 0x22049C20: code 2 is a no-op before even the debug option.
// Code 0 dispatches owner+0x4DC(0); other codes overwrite owner+0x880.
// The native debug formatter and routing helper are supplied by hooks.
inline void numericSignal(State& s, int32_t code, bool debugGlobal,
                          SignalHooks& hooks) {
    if (code == 2) return;
    if (debugGlobal && (s.raw_0xA9 & 0x10U)) hooks.debugSignal(code);
    if (code == 0) { hooks.ownerVirtual4DC(0); return; }
    s.raw_0x880 = code;
    if (code == 0x9A) return;
    if (hooks.canRouteAction()) hooks.routeAction(code);
}
// 0x22028200: original fnstcw/truncation/and-0xFFFF/fild/x87 scaling.
// This is a finite double approximation; x87 extended-precision boundary
// behavior and masked invalid exceptions are not claimed to be reproduced.
inline float quantizeDegreesHost(float input) {
    if (!std::isfinite(input)) throw std::domain_error("nonfinite angle unsupported");
    constexpr double encode = 182.0444488525390625;
    constexpr double decode = 0.0054931640625;
    const double scaled = std::trunc(static_cast<double>(input)*encode);
    if (scaled < static_cast<double>(std::numeric_limits<int32_t>::min()) ||
        scaled > static_cast<double>(std::numeric_limits<int32_t>::max()))
        throw std::out_of_range("native x87 integer exception unsupported");
    const uint32_t fixed = static_cast<uint32_t>(static_cast<int32_t>(scaled)) & 0xFFFFU;
    return static_cast<float>(static_cast<double>(fixed)*decode);
}
struct EventHooks {
    virtual ~EventHooks() = default;
    virtual int32_t randomInt(int32_t lower, int32_t upper) = 0;
    virtual float randomFloat(float lower,float upper) = 0;
    virtual void eventCall(uint32_t original_va,int32_t first,int32_t second) = 0;
    // Actual native 0x22141A00 vector construction is not reconstructed.
    // Report the known angle and float arguments, do not invent final XYZ.
    virtual void originalDirectionalCall(float encoded_degrees,float rand_value,
                                         float original_scale) = 0;
};
// Grunt original event 0x92: all five original calls in native stack order;
// latest float time starts from original global engine time.
inline void onEvent92(State& s, float engine_time, EventHooks& h) {
    h.eventCall(0x2214C730U,0x400,0);
    h.eventCall(0x22032220U,0,0);
    h.eventCall(0x220303B0U,4,0);
    h.eventCall(0x22030390U,1,0);
    h.eventCall(0x22143610U,3,0);
    s.raw_0xC20 = engine_time + h.randomFloat(5.0f,10.0f);
}
// Grunt original event 0x8D: authentic random integer and float ranges,
// directional helper, scheduler numeric code 0x19, and new 2..5s deadline.
// Avoid false x87/sin/cos/vector parity claims: native vector stays opaque.
inline void onEvent8D(State& s,float engine_time,EventHooks& h,
                       SignalHooks& signal,bool debugGlobal) {
    const int32_t raw_angle = h.randomInt(-180,180);
    const float encoded = quantizeDegreesHost(static_cast<float>(raw_angle));
    const float shaping = h.randomFloat(0.1f,0.5f);
    h.originalDirectionalCall(encoded,shaping,220.0f);
    numericSignal(s,0x19,debugGlobal,signal);
    s.raw_0xC3C=0;
    s.raw_0xC38=engine_time+h.randomFloat(2.0f,5.0f);
}
}} // namespace sin_compat::original_grunt_event_signals_phase582