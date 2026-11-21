#pragma once
// Bounded, original-free host model of native server.dll 0x2206EF20.
// The two opaque 0x2206E730 trace calls are supplied by a host, not emulated.
#include <cstdint>
#include <optional>

namespace sin_compat::phase575 {
struct Vec3 { float x=0.f, y=0.f, z=0.f; };
struct EntityHit {
    std::int32_t raw_state_0x204=0;
};
struct Trace {
    // Native stack byte outputs +0x8E and +0x8F from 0x2206E730.
    bool raw_flag_8e=false;
    bool raw_flag_8f=false;
    // AH from native `fnstsw ax` after comparison against 1.0f.
    // The engine is responsible for preserving genuine x87 status if needed.
    std::uint8_t fraction_compare_ah=0;
    Vec3 calculated_point{}; // native final trace-vector candidate
    std::optional<EntityHit> hit{};
};
enum class ReturnPath { TracePointAccepted, SourcePointNearCandidate, SourcePointRejected };
struct Outcome { bool accepted; Vec3 selected; ReturnPath path; bool second_trace_used; };

constexpr bool even_parity8(std::uint8_t n) noexcept {
    // Equivalence of x86 parity flag (PF==1 iff low-byte bit count is even).
    unsigned count=0;
    for(unsigned bit=0;bit<8;++bit) count+=(n>>bit)&1u;
    return (count%2u)==0u;
}
constexpr bool original_fraction_branch(std::uint8_t fnstsw_ah) noexcept {
    // Native `test ah,0x44; jnp fallback`: do NOT assume regular ordered <1.
    return even_parity8(static_cast<std::uint8_t>(fnstsw_ah&0x44u));
}
constexpr bool original_near_candidate(std::uint8_t fnstsw_ah) noexcept {
    // Native `test ah,0x05; jp skip`: only odd parity sets candidate flag.
    return !even_parity8(static_cast<std::uint8_t>(fnstsw_ah&0x05u));
}

template<class Host>
Outcome clearance_select(Host& host, Vec3 source) {
    Trace trace=host.primary_trace_2206e730();
    bool near=false;
    bool second=false;
    if(trace.raw_flag_8f) {
        // Candidate eligibility: entity state +0x204==6 OR virtual +0xF4.
        if(trace.hit && (trace.hit->raw_state_0x204==6 || host.entity_virtual_f4(*trace.hit))) {
            // Host must supply comparison AH for original 3D distance from
            // source to owner +0x360 vs native double 0.1. Floating-point
            // evaluation/exceptions are not approximated as x87 equivalence.
            near=original_near_candidate(host.candidate_distance_comparison_ah(source));
        }
        // The native secondary call uses an original literal +0.0625f in Z.
        trace=host.secondary_trace_2206e730(0.0625f);
        second=true;
    }
    const bool trace_path=original_fraction_branch(trace.fraction_compare_ah)
                         && !trace.raw_flag_8e && (!near || !trace.raw_flag_8f);
    if(trace_path)
        return {true,trace.calculated_point,ReturnPath::TracePointAccepted,second};
    // Original fallback copies the caller's XYZ, not the trace position;
    // it returns the captured near-candidate byte (BL) as its Boolean.
    if(near)
        return {true,source,ReturnPath::SourcePointNearCandidate,second};
    return {false,source,ReturnPath::SourcePointRejected,second};
}
} // namespace sin_compat::phase575