#include <jni.h>
#include "sin_compat/event_bridge_v1.h"
#include <cstdint>
#include <cstring>
#include <sstream>
#include <string>

namespace {

struct FakeHost {
    sin_entity_ref_v1 player{8, 3};
    sin_entity_ref_v1 weapon{14, 7};
    sin_entity_ref_v1 selected{14, 7};
    sin_entity_ref_v1 model{30, 4};
    sin_entity_ref_v1 slots[2]{{UINT32_MAX, UINT32_MAX},
                               {UINT32_MAX, UINT32_MAX}};
    bool player_live = true;
    bool weapon_live = true;
    bool model_live = false;
    int creates = 0;
    int removes = 0;
    int weapon_hides = 0;
    int model_hides = 0;
    int fov_changes = 0;
    int model_binds = 0;
    int sequence_plays = 0;
};

bool same(sin_entity_ref_v1 a, sin_entity_ref_v1 b) {
    return a.index == b.index && a.generation == b.generation;
}

sin_entity_ref_v1 invalid_ref() { return {UINT32_MAX, UINT32_MAX}; }
FakeHost &get(void *ctx) { return *static_cast<FakeHost *>(ctx); }

int is_live(void *ctx, sin_entity_ref_v1 entity) {
    auto &h = get(ctx);
    return (h.player_live && same(entity, h.player)) ||
           (h.weapon_live && same(entity, h.weapon)) ||
           (h.model_live && same(entity, h.model));
}
sin_entity_ref_v1 owner_of(void *ctx, sin_entity_ref_v1 weapon) {
    auto &h = get(ctx);
    return same(weapon, h.weapon) ? h.player : invalid_ref();
}
sin_entity_ref_v1 selected_weapon_of(void *ctx, sin_entity_ref_v1 player) {
    auto &h = get(ctx);
    return same(player, h.player) ? h.selected : invalid_ref();
}
void set_weapon_nodraw(void *ctx, sin_entity_ref_v1 weapon, int hide) {
    auto &h = get(ctx);
    if (same(weapon, h.weapon) && hide) ++h.weapon_hides;
}
int is_player(void *ctx, sin_entity_ref_v1 player) {
    auto &h = get(ctx);
    return h.player_live && same(player, h.player);
}
int32_t viewmodel_index_of(void *, sin_entity_ref_v1) { return 1; }
int is_valid_viewmodel_index(void *ctx, sin_entity_ref_v1 player, int32_t index) {
    return same(player, get(ctx).player) && index >= 0 && index < 2;
}
sin_entity_ref_v1 viewmodel_of(void *ctx, sin_entity_ref_v1 player, int32_t index) {
    auto &h = get(ctx);
    return same(player, h.player) && index >= 0 && index < 2
        ? h.slots[index] : invalid_ref();
}
int is_viewmodel_owned_by(void *ctx, sin_entity_ref_v1 model, sin_entity_ref_v1 player) {
    auto &h = get(ctx);
    return h.model_live && same(model, h.model) && same(player, h.player);
}
void set_viewmodel_nodraw(void *ctx, sin_entity_ref_v1 model, int hide) {
    auto &h = get(ctx);
    if (same(model, h.model) && hide) ++h.model_hides;
}
int32_t viewmodel_slot_count(void *ctx, sin_entity_ref_v1 player) {
    return same(player, get(ctx).player) ? 2 : 0;
}
sin_entity_ref_v1 viewmodel_at(void *ctx, sin_entity_ref_v1 player, int32_t index) {
    return viewmodel_of(ctx, player, index);
}
sin_entity_ref_v1 create_named_entity(void *ctx, const char *name) {
    auto &h = get(ctx);
    if (std::strcmp(name, "viewmodel") != 0) return invalid_ref();
    h.model = {30, static_cast<uint32_t>(4 + h.creates)};
    h.model_live = true;
    ++h.creates;
    return h.model;
}
int configure_viewmodel(void *ctx, sin_entity_ref_v1 model,
                        sin_entity_ref_v1 player, int32_t index) {
    auto &h = get(ctx);
    return same(model, h.model) && same(player, h.player) && index == 1;
}
int store_viewmodel_handle(void *ctx, sin_entity_ref_v1 player, int32_t index,
                           sin_entity_ref_v1 model) {
    auto &h = get(ctx);
    if (!same(player, h.player) || index < 0 || index > 1) return 0;
    h.slots[index] = model;
    return 1;
}
void remove_viewmodel(void *ctx, sin_entity_ref_v1 model) {
    auto &h = get(ctx);
    if (same(model, h.model)) {
        h.model_live = false;
        ++h.removes;
    }
}
void clear_viewmodel_handle(void *ctx, sin_entity_ref_v1 player, int32_t index) {
    auto &h = get(ctx);
    if (same(player, h.player) && index >= 0 && index < 2)
        h.slots[index] = invalid_ref();
}
int32_t resolve_sequence(void *, int32_t activity) {
    return activity >= 0 ? activity + 100 : -1;
}
void bind_viewmodel(void *ctx, const char *name, const float origin[3]) {
    auto &h = get(ctx);
    if (std::strcmp(name, "fake_model.mdl") == 0 && origin[0] == 1.0f)
        ++h.model_binds;
}
void play_sequence(void *ctx, int32_t sequence) {
    if (sequence >= 100) ++get(ctx).sequence_plays;
}
void set_flatscreen_fov(void *ctx, int32_t, float) {
    ++get(ctx).fov_changes;
}

sin_host_callbacks_v1 fake_callbacks() {
    sin_host_callbacks_v1 cb{};
    cb.version = SIN_COMPAT_ABI_VERSION_1;
    cb.struct_size = sizeof(cb);
    cb.is_live = is_live;
    cb.owner_of = owner_of;
    cb.selected_weapon_of = selected_weapon_of;
    cb.set_weapon_nodraw = set_weapon_nodraw;
    cb.is_player_owner = is_player;
    cb.viewmodel_index_of = viewmodel_index_of;
    cb.is_valid_viewmodel_index = is_valid_viewmodel_index;
    cb.viewmodel_of = viewmodel_of;
    cb.is_viewmodel_owned_by = is_viewmodel_owned_by;
    cb.set_viewmodel_nodraw = set_viewmodel_nodraw;
    cb.is_player = is_player;
    cb.viewmodel_slot_count = viewmodel_slot_count;
    cb.viewmodel_at = viewmodel_at;
    cb.create_named_entity = create_named_entity;
    cb.configure_viewmodel = configure_viewmodel;
    cb.store_viewmodel_handle = store_viewmodel_handle;
    cb.remove_viewmodel = remove_viewmodel;
    cb.clear_viewmodel_handle = clear_viewmodel_handle;
    cb.resolve_sequence = resolve_sequence;
    cb.bind_viewmodel = bind_viewmodel;
    cb.play_sequence = play_sequence;
    cb.set_flatscreen_fov = set_flatscreen_fov;
    return cb;
}

std::string self_test() {
    std::ostringstream out;
    int failures = 0;
    auto check = [&](bool success, const char *label) {
        out << (success ? "[PASS] " : "[FAIL] ") << label << "\n";
        if (!success) ++failures;
    };

    check(sin_abi_version_v1() == 1, "compatibility ABI v1 loaded");
    check(sin_event_bridge_abi_version_v1() == 1, "event bridge ABI v1 loaded");

    FakeHost host;
    const auto callbacks = fake_callbacks();
    sin_event_bridge_v1 *bridge = sin_event_bridge_create_v1(&callbacks, &host);
    check(bridge != nullptr, "create native event bridge");
    if (!bridge) {
        out << "\nRESULT: FAIL (bridge creation)\n";
        return out.str();
    }

    const sin_weapon_config_v1 config{
        "fake_model.mdl", {1.f, 0.f, 0.f}, 50, 6, -1
    };  // Deliberately synthetic ammo/model fixture, not a SiN asset.
    check(sin_event_bridge_attach_v1(bridge, host.player, host.weapon,
                                      0, &config, 1) == 1,
          "attach synthetic player and weapon in VR mode");
    check(sin_event_bridge_dispatch_v1(
              bridge, SIN_BRIDGE_EQUIP_V1, nullptr, nullptr) == 6,
          "create one synthetic viewmodel");
    check(sin_event_bridge_dispatch_v1(
              bridge, SIN_BRIDGE_EQUIP_V1, nullptr, nullptr) == 2 &&
              host.creates == 1,
          "reuse existing viewmodel without duplicate creation");
    sin_bridge_event_input_v1 input{};
    input.buttons = 0x00080000u;
    const int32_t zoom = sin_event_bridge_dispatch_v1(
        bridge, SIN_BRIDGE_USERCMD_V1, &input, nullptr);
    check((zoom & 1) != 0 && host.fov_changes == 0,
          "VR aiming leaves headset FOV untouched");

    input.ammo = {2, 0, 5, 0};
    check(sin_event_bridge_dispatch_v1(
              bridge, SIN_BRIDGE_SET_AMMO_V1, &input, nullptr) == 0,
          "supply initial synthetic ammo");
    input.start_activity = -1;
    input.end_activity = -1;
    check(sin_event_bridge_dispatch_v1(
              bridge, SIN_BRIDGE_RELOAD_BEGIN_V1, &input, nullptr) == 1,
          "begin synthetic reload");
    sin_bridge_event_output_v1 output{};
    const int32_t transfer = sin_event_bridge_dispatch_v1(
        bridge, SIN_BRIDGE_RELOAD_TRANSFER_V1, nullptr, &output);
    check(transfer == 0 && output.primary_transfer == 4 &&
              output.secondary_transfer == 0,
          "transfer four units of ammo on reload event");
    check(sin_event_bridge_dispatch_v1(
              bridge, SIN_BRIDGE_GET_AMMO_V1, nullptr, &output) == 0 &&
              output.ammo.clip == 6 && output.ammo.reserve == 1,
          "read updated clip and reserve ammo");
    check(sin_event_bridge_dispatch_v1(
              bridge, SIN_BRIDGE_RELOAD_END_V1, nullptr, nullptr) == 0,
          "finish synthetic reload");

    check(sin_event_bridge_dispatch_v1(
              bridge, SIN_BRIDGE_MAP_UNLOAD_V1, nullptr, nullptr) == 0 &&
              sin_event_bridge_is_active_v1(bridge) == 0 &&
              host.removes == 1,
          "detach and remove model on map unload");
    check(sin_event_bridge_dispatch_v1(
              bridge, SIN_BRIDGE_MAP_UNLOAD_V1, nullptr, nullptr) == 0 &&
              host.removes == 1,
          "repeated map unload is harmless");

    check(sin_event_bridge_attach_v1(bridge, host.player, host.weapon,
                                      0, &config, 1) == 1,
          "reattach after map transition");
    ++host.weapon.generation;  // Same index, different host generation.
    host.selected = host.weapon;
    check(sin_event_bridge_dispatch_v1(
              bridge, SIN_BRIDGE_USERCMD_V1, &input, nullptr) == -1 &&
              sin_event_bridge_is_active_v1(bridge) == 0,
          "reject stale weapon generation");

    sin_event_bridge_destroy_v1(bridge);
    out << "\nRESULT: " << (failures ? "FAIL" : "PASS") << " ("
        << failures << " failed checks)\n";
    out << "Scope: native synthetic host only. Not SourceVR or SiN gameplay.\n";
    return out.str();
}

} // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_org_sincompat_screenprobe_MainActivity_nativeRunSelfTest(
    JNIEnv *env, jobject) {
    try {
        const std::string result = self_test();
        return env->NewStringUTF(result.c_str());
    } catch (...) {
        return env->NewStringUTF("RESULT: FAIL\nNative exception in self-test.");
    }
}
