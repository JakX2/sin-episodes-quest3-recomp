#include "sin_compat/original_grunt_mode0_phase574.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
using namespace sin_compat::phase574;

struct HandleHost {
    std::uint32_t raw=0xFFFFFFFFu;
    std::optional<bool> reply{};
    int lookups=0;
    std::uint32_t argument=0;
    std::uint32_t nav_handle_0x7c() {return raw;}
    std::optional<bool> resolve_and_virtual_120(std::uint32_t arg) {
        ++lookups;argument=arg;return reply;
    }
};
struct Mode0Host {
    std::vector<std::string> log;
    float raw_metric=4.f;
    bool clearance=false;
    float last_distance=-1.f, last_scaled=-1.f;
    float owner_virtual_5f8(float threshold,Vec3 delta) {
        assert(threshold==-720.f);
        assert(delta.x==3.f && delta.y==4.f && delta.z==90.f);
        log.push_back("owner-virtual");return raw_metric;
    }
    bool first_clearance_2206ef20(float distance,float scaled) {
        log.push_back("first-clearance");last_distance=distance;
        last_scaled=scaled;return clearance;
    }
};
struct AlternateHost {
    float metric=5.f, last=0.f;
    int owner_calls=0,clearance_calls=0;
    float alternate_owner_virtual_5f8(float threshold) {
        assert(threshold==-720.f);++owner_calls;return metric;
    }
    bool alternate_clearance_2206ef20(float scaled){++clearance_calls;last=scaled;return scaled>10.f;}
};
int main() {
    {HandleHost h;auto s=nav_scalar(h);assert(s.scale==1.f && s.path==ScalarPath::InvalidEntityHandle && h.lookups==0);}
    {HandleHost h;h.raw=0xB0004u;auto s=nav_scalar(h);assert(s.scale==1.f && h.lookups==1 && h.argument==h.raw);}
    {HandleHost h;h.raw=0xA0002u;h.reply=false;auto s=nav_scalar(h);assert(s.scale>2.59f && s.scale<2.61f && s.path==ScalarPath::EntityVirtualFalse);}
    {HandleHost h;h.raw=0xC0001u;h.reply=true;auto s=nav_scalar(h);assert(s.scale==10.f && s.path==ScalarPath::EntityVirtualTrue);}
    const Vec3 start{1.f,2.f,5.f}, endpoint{4.f,6.f,95.f};
    {
        Mode0Host h;Report r;r.status=999;
        auto out=mode0_first_stage(h,start,endpoint,2.6f,r);
        assert(out.stage==Mode0Stage::RejectedMinusTwo);
        assert(std::fabs(out.scaled_owner_metric-10.4f)<0.0001f);
        assert(h.last_distance==5.f && std::fabs(h.last_scaled-10.4f)<0.0001f);
        assert(r.xy_distance==5.f && r.secondary_distance==5.f && r.status==-2);
        assert(r.copied_start.x==1.f && r.copied_start.y==2.f && r.copied_start.z==5.f);
        assert((h.log==std::vector<std::string>{"owner-virtual","first-clearance"}));
    }
    {
        Mode0Host h;h.clearance=true;Report r;r.status=-99;
        auto out=mode0_first_stage(h,start,endpoint,10.f,r);
        assert(out.stage==Mode0Stage::RequiresNativeContinuation);
        assert(out.scaled_owner_metric==40.f);
        assert(r.xy_distance==5.f && r.status==0 && r.secondary_distance==0.f);
    }
    {
        Mode0Host h;Report r;
        auto out=mode0_first_stage(h,Vec3{NAN,2.f,3.f},endpoint,1.f,r);
        assert(out.stage==Mode0Stage::HostInputUnsupported);
        assert(h.log.empty());
    }
    {
        AlternateHost h;
        assert(alternate_scaled_clearance(h,2.6f));
        assert(h.owner_calls==1 && h.clearance_calls==1);
        assert(std::fabs(h.last-13.f)<0.00001f);
        h.metric=1.f;
        assert(!alternate_scaled_clearance(h,10.f));
        assert(h.clearance_calls==2 && h.last==10.f);
    }
    return 0;
}