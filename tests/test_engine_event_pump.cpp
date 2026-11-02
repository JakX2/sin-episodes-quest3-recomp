#ifdef NDEBUG
#undef NDEBUG
#endif
#include "sin_compat/engine_event_pump.hpp"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>

using sin_compat::EngineEventPump;
namespace {
struct Host {
    sin_entity_ref_v1 player{8, 3}, weapon{14, 7}, selected{14, 7};
    sin_entity_ref_v1 model{30, 4};
    sin_entity_ref_v1 slots[2]{{0, 0}, {0, 0}};
    bool player_live = true, weapon_live = true, model_live = false;
    int creates = 0, removes = 0, hide_weapon = 0, hide_model = 0;
    int binds = 0, plays = 0, fov_calls = 0;
};

bool same(sin_entity_ref_v1 a, sin_entity_ref_v1 b) {
    return a.index == b.index && a.generation == b.generation;
}
sin_entity_ref_v1 invalid() { return {UINT32_MAX, UINT32_MAX}; }
Host& host(void* p) { return *static_cast<Host*>(p); }

int is_live(void* p, sin_entity_ref_v1 ref) {
    auto& h = host(p);
    return (h.player_live && same(ref, h.player)) ||
           (h.weapon_live && same(ref, h.weapon)) ||
           (h.model_live && same(ref, h.model));
}
sin_entity_ref_v1 owner_of(void* p, sin_entity_ref_v1 ref) {
    auto& h = host(p);
    return same(ref, h.weapon) ? h.player : invalid();
}
sin_entity_ref_v1 selected(void* p, sin_entity_ref_v1 ref) {
    auto& h = host(p);
    return same(ref, h.player) ? h.selected : invalid();
}
void hide_weapon(void* p, sin_entity_ref_v1 ref, int hidden) {
    auto& h = host(p);
    assert(same(ref, h.weapon) && hidden);
    ++h.hide_weapon;
}
int is_player(void* p, sin_entity_ref_v1 ref) {
    return same(ref, host(p).player) && host(p).player_live;
}
int32_t viewmodel_index(void* p, sin_entity_ref_v1 ref) {
    assert(same(ref, host(p).weapon));
    return 1;
}
int valid_slot(void* p, sin_entity_ref_v1 ref, int32_t index) {
    return same(ref, host(p).player) && index >= 0 && index < 2;
}
sin_entity_ref_v1 viewmodel_of(void* p, sin_entity_ref_v1 ref, int32_t index) {
    auto& h = host(p);
    return same(ref, h.player) && index >= 0 && index < 2
        ? h.slots[index] : invalid();
}
int model_owned(void* p, sin_entity_ref_v1 model, sin_entity_ref_v1 player) {
    auto& h = host(p);
    return same(model, h.model) && same(player, h.player) && h.model_live;
}
void hide_model(void* p, sin_entity_ref_v1 model, int hidden) {
    auto& h = host(p);
    assert(same(model, h.model) && hidden);
    ++h.hide_model;
}
int32_t slot_count(void* p, sin_entity_ref_v1 ref) {
    return same(ref, host(p).player) ? 2 : 0;
}
sin_entity_ref_v1 create(void* p, const char* name) {
    auto& h = host(p);
    assert(std::strcmp(name, "viewmodel") == 0);
    ++h.creates;
    h.model = {30, static_cast<uint32_t>(3 + h.creates)};
    h.model_live = true;
    return h.model;
}
int configure(void* p, sin_entity_ref_v1 model, sin_entity_ref_v1 player, int32_t index) {
    auto& h = host(p);
    return same(model, h.model) && same(player, h.player) && index == 1;
}
int store(void* p, sin_entity_ref_v1 player, int32_t index, sin_entity_ref_v1 model) {
    auto& h = host(p);
    if (!same(player, h.player) || index < 0 || index > 1) return 0;
    h.slots[index] = model;
    return 1;
}
void remove_model(void* p, sin_entity_ref_v1 model) {
    auto& h = host(p);
    assert(same(model, h.model));
    h.model_live = false;
    ++h.removes;
}
void clear_slot(void* p, sin_entity_ref_v1 player, int32_t index) {
    auto& h = host(p);
    assert(same(player, h.player) && index >= 0 && index < 2);
    h.slots[index] = invalid();
}
int32_t resolve_sequence(void*, int32_t activity) {
    return activity >= 0 ? activity + 100 : -1;
}
void bind(void* p, const char* name, const float offset[3]) {
    auto& h = host(p);
    assert(std::strcmp(name, "fake_model.mdl") == 0);
    assert(offset[0] == 1.0f);
    ++h.binds;
}
void play(void* p, int32_t sequence) {
    assert(sequence >= 100);
    ++host(p).plays;
}
void fov(void* p, int32_t, float) { ++host(p).fov_calls; }

sin_host_callbacks_v1 callbacks() {
    sin_host_callbacks_v1 cb{};
    cb.version = SIN_COMPAT_ABI_VERSION_1;
    cb.struct_size = sizeof(cb);
    cb.is_live = is_live;
    cb.owner_of = owner_of;
    cb.selected_weapon_of = selected;
    cb.set_weapon_nodraw = hide_weapon;
    cb.is_player_owner = is_player;
    cb.viewmodel_index_of = viewmodel_index;
    cb.is_valid_viewmodel_index = valid_slot;
    cb.viewmodel_of = viewmodel_of;
    cb.is_viewmodel_owned_by = model_owned;
    cb.set_viewmodel_nodraw = hide_model;
    cb.is_player = is_player;
    cb.viewmodel_slot_count = slot_count;
    cb.viewmodel_at = viewmodel_of;
    cb.create_named_entity = create;
    cb.configure_viewmodel = configure;
    cb.store_viewmodel_handle = store;
    cb.remove_viewmodel = remove_model;
    cb.clear_viewmodel_handle = clear_slot;
    cb.resolve_sequence = resolve_sequence;
    cb.bind_viewmodel = bind;
    cb.play_sequence = play;
    cb.set_flatscreen_fov = fov;
    return cb;
}
sin_weapon_config_v1 config() {
    return {"fake_model.mdl", {1.0f, 0.0f, 0.0f}, 55, 6, -1};
}
void test_full_vr_lifecycle() {
    Host h;
    auto cb = callbacks();
    EngineEventPump pump(cb, &h);
    assert(!pump.active());
    assert(pump.on_equip() == -1);
    assert(pump.attach(h.player, h.weapon, 0, config(), true));
    assert(pump.on_equip() == 6);  // created
    assert(pump.on_equip() == 2);  // existing
    assert(h.creates == 1);
    assert(pump.on_hide_weapon() == 3);
    assert(pump.on_usercmd(0x00080000u, false) & 1);
    assert(h.fov_calls == 0); // never alter Quest headset projection
    assert(pump.on_set_ammo({2, 0, 5, 0}) == 0);
    assert(pump.on_reload_begin(-1, -1) == 1);
    assert(pump.on_set_ammo({0, 0, 99, 0}) == -1);
    int32_t primary = -1, secondary = -1;
    assert(pump.on_reload_event(primary, secondary) == 0);
    assert(primary == 4 && secondary == 0);
    assert(pump.on_reload_event(primary, secondary) == 0);
    assert(primary == 0 && secondary == 0);
    sin_ammo_v1 ammo{};
    assert(pump.on_get_ammo(ammo) == 0);
    assert(ammo.clip == 6 && ammo.reserve == 1);
    assert(pump.on_reload_end() == 0);
    pump.on_map_unload();
    assert(!pump.active());
    assert(h.removes == 1);
    pump.on_map_unload();  // duplicate unload must not double-delete
    assert(h.removes == 1);
    assert(pump.on_usercmd(0, false) == -1);
}
void test_generation_reuse_cannot_retarget_session() {
    Host h;
    auto cb = callbacks();
    EngineEventPump pump(cb, &h);
    assert(pump.attach(h.player, h.weapon, 1, config(), true));
    assert(pump.on_equip() == 6);
    // Same engine entity slot, a new generation: old session is invalid.
    ++h.weapon.generation;
    h.selected = h.weapon;
    assert(pump.on_usercmd(0x00080000u, false) == -1);
    assert(!pump.active());
    assert(pump.on_reload_begin(-1, -1) == -1);
    assert(pump.attach(h.player, h.weapon, 1, config(), true));
    assert(pump.on_equip() == 6);
    pump.on_weapon_removed();
    assert(!pump.active());
    assert(pump.on_holster() == -1);
}
void test_invalid_host_and_failed_attach() {
    Host h;
    auto cb = callbacks();
    cb.version = 5;
    EngineEventPump mismatch(cb, &h);
    assert(!mismatch.attach(h.player, h.weapon, 0, config(), true));
    assert(!mismatch.active());
    cb = callbacks();
    EngineEventPump pump(cb, &h);
    assert(!pump.attach(h.player, h.weapon, 5, config(), true));
    h.player_live = false;
    assert(!pump.attach(h.player, h.weapon, 0, config(), true));
    h.player_live = true;
    assert(pump.attach(h.player, h.weapon, 2, config(), true));
    pump.detach();
    assert(!pump.active());
}
} // namespace

int main() {
    test_full_vr_lifecycle();
    test_generation_reuse_cannot_retarget_session();
    test_invalid_host_and_failed_attach();
    std::cout << "Engine-neutral event pump lifecycle tests passed\n";
}
