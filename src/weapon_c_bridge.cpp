#define SIN_WEAPON_BRIDGE_BUILD
#include "sin_compat/weapon_c_bridge.h"
#include "sin_compat/weapon_runtime.hpp"

#include <cmath>
#include <string>

namespace {

class CallbackHost final : public sin_compat::IWeaponHost {
public:
    CallbackHost(SinWeaponHostCallbacks cb, void* user)
        : callbacks_(cb), user_(user) {}

    int resolve_sequence(int activity_id) override {
        return callbacks_.resolve_sequence(user_, activity_id);
    }
    void bind_viewmodel(const std::string& model,
                        const std::array<float, 3>& offset) override {
        callbacks_.bind_viewmodel(user_, model.c_str(), offset.data());
    }
    void play_sequence(int sequence) override {
        callbacks_.play_sequence(user_, sequence);
    }
    void set_flatscreen_fov(int fov, float duration) override {
        callbacks_.set_flatscreen_fov(user_, fov, duration);
    }

private:
    SinWeaponHostCallbacks callbacks_;
    void* user_;
};

bool settings_ok(const SinWeaponSettings* settings) {
    return settings && settings->viewmodel_name && settings->viewmodel_name[0] &&
           settings->zoom_fov >= 0 && settings->zoom_fov <= 179 &&
           settings->clip_size >= -1 && settings->clip2_size >= -1 &&
           std::isfinite(settings->origin_offset[0]) &&
           std::isfinite(settings->origin_offset[1]) &&
           std::isfinite(settings->origin_offset[2]);
}

bool callbacks_ok(const SinWeaponHostCallbacks* callbacks) {
    return callbacks && callbacks->resolve_sequence && callbacks->bind_viewmodel &&
           callbacks->play_sequence && callbacks->set_flatscreen_fov;
}

sin_compat::WeaponConfig make_config(const SinWeaponSettings& settings) {
    sin_compat::WeaponConfig config;
    config.viewmodel_name = settings.viewmodel_name;
    config.origin_offset = {settings.origin_offset[0], settings.origin_offset[1],
                            settings.origin_offset[2]};
    config.zoom_fov = settings.zoom_fov;
    config.ammo.clip_size = settings.clip_size;
    config.ammo.clip2_size = settings.clip2_size;
    return config;
}

sin_compat::WeaponKind weapon_kind(int32_t kind) {
    switch (kind) {
        case SIN_WEAPON_MAGNUM: return sin_compat::WeaponKind::Magnum;
        case SIN_WEAPON_ASSAULT_RIFLE: return sin_compat::WeaponKind::AssaultRifle;
        default: return sin_compat::WeaponKind::Scattergun;
    }
}

}  // namespace

struct SinWeaponHandle {
    CallbackHost host;
    sin_compat::WeaponRuntime runtime;
    SinWeaponHandle(int32_t kind, const SinWeaponSettings& settings,
                    const SinWeaponHostCallbacks& callbacks, void* user, bool vr)
        : host(callbacks, user), runtime(weapon_kind(kind), make_config(settings), host, vr) {}
};

extern "C" {
uint32_t sin_weapon_abi_version(void) { return SIN_WEAPON_BRIDGE_ABI_VERSION; }

SinWeaponHandle* sin_weapon_create(int32_t kind, const SinWeaponSettings* settings,
                                    const SinWeaponHostCallbacks* callbacks,
                                    void* user, int32_t vr_mode) {
    if (kind < SIN_WEAPON_MAGNUM || kind > SIN_WEAPON_SCATTERGUN ||
        !settings_ok(settings) || !callbacks_ok(callbacks)) return nullptr;
    try {
        return new SinWeaponHandle(kind, *settings, *callbacks, user, vr_mode != 0);
    } catch (...) { return nullptr; }
}

void sin_weapon_destroy(SinWeaponHandle* handle) {
    delete handle;
}

int32_t sin_weapon_frame(SinWeaponHandle* handle, uint32_t buttons,
                         int32_t player_exists, int32_t owner_ready,
                         int32_t external_zoom_block, SinWeaponFrameResult* out) {
    if (!handle || !out) return 0;
    try {
        const auto r = handle->runtime.frame(buttons, player_exists != 0,
                                               owner_ready != 0, external_zoom_block != 0);
        *out = {r.entered_zoom, r.exited_zoom, r.automatically_restored, r.was_interrupted};
        return 1;
    } catch (...) { return 0; }
}

int32_t sin_weapon_set_ammo(SinWeaponHandle* handle, int32_t clip, int32_t reserve,
                            int32_t clip2, int32_t reserve2) {
    if (!handle || clip < 0 || reserve < 0 || clip2 < 0 || reserve2 < 0) return 0;
    auto& ammo = handle->runtime.ammo();
    ammo.clip = clip;
    ammo.reserve = reserve;
    ammo.clip2 = clip2;
    ammo.reserve2 = reserve2;
    return 1;
}

int32_t sin_weapon_get_ammo(const SinWeaponHandle* handle, SinWeaponAmmo* out) {
    if (!handle || !out) return 0;
    const auto& ammo = handle->runtime.ammo();
    *out = {ammo.clip, ammo.reserve, ammo.clip2, ammo.reserve2,
            ammo.reload_active, handle->runtime.is_zoomed(),
            handle->runtime.is_holstered()};
    return 1;
}

int32_t sin_weapon_begin_reload(SinWeaponHandle* handle, int32_t start_activity,
                                int32_t end_activity) {
    if (!handle) return 0;
    try { return handle->runtime.begin_reload(start_activity, end_activity); }
    catch (...) { return 0; }
}

int32_t sin_weapon_reload_transfer_event(SinWeaponHandle* handle, int32_t owner_valid,
                                         int32_t* primary_transferred,
                                         int32_t* secondary_transferred) {
    if (!handle || !primary_transferred || !secondary_transferred) return 0;
    *primary_transferred = 0;
    *secondary_transferred = 0;
    try {
        const auto r = handle->runtime.transfer_bulk_at_validated_event(owner_valid != 0);
        *primary_transferred = r.primary;
        *secondary_transferred = r.secondary;
        return 1;
    } catch (...) { return 0; }
}

int32_t sin_weapon_finish_reload(SinWeaponHandle* handle) {
    if (!handle) return 0;
    handle->runtime.finish_reload();
    return 1;
}

int32_t sin_weapon_primary_attack(SinWeaponHandle* handle, int32_t counter) {
    if (!handle) return 0;
    try { return handle->runtime.primary_attack(counter); }
    catch (...) { return 0; }
}

int32_t sin_weapon_secondary_attack(SinWeaponHandle* handle) {
    if (!handle) return 0;
    try { return handle->runtime.secondary_attack(); }
    catch (...) { return 0; }
}

int32_t sin_weapon_holster(SinWeaponHandle* handle) {
    if (!handle) return 0;
    try { return handle->runtime.holster(); }
    catch (...) { return 0; }
}
}  // extern "C"
