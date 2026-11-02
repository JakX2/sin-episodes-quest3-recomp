#include "sin_compat/engine_event_pump.hpp"

namespace sin_compat {

EngineEventPump::EngineEventPump(const sin_host_callbacks_v1& callbacks, void* userdata) noexcept
    : callbacks_(callbacks), userdata_(userdata) {}

EngineEventPump::~EngineEventPump() noexcept { detach(); }

void EngineEventPump::detach() noexcept {
    if (!session_) return;
    // Cleanup is performed while the host still owns its callback context.
    // This method must never be invoked re-entrantly from a host callback.
    sin_session_cleanup_v1(session_);
    sin_session_destroy_v1(session_);
    session_ = nullptr;
    player_ = {};
    weapon_ = {};
}

bool EngineEventPump::attach(sin_entity_ref_v1 player, sin_entity_ref_v1 weapon,
                             int32_t kind, const sin_weapon_config_v1& config,
                             bool vr_mode) {
    detach();
    if (callbacks_.version != SIN_COMPAT_ABI_VERSION_1 ||
        callbacks_.struct_size != sizeof(callbacks_) ||
        !callbacks_.is_live || !callbacks_.is_live(userdata_, player) ||
        !callbacks_.is_live(userdata_, weapon)) {
        return false;
    }
    session_ = sin_session_create_v1(
        &callbacks_, userdata_, player, weapon, kind, &config, vr_mode ? 1 : 0);
    if (!session_) return false;
    player_ = player;
    weapon_ = weapon;
    return true;
}

bool EngineEventPump::valid_session() {
    if (!session_) return false;
    // Identity is a (slot, generation) pair, not a slot number alone.
    // Host callbacks are responsible for comparing both.
    if (!callbacks_.is_live(userdata_, player_) ||
        !callbacks_.is_live(userdata_, weapon_)) {
        detach();
        return false;
    }
    return true;
}

int32_t EngineEventPump::on_equip() {
    return valid_session() ? sin_session_ensure_viewmodel_v1(session_) : -1;
}

int32_t EngineEventPump::on_hide_weapon() {
    return valid_session() ? sin_session_hide_weapon_v1(session_) : -1;
}

int32_t EngineEventPump::on_usercmd(uint32_t bits, bool blocked) {
    return valid_session() ? sin_session_usercmd_v1(session_, bits, blocked ? 1 : 0) : -1;
}

int32_t EngineEventPump::on_reload_begin(int32_t start_activity, int32_t end_activity) {
    return valid_session() ? sin_session_reload_begin_v1(session_, start_activity, end_activity) : -1;
}

int32_t EngineEventPump::on_reload_event(int32_t& primary, int32_t& secondary) {
    if (!valid_session()) return -1;
    int32_t p = 0, s = 0;
    const int32_t result = sin_session_reload_event_v1(session_, &p, &s);
    if (result == 0) {
        primary = p;
        secondary = s;
    }
    return result;
}

int32_t EngineEventPump::on_reload_end() {
    return valid_session() ? sin_session_reload_end_v1(session_) : -1;
}

int32_t EngineEventPump::on_set_ammo(sin_ammo_v1 ammo) {
    return valid_session() ? sin_session_set_ammo_v1(session_, ammo) : -1;
}

int32_t EngineEventPump::on_get_ammo(sin_ammo_v1& ammo) {
    return valid_session() ? sin_session_get_ammo_v1(session_, &ammo) : -1;
}

int32_t EngineEventPump::on_holster() {
    return valid_session() ? sin_session_holster_v1(session_) : -1;
}

} // namespace sin_compat
