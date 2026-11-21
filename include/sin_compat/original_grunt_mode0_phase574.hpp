#pragma once
// Source-bounded, original-free C++17 reference for 2006 server.dll mode-0
// admission. This is NOT an engine-facing ABI or bit-exact x87 replacement.
#include <cmath>
#include <cstdint>
#include <optional>

namespace sin_compat::phase574 {

struct Vec3 { float x=0.f, y=0.f, z=0.f; };
enum class ScalarPath { InvalidEntityHandle, EntityVirtualFalse, EntityVirtualTrue };
struct ScalarChoice { float scale=1.f; ScalarPath path=ScalarPath::InvalidEntityHandle; };

// Native 0x22076910 reads nav+0x7C, resolves a 12-bit index and serial,
// then queries resolved entity vtable+0x120. It actually resolves twice;
// this host API assumes a stable registry snapshot. Missing/racy snapshots
// are not asserted to have identical 2006 crash behavior.
template<class HandleHost>
ScalarChoice nav_scalar(HandleHost& host) {
    const std::uint32_t encoded=host.nav_handle_0x7c();
    if(encoded==0xFFFFFFFFu) return {1.f,ScalarPath::InvalidEntityHandle};
    const std::optional<bool> entity_reply=host.resolve_and_virtual_120(encoded);
    if(!entity_reply) return {1.f,ScalarPath::InvalidEntityHandle};
    return *entity_reply ? ScalarChoice{10.f,ScalarPath::EntityVirtualTrue}
                         : ScalarChoice{2.6f,ScalarPath::EntityVirtualFalse};
}

struct Report {
    std::int32_t status=0; // native output +0x00
    Vec3 copied_start{};   // on first-stage reject: native output +0x04..+0x0c
    float xy_distance=0.f; // native output +0x20
    float secondary_distance=0.f; // original reject copies +0x20 to +0x24
};

enum class Mode0Stage { RejectedMinusTwo, RequiresNativeContinuation, HostInputUnsupported };
struct Mode0Outcome {
    Mode0Stage stage;
    float scaled_owner_metric;
};

// Native 0x22071A20 has two primary stages. Its native followup also calls
// 0x2206F940 (unless flags&4), 0x22071040, and entity virtual checks.
// Native-mode0-success is NEVER claimed solely because early clearance passed.
// The owner virtual +0x5F8 receives float -720 and a vector-like argument.
template<class Mode0Host>
Mode0Outcome mode0_first_stage(Mode0Host& host, const Vec3& start,
                              const Vec3& endpoint, float nav_factor,
                              Report& report) {
    report=Report{};
    if(!std::isfinite(start.x)||!std::isfinite(start.y)||
       !std::isfinite(endpoint.x)||!std::isfinite(endpoint.y)||
       !std::isfinite(nav_factor))
        return {Mode0Stage::HostInputUnsupported,0.f};
    const float dx=endpoint.x-start.x;
    const float dy=endpoint.y-start.y;
    const float distance=std::sqrt(dx*dx+dy*dy);  // native x87 fsqrt indirection
    if(!std::isfinite(distance))
        return {Mode0Stage::HostInputUnsupported,0.f};
    report.xy_distance=distance;
    // Native's original owner virtual returns x87 float; this safe host
    // model delegates it instead of inventing its engine semantics.
    const float raw_metric=host.owner_virtual_5f8(-720.f,Vec3{dx,dy,endpoint.z-start.z});
    if(!std::isfinite(raw_metric))
        return {Mode0Stage::HostInputUnsupported,0.f};
    const float scaled=raw_metric*nav_factor;
    if(!std::isfinite(scaled))
        return {Mode0Stage::HostInputUnsupported,0.f};
    if(!host.first_clearance_2206ef20(distance,scaled)) {
        report.status=-2;
        report.secondary_distance=report.xy_distance;
        report.copied_start=start;
        return {Mode0Stage::RejectedMinusTwo,scaled};
    }
    return {Mode0Stage::RequiresNativeContinuation,scaled};
}

// Native 0x2206F940 is an alternate callsite of the SAME scaled-clearance
// method 0x2206EF20. Caller supplies additional opaque engine arguments.
template<class AlternateHost>
bool alternate_scaled_clearance(AlternateHost& host, float nav_factor) {
    const float owner_metric=host.alternate_owner_virtual_5f8(-720.f);
    if(!std::isfinite(nav_factor)||!std::isfinite(owner_metric)) return false;
    const float scaled=owner_metric*nav_factor;
    return std::isfinite(scaled) && host.alternate_clearance_2206ef20(scaled);
}
}  // namespace sin_compat::phase574