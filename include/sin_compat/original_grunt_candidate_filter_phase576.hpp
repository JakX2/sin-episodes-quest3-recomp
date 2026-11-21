#ifndef SIN_COMPAT_ORIGINAL_GRUNT_CANDIDATE_FILTER_PHASE576_HPP_
#define SIN_COMPAT_ORIGINAL_GRUNT_CANDIDATE_FILTER_PHASE576_HPP_

// Independently authored safe model of selected, directly authenticated 2006
// SiN original server.dll control-flow behavior. NOT a native object layout,
// original implementation, ABI-compatible class, physics/trace substitute or
// complete 0x22071040 translation.

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace sin_compat::phase576 {

struct Vec2 { float x = 0.0f, y = 0.0f; };
struct Vec3 { float x = 0.0f, y = 0.0f, z = 0.0f; };

struct Output {
    std::int32_t status = 0;
    Vec3 source{};                // Original report +0x04..+0x0C
    Vec3 default_vector{};        // Original report +0x10..+0x18
    std::uintptr_t entity_key = 0; // Original report +0x1C (not an ABI pointer)
    float planar_length = 0.0f;  // Original report +0x20
    float distance_delta = 0.0f; // Original report +0x24
    float accumulation = 0.0f;   // Original report +0x34
    // No assertions about original +0x28..+0x30, object lifetime or alignment.
};

struct Inputs {
    Vec3 source{};
    Vec3 destination{};
    float budget = 0.0f;          // Original fourth stack argument
    std::uint32_t flags = 0;      // Original fifth stack argument
    Vec3 default_vector{};       // Original mutable engine-global float3
};

struct PlanarResult {
    float length = 0.0f;
    Vec2 direction{};
    bool supported_finite = true;
};

// The original helper 0x22027C30 returns magnitude in x87 ST(0) and mutates
// input XY in place to unit length when magnitude compares >0, else zeroes it.
// This reference only handles *finite* safe host input and does not claim
// original x87 rounding/NaN/exception equivalence.
inline PlanarResult normalize_xy(float dx, float dy) {
    PlanarResult r;
    if (!std::isfinite(dx) || !std::isfinite(dy)) {
        r.supported_finite = false;
        return r;
    }
    const double sum = static_cast<double>(dx)*dx + static_cast<double>(dy)*dy;
    const double length = std::sqrt(sum);
    if (!std::isfinite(length) || length > std::numeric_limits<float>::max()) {
        r.supported_finite = false;
        return r;
    }
    r.length = static_cast<float>(length);
    if (r.length > 0.0f) {
        const double inv = 1.0 / length;
        r.direction = {static_cast<float>(dx*inv), static_cast<float>(dy*inv)};
    }
    return r;
}

// The adjacent original native 0x22139F60 gates the original entity pointer
// through virtual byte offset +0xF4 and returns the SAME pointer or null.
inline std::uintptr_t gated_entity(std::uintptr_t entity, bool virtual_f4) {
    return virtual_f4 ? entity : std::uintptr_t{0};
}

inline std::int32_t selected_entity_status(bool virtual_f4,
                                            bool engine_virtual_48_nonzero) {
    // Original 0x22071822 / 0x2207182B: eligible candidate -> -3.
    // Otherwise 0x22071844..4A: nonzero virtual +0x48 -> -1, zero -> -2.
    if (virtual_f4) return -3;
    return engine_virtual_48_nonzero ? -1 : -2;
}

inline constexpr int candidate_search_upper_bound() { return 16; }

enum class ResultState { ImmediateTrue, NeedsNativeCandidateSearch, UnsupportedInput };

struct PrefixDecision {
    ResultState state = ResultState::UnsupportedInput;
    int selection = 0;       // Original raw ESI 0, 1, or 2
    float working_budget = 0.0f;
    PlanarResult horizontal{};
};

struct IProbeHost {
    virtual ~IProbeHost() = default;
    // Original 0x2206F500 predicate called with distinct callsite context.
    virtual bool original_probe_predicate(bool debug_path) = 0;
    // Original 0x2225C840 debug drawing is an external callback, not modeled.
    virtual void original_debug_marker() = 0;
};

// Prefix-only model of original 0x22071040. The native method subsequently
// traverses 0..15 candidate probes and selects a report via several native
// geometry/trace virtuals; those paths are intentionally NOT executed here.
inline PrefixDecision begin_candidate_filter(const Inputs& in,
                                             Output& out,
                                             IProbeHost& host) {
    PrefixDecision result;
    out.status = 0;
    out.source = in.source;
    out.default_vector = in.default_vector;
    out.entity_key = 0;
    out.distance_delta = 0.0f;
    out.accumulation = 0.0f;
    out.planar_length = 0.0f;

    if (!std::isfinite(in.source.x) || !std::isfinite(in.source.y) ||
        !std::isfinite(in.destination.x) || !std::isfinite(in.destination.y) ||
        !std::isfinite(in.budget)) return result;

    const auto dx = static_cast<float>(in.destination.x-in.source.x);
    const auto dy = static_cast<float>(in.destination.y-in.source.y);
    result.horizontal = normalize_xy(dx,dy);
    if (!result.horizontal.supported_finite) return result;
    out.planar_length = result.horizontal.length;
    if (out.planar_length == 0.0f) {
        result.state = ResultState::ImmediateTrue;
        return result;
    }

    result.state = ResultState::NeedsNativeCandidateSearch;
    if ((in.flags & 1u) != 0u || static_cast<double>(in.budget) < 0.001) {
        result.selection = 0;
        result.working_budget = 0.0f;
    } else {
        result.working_budget = static_cast<double>(in.budget) > 99.999
            ? 100.0f : in.budget;
        if ((in.flags & 2u) != 0u) {
            result.selection = 1;
        } else {
            result.selection = host.original_probe_predicate(false) ? 1 : 2;
        }
    }
    if ((in.flags & 8u) != 0u && !host.original_probe_predicate(true)) {
        host.original_debug_marker();
    }
    return result;
}

// Source-bounded terminal *candidate selected* branch from native 0x22071808
// onwards. Other branches, including no-candidate route and vertical fallback,
// have different native behavior and are not claimed covered by this helper.
inline bool complete_selected_candidate(Output& report,
                                        std::uintptr_t entity,
                                        bool virtual_f4,
                                        bool engine_virtual_48_nonzero,
                                        float consumed_distance) {
    report.entity_key = entity;
    report.status = selected_entity_status(virtual_f4, engine_virtual_48_nonzero);
    report.distance_delta = report.planar_length - consumed_distance;
    return false;  // Actual native candidate-selected return at 0x220718B7.
}

// The **original caller** 0x22071A20 ignores 0x22071040's AL return.
// From 0x22071B7D onward it tests an independent candidate entity, possibly
// resolves a related entity, and compares the result with report+0x1C.
// This is a source-bounded *caller* decision, not additional new-method bytes.
struct ICallerAssociationHost {
    virtual ~ICallerAssociationHost() = default;
    virtual bool virtual_120(std::uintptr_t candidate) = 0;
    virtual bool native_22035100(std::uintptr_t candidate) = 0;
    virtual std::uintptr_t native_22035090_virtual_28(std::uintptr_t candidate) = 0;
};

inline bool original_caller_clear_matching_association(
    Output& report, std::uintptr_t candidate,
    const Vec3& runtime_global_default, ICallerAssociationHost& host) {
    if (!candidate) return false;  // Original 0x22071B85 branch.
    bool linked_match = false;
    // Observe native side effects/call ordering *before* testing direct
    // equality: 0x22071B8D -> 0x22071B99 -> 0x22071BA4/+0x28.
    if (host.virtual_120(candidate) && host.native_22035100(candidate)) {
        linked_match = host.native_22035090_virtual_28(candidate)
            == report.entity_key;
    }
    if (candidate != report.entity_key && !linked_match) return false;
    report.distance_delta = 0.0f;  // Original report +0x24.
    report.entity_key = 0;         // Original report +0x1C.
    report.default_vector = runtime_global_default; // +0x10..+0x18.
    report.status = 0;             // Original report +0x00.
    return true;
}

}  // namespace sin_compat::phase576
#endif