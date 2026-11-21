#include "sin_compat/original_grunt_nav_phase567.hpp"
#include <cassert>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>
using namespace sin_compat::phase567;
namespace {
struct Hooks: IComponentHooks {
    std::vector<std::uint32_t> seen{};
    std::uint32_t fail_offset=0;
    NavComponent* nav=nullptr;
    bool create_success=true;
    bool acquire(std::uint32_t slot) override {
        seen.push_back(slot);
        return slot!=fail_offset;
    }
    NavComponent* create_nav() override {
        seen.push_back(0x40C);
        return create_success?nav:nullptr;
    }
};
}
int main() {
    static_assert(sizeof(std::uint32_t)==4);
    std::array<std::uint32_t,12> src{};
    for(std::uint32_t i=0;i<12;i++)src[i]=0x11111111u+i;
    const auto w=prepare_waypoint(src);
    for(std::uint32_t i=0;i<12;i++)
        assert(w.copied_words[i]==(i==5?0xBF800000u:((i==10||i==11)?0u:src[i])));
    // Node constructor cannot be assumed to append itself to a route.
    RouteState route;
    assert(route.first==nullptr && route.field_0x44==0);
    assert(route.field_0x10==-1 && route.field_0x24==-1);
    assert(route.field_0x28==-1 && route.field_0x2c==-1);
    assert(route.raw_0x20==0.1f && route.raw_0x4c==std::numeric_limits<float>::max());
    auto nav=construct_grunt_nav(&route);
    assert(nav.route==&route && !nav.flag_byte_0x38);
    assert(nav.distance_lower_raw_0x64==-1.0f);
    assert(nav.distance_scale_raw_0x68==0.25f);
    assert(nav.distance_scale_raw_0x70==3.0f);
    assert(nav.distance_scale_raw_0x78==5.0f);
    assert(!has_waypoint_route(&nav));
    assert(!route_flag_nonzero(&nav));
    route.field_0x44=7;
    assert(route_flag_nonzero(&nav));
    assert(!has_waypoint_route(&nav));
    route.first=&w;
    assert(has_waypoint_route(&nav));
    route.field_0x44=0;
    assert(!route_flag_nonzero(&nav) && has_waypoint_route(&nav));
    assert(!has_waypoint_route(nullptr) && !route_flag_nonzero(nullptr));
    NavComponent bad;
    assert(!has_waypoint_route(&bad) && !route_flag_nonzero(&bad));
    const std::vector<std::uint32_t> full{0x3FC,0x404,0x408,0x400,0x40C,0x410,0x414};
    NpcState npc;
    Hooks hooks;
    hooks.nav=&nav;
    assert(initialize_npc_components(npc,hooks));
    assert(npc.nav_0x8f8==&nav && hooks.seen==full);
    // The real original initializer short-circuits and does not perform
    // speculative rollback of earlier members when a later slot fails.
    for(std::uint32_t fail:{0x3FCu,0x404u,0x408u,0x400u,0x410u,0x414u}) {
        Hooks f;f.nav=&nav;f.fail_offset=fail;
        NpcState n;
        assert(!initialize_npc_components(n,f));
        const auto at=std::find(full.begin(),full.end(),fail);
        assert(at!=full.end());
        assert(f.seen==std::vector<std::uint32_t>(full.begin(),at+1));
        assert(n.nav_0x8f8==(fail>=0x410?&nav:nullptr));
    }
    Hooks failed_alloc; failed_alloc.create_success=false;
    NpcState missing;
    assert(!initialize_npc_components(missing,failed_alloc));
    assert(missing.nav_0x8f8==nullptr);
    assert(failed_alloc.seen==std::vector<std::uint32_t>(full.begin(),full.begin()+5));
    return 0;
}