#include "sin_compat/event_bridge_v1.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>

namespace {
using Version = uint32_t (*)();
using Create = sin_event_bridge_v1* (*)(const sin_host_callbacks_v1*, void*);
using Attach = int32_t (*)(sin_event_bridge_v1*, sin_entity_ref_v1,
                           sin_entity_ref_v1, int32_t, const sin_weapon_config_v1*, int32_t);
using Dispatch = int32_t (*)(sin_event_bridge_v1*, int32_t,
                             const sin_bridge_event_input_v1*, sin_bridge_event_output_v1*);
using Active = int32_t (*)(const sin_event_bridge_v1*);
using Destroy = void (*)(sin_event_bridge_v1*);
Dispatch g_dispatch = nullptr;
Destroy g_destroy = nullptr;

struct Host {
    sin_entity_ref_v1 player{8, 3}, weapon{14, 7}, selected{14, 7};
    sin_entity_ref_v1 model{30, 4}, slots[2]{{0, 0}, {0, 0}};
    int player_live = 1, weapon_live = 1, model_live = 0;
    int creates = 0, removes = 0, hides = 0, model_hides = 0, fov = 0, binds = 0;
    sin_event_bridge_v1* bridge = nullptr;
    int nested_trigger = 0, nested_result = 123, nested_destroy_attempts = 0;
};
Host& h(void* p) { return *static_cast<Host*>(p); }
bool same(sin_entity_ref_v1 a, sin_entity_ref_v1 b) {
    return a.index == b.index && a.generation == b.generation;
}
sin_entity_ref_v1 invalid() { return {UINT32_MAX, UINT32_MAX}; }
int live(void* p, sin_entity_ref_v1 r) {
    auto& x = h(p);
    if (x.nested_trigger) {
        x.nested_trigger = 0;
        x.nested_result = g_dispatch(x.bridge, SIN_BRIDGE_EQUIP_V1, nullptr, nullptr);
        g_destroy(x.bridge);  // must be ignored until outer call returns
        ++x.nested_destroy_attempts;
    }
    return (x.player_live && same(r, x.player)) ||
           (x.weapon_live && same(r, x.weapon)) ||
           (x.model_live && same(r, x.model));
}
sin_entity_ref_v1 owner(void* p, sin_entity_ref_v1 r) {
    return same(r, h(p).weapon) ? h(p).player : invalid();
}
sin_entity_ref_v1 selected_weapon(void* p, sin_entity_ref_v1 r) {
    return same(r, h(p).player) ? h(p).selected : invalid();
}
void hide_weapon(void* p, sin_entity_ref_v1 r, int flag) {
    assert(same(r, h(p).weapon) && flag); ++h(p).hides;
}
int player(void* p, sin_entity_ref_v1 r) {
    return same(r, h(p).player) && h(p).player_live;
}
int32_t model_slot(void* p, sin_entity_ref_v1 r) {
    assert(same(r, h(p).weapon)); return 1;
}
int slot_valid(void* p, sin_entity_ref_v1 r, int32_t i) {
    return same(r, h(p).player) && i >= 0 && i < 2;
}
sin_entity_ref_v1 slot_at(void* p, sin_entity_ref_v1 r, int32_t i) {
    return same(r, h(p).player) && i >= 0 && i < 2 ? h(p).slots[i] : invalid();
}
int model_owned(void* p, sin_entity_ref_v1 r, sin_entity_ref_v1 owner_ref) {
    auto& x = h(p);
    return x.model_live && same(r, x.model) && same(owner_ref, x.player);
}
void hide_model(void* p, sin_entity_ref_v1 r, int flag) {
    assert(same(r, h(p).model) && flag); ++h(p).model_hides;
}
int32_t slots(void* p, sin_entity_ref_v1 r) {
    return same(r, h(p).player) ? 2 : 0;
}
sin_entity_ref_v1 create(void* p, const char* name) {
    auto& x = h(p);
    assert(std::strcmp(name, "viewmodel") == 0);
    ++x.creates;
    x.model = {30, static_cast<uint32_t>(3 + x.creates)};
    x.model_live = 1;
    return x.model;
}
int configure(void* p, sin_entity_ref_v1 r, sin_entity_ref_v1 owner_ref, int32_t i) {
    return same(r, h(p).model) && same(owner_ref, h(p).player) && i == 1;
}
int store(void* p, sin_entity_ref_v1 r, int32_t i, sin_entity_ref_v1 model_ref) {
    if (!same(r, h(p).player) || i < 0 || i >= 2) return 0;
    h(p).slots[i] = model_ref;
    return 1;
}
void remove_model(void* p, sin_entity_ref_v1 r) {
    auto& x = h(p); assert(same(r, x.model));
    x.model_live = 0; ++x.removes;
}
void clear(void* p, sin_entity_ref_v1 r, int32_t i) {
    assert(same(r, h(p).player) && i >= 0 && i < 2);
    h(p).slots[i] = invalid();
}
int32_t resolve(void*, int32_t activity) { return activity >= 0 ? activity + 100 : -1; }
void bind(void* p, const char* name, const float origin[3]) {
    assert(std::strcmp(name, "fake_model.mdl") == 0 && origin[0] == 1.f);
    ++h(p).binds;
}
void play(void*, int32_t seq) { assert(seq >= 100); }
void fov(void* p, int32_t, float) { ++h(p).fov; }

sin_host_callbacks_v1 callbacks() {
    sin_host_callbacks_v1 c{};
    c.version = SIN_COMPAT_ABI_VERSION_1; c.struct_size = sizeof(c);
    c.is_live = live; c.owner_of = owner; c.selected_weapon_of = selected_weapon;
    c.set_weapon_nodraw = hide_weapon; c.is_player_owner = player;
    c.viewmodel_index_of = model_slot; c.is_valid_viewmodel_index = slot_valid;
    c.viewmodel_of = slot_at; c.is_viewmodel_owned_by = model_owned;
    c.set_viewmodel_nodraw = hide_model; c.is_player = player;
    c.viewmodel_slot_count = slots; c.viewmodel_at = slot_at;
    c.create_named_entity = create; c.configure_viewmodel = configure;
    c.store_viewmodel_handle = store; c.remove_viewmodel = remove_model;
    c.clear_viewmodel_handle = clear; c.resolve_sequence = resolve;
    c.bind_viewmodel = bind; c.play_sequence = play; c.set_flatscreen_fov = fov;
    return c;
}

template<typename T> T symbol(void* library, const char* name) {
    T value = reinterpret_cast<T>(dlsym(library, name));
    if (!value) std::fprintf(stderr, "Missing symbol: %s\n", name);
    assert(value); return value;
}
} // namespace

int main(int argc, char** argv) {
    assert(argc == 2);
    void* library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!library) {
        std::fprintf(stderr, "dlopen failed: %s\n", dlerror());
        return 1;
    }
    Version version = symbol<Version>(library, "sin_event_bridge_abi_version_v1");
    Create create_bridge = symbol<Create>(library, "sin_event_bridge_create_v1");
    Attach attach = symbol<Attach>(library, "sin_event_bridge_attach_v1");
    g_dispatch = symbol<Dispatch>(library, "sin_event_bridge_dispatch_v1");
    Active active = symbol<Active>(library, "sin_event_bridge_is_active_v1");
    g_destroy = symbol<Destroy>(library, "sin_event_bridge_destroy_v1");
    assert(version() == SIN_EVENT_BRIDGE_ABI_VERSION_1);
    Host fixture;
    sin_host_callbacks_v1 cb = callbacks();
    cb.version = 99;
    assert(create_bridge(&cb, &fixture) == nullptr);
    cb.version = 1;
    cb.struct_size -= 4;
    assert(create_bridge(&cb, &fixture) == nullptr);
    cb.struct_size = sizeof(cb);
    auto* bridge = create_bridge(&cb, &fixture);
    assert(bridge);
    fixture.bridge = bridge;
    assert(active(bridge) == 0);
    assert(g_dispatch(bridge, SIN_BRIDGE_EQUIP_V1, nullptr, nullptr) == -1);
    sin_weapon_config_v1 cfg{"fake_model.mdl", {1.f, 0.f, 0.f}, 55, 6, -1};
    assert(attach(bridge, fixture.player, fixture.weapon, 3, &cfg, 1) == 0);
    assert(attach(bridge, fixture.player, fixture.weapon, 0, &cfg, 1) == 1);
    assert(active(bridge) == 1);
    assert(g_dispatch(bridge, SIN_BRIDGE_EQUIP_V1, nullptr, nullptr) == 6);
    assert(g_dispatch(bridge, SIN_BRIDGE_EQUIP_V1, nullptr, nullptr) == 2);
    assert(fixture.creates == 1);
    fixture.nested_trigger = 1;
    assert(g_dispatch(bridge, SIN_BRIDGE_HIDE_WEAPON_V1, nullptr, nullptr) == 3);
    assert(fixture.nested_result == -1 && fixture.nested_destroy_attempts == 1);
    assert(active(bridge) == 1);
    sin_bridge_event_input_v1 input{};
    input.buttons = 0x00080000u;
    assert(g_dispatch(bridge, SIN_BRIDGE_USERCMD_V1, nullptr, nullptr) == -1);
    assert(g_dispatch(bridge, SIN_BRIDGE_USERCMD_V1, &input, nullptr) & 1);
    assert(fixture.fov == 0);  // VR HMD projection must never be changed
    input.ammo = {2, 0, 5, 0};
    assert(g_dispatch(bridge, SIN_BRIDGE_SET_AMMO_V1, &input, nullptr) == 0);
    input.start_activity = -1; input.end_activity = -1;
    assert(g_dispatch(bridge, SIN_BRIDGE_RELOAD_BEGIN_V1, &input, nullptr) == 1);
    sin_bridge_event_output_v1 output{};
    output.primary_transfer = -123;
    assert(g_dispatch(bridge, SIN_BRIDGE_RELOAD_TRANSFER_V1, nullptr, &output) == 0);
    assert(output.primary_transfer == 4 && output.secondary_transfer == 0);
    assert(g_dispatch(bridge, SIN_BRIDGE_GET_AMMO_V1, nullptr, &output) == 0);
    assert(output.ammo.clip == 6 && output.ammo.reserve == 1);
    assert(g_dispatch(bridge, SIN_BRIDGE_RELOAD_END_V1, nullptr, nullptr) == 0);
    assert(g_dispatch(bridge, 999, nullptr, &output) == -1);
    assert(g_dispatch(bridge, SIN_BRIDGE_MAP_UNLOAD_V1, nullptr, nullptr) == 0);
    assert(fixture.removes == 1 && active(bridge) == 0);
    assert(g_dispatch(bridge, SIN_BRIDGE_MAP_UNLOAD_V1, nullptr, nullptr) == 0);
    assert(fixture.removes == 1);
    ++fixture.weapon.generation;
    fixture.selected = fixture.weapon;
    assert(attach(bridge, fixture.player, fixture.weapon, 0, &cfg, 1) == 1);
    assert(g_dispatch(bridge, SIN_BRIDGE_EQUIP_V1, nullptr, nullptr) == 6);
    ++fixture.weapon.generation;  // recycled engine index
    fixture.selected = fixture.weapon;
    assert(g_dispatch(bridge, SIN_BRIDGE_USERCMD_V1, &input, nullptr) == -1);
    assert(active(bridge) == 0);
    g_destroy(bridge);
    assert(dlclose(library) == 0);
    std::puts("Native event bridge shared-library dlopen lifecycle passed");
    return 0;
}
