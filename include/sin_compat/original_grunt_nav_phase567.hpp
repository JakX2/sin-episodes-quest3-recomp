#pragma once
// Independently written C++17 behavior reference for authenticated original
// navigation setup. NOT a 2006 MSVC ABI layout or executable DLL substitute.
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>

namespace sin_compat::phase567 {
struct WaypointNode {
    // Host-safe representation of native 0x30-byte record initialization,
    // including original raw x87 +0x14 invalid-cache sentinel and links.
    std::array<std::uint32_t,12> copied_words{};
};
inline WaypointNode prepare_waypoint(const std::array<std::uint32_t,12>& src) {
    WaypointNode node{src};
    node.copied_words[5]=0xBF800000u; // raw float at +0x14, -1.0f
    node.copied_words[10]=0; // native forward +0x28 pointer
    node.copied_words[11]=0; // native back +0x2C pointer
    return node;
}
struct RouteState {
    // These are logical host fields, NOT portable binary in-memory offsets.
    const WaypointNode* first=nullptr;
    std::int32_t field_0x10=-1;
    std::int32_t field_0x24=-1;
    std::int32_t field_0x28=-1;
    std::int32_t field_0x2c=-1;
    float raw_0x20=0.1f;
    float raw_0x4c=std::numeric_limits<float>::max();
    std::uint32_t field_0x44=0;
};
struct NavComponent {
    // Original 0x22031F50 makes 0x9C-byte allocation. Its native ctor
    // 0x220747A0 separately allocates a 0x6C-byte route at raw +0x24.
    RouteState* route=nullptr;
    bool flag_byte_0x38=false;
    float distance_lower_raw_0x64=-1.0f;
    float distance_lower_raw_0x6c=-1.0f;
    float distance_lower_raw_0x74=-1.0f;
    float distance_scale_raw_0x68=0.25f;
    float distance_scale_raw_0x70=3.0f;
    float distance_scale_raw_0x78=5.0f;
};
inline NavComponent construct_grunt_nav(RouteState* backing) {
    NavComponent n;
    n.route=backing;
    n.flag_byte_0x38=false;
    return n;
}
inline bool has_waypoint_route(const NavComponent* nav) {
    return nav && nav->route && nav->route->first;
}
inline bool route_flag_nonzero(const NavComponent* nav) {
    return nav && nav->route && nav->route->field_0x44!=0;
}
struct NpcState { NavComponent* nav_0x8f8=nullptr; };
// Native 0x22031D50 stops at first failed virtual slot. Only on the
// fifth (+0x40C) success does it store NPC +0x8F8. Remaining callbacks
// still run before final success. Acquired unrelated engine interfaces are
// intentionally omitted from this model.
struct IComponentHooks {
    virtual ~IComponentHooks()=default;
    virtual bool acquire(std::uint32_t vtable_byte_offset)=0;
    virtual NavComponent* create_nav()=0;
};
inline bool initialize_npc_components(NpcState& npc,IComponentHooks& hooks) {
    for(const auto offset: {0x3FCu,0x404u,0x408u,0x400u})
        if(!hooks.acquire(offset))return false;
    npc.nav_0x8f8=hooks.create_nav();
    if(!npc.nav_0x8f8)return false;
    for(const auto offset: {0x410u,0x414u})
        if(!hooks.acquire(offset))return false;
    return true;
}
} // namespace sin_compat::phase567