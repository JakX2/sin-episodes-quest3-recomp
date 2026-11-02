#ifndef SIN_COMPAT_ENGINE_EVENT_PUMP_HPP
#define SIN_COMPAT_ENGINE_EVENT_PUMP_HPP

#include "sin_compat/host_abi_v1.h"
#include <cstdint>

namespace sin_compat {

/* A small, independently authored bridge between engine-like events and our
 * existing v1 C shared-library ABI. No Source/SourceVR SDK is assumed.
 *
 * The caller must provide working host callbacks with generation-safe refs
 * and deliver events on a single thread; do NOT call this from host callbacks.
 * The pump owns one session. It invalidates the session on lost player/weapon
 * identity, and is idempotent across explicit map or weapon teardown.
 * No SourceVR loading or game execution is implied by this class.
 */
class EngineEventPump final {
public:
    EngineEventPump(const sin_host_callbacks_v1& callbacks, void* userdata) noexcept;
    ~EngineEventPump() noexcept;

    EngineEventPump(const EngineEventPump&) = delete;
    EngineEventPump& operator=(const EngineEventPump&) = delete;
    EngineEventPump(EngineEventPump&&) = delete;
    EngineEventPump& operator=(EngineEventPump&&) = delete;

    bool attach(sin_entity_ref_v1 player, sin_entity_ref_v1 weapon,
                int32_t kind, const sin_weapon_config_v1& config,
                bool vr_mode);
    bool active() const noexcept { return session_ != nullptr; }

    // Any event returns -1 if detached or the tracked entity generation died.
    int32_t on_equip();
    int32_t on_hide_weapon();
    int32_t on_usercmd(uint32_t bits, bool blocked);
    int32_t on_reload_begin(int32_t start_activity, int32_t end_activity);
    int32_t on_reload_event(int32_t& primary_transferred,
                            int32_t& secondary_transferred);
    int32_t on_reload_end();
    int32_t on_set_ammo(sin_ammo_v1 ammo);
    int32_t on_get_ammo(sin_ammo_v1& ammo);
    int32_t on_holster();

    // Host must call before disposing entity objects or unloading a map.
    void detach() noexcept;
    void on_map_unload() noexcept { detach(); }
    void on_weapon_removed() noexcept { detach(); }

private:
    bool valid_session();
    sin_host_callbacks_v1 callbacks_;
    void* userdata_;
    sin_entity_ref_v1 player_{};
    sin_entity_ref_v1 weapon_{};
    sin_session_v1* session_ = nullptr;
};

} // namespace sin_compat
#endif
