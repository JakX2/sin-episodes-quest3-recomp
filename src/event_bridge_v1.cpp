#define SIN_COMPAT_ABI_EXPORT
#include "sin_compat/event_bridge_v1.h"
#include "sin_compat/engine_event_pump.hpp"
#include <new>

struct sin_event_bridge_v1 {
    bool busy = false;
    sin_compat::EngineEventPump pump;

    sin_event_bridge_v1(const sin_host_callbacks_v1& callbacks, void* userdata)
        : pump(callbacks, userdata) {}
};

namespace {
struct Gate final {
    sin_event_bridge_v1* bridge;
    bool entered = false;

    explicit Gate(sin_event_bridge_v1* value) noexcept : bridge(value) {
        if (bridge && !bridge->busy) {
            bridge->busy = true;
            entered = true;
        }
    }
    ~Gate() noexcept {
        if (entered) bridge->busy = false;
    }
    Gate(const Gate&) = delete;
    Gate& operator=(const Gate&) = delete;
    explicit operator bool() const noexcept { return entered; }
};
}

extern "C" {

uint32_t sin_event_bridge_abi_version_v1(void) {
    return SIN_EVENT_BRIDGE_ABI_VERSION_1;
}

sin_event_bridge_v1 *sin_event_bridge_create_v1(
    const sin_host_callbacks_v1 *cb, void *context) {
    if (!cb || cb->version != SIN_COMPAT_ABI_VERSION_1 ||
        cb->struct_size != sizeof(sin_host_callbacks_v1)) {
        return nullptr;
    }
    try {
        return new (std::nothrow) sin_event_bridge_v1(*cb, context);
    } catch (...) {
        return nullptr;
    }
}

int32_t sin_event_bridge_attach_v1(
    sin_event_bridge_v1 *b, sin_entity_ref_v1 player, sin_entity_ref_v1 weapon,
    int32_t kind, const sin_weapon_config_v1 *config, int32_t vr_mode) {
    if (!b || !config) return -1;
    Gate call(b);
    if (!call) return -1;
    try {
        return b->pump.attach(player, weapon, kind, *config, vr_mode != 0) ? 1 : 0;
    } catch (...) {
        return -1;
    }
}

int32_t sin_event_bridge_dispatch_v1(
    sin_event_bridge_v1 *b, int32_t event,
    const sin_bridge_event_input_v1 *input, sin_bridge_event_output_v1 *output) {
    if (!b) return -1;
    Gate call(b);
    if (!call) return -1;
    try {
        switch (event) {
        case SIN_BRIDGE_EQUIP_V1:
            return b->pump.on_equip();
        case SIN_BRIDGE_HIDE_WEAPON_V1:
            return b->pump.on_hide_weapon();
        case SIN_BRIDGE_USERCMD_V1:
            return input ? b->pump.on_usercmd(input->buttons, input->blocked != 0) : -1;
        case SIN_BRIDGE_RELOAD_BEGIN_V1:
            return input ? b->pump.on_reload_begin(
                input->start_activity, input->end_activity) : -1;
        case SIN_BRIDGE_RELOAD_TRANSFER_V1: {
            if (!output) return -1;
            int32_t primary = 0, secondary = 0;
            const int32_t result = b->pump.on_reload_event(primary, secondary);
            if (result == 0) {
                output->primary_transfer = primary;
                output->secondary_transfer = secondary;
            }
            return result;
        }
        case SIN_BRIDGE_RELOAD_END_V1:
            return b->pump.on_reload_end();
        case SIN_BRIDGE_SET_AMMO_V1:
            return input ? b->pump.on_set_ammo(input->ammo) : -1;
        case SIN_BRIDGE_GET_AMMO_V1: {
            if (!output) return -1;
            sin_ammo_v1 value{};
            const int32_t result = b->pump.on_get_ammo(value);
            if (result == 0) output->ammo = value;
            return result;
        }
        case SIN_BRIDGE_HOLSTER_V1:
            return b->pump.on_holster();
        case SIN_BRIDGE_MAP_UNLOAD_V1:
            b->pump.on_map_unload();
            return 0;
        case SIN_BRIDGE_WEAPON_REMOVED_V1:
            b->pump.on_weapon_removed();
            return 0;
        default:
            return -1;
        }
    } catch (...) {
        return -1;
    }
}

int32_t sin_event_bridge_is_active_v1(const sin_event_bridge_v1 *b) {
    if (!b || b->busy) return -1;
    return b->pump.active() ? 1 : 0;
}

void sin_event_bridge_destroy_v1(sin_event_bridge_v1 *b) {
    if (!b || b->busy) return;
    // Ensure callbacks triggered during pump cleanup cannot reenter and
    // destroy the bridge twice. The pointer is invalid after this returns.
    b->busy = true;
    delete b;
}

} // extern "C"
