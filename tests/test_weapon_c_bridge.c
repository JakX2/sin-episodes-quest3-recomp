#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "sin_compat/weapon_c_bridge.h"

typedef struct HostLog {
    int resolves;
    int binds;
    int plays;
    int fov_calls;
    int last_fov;
    int last_activity;
    char last_model[128];
} HostLog;

static int32_t resolve(void* user, int32_t activity) {
    HostLog* log = (HostLog*)user;
    ++log->resolves;
    log->last_activity = activity;
    return activity + 1000;
}
static void bind(void* user, const char* model, const float offset[3]) {
    HostLog* log = (HostLog*)user;
    ++log->binds;
    assert(fabsf(offset[0] - 0.25f) < 0.00001f);
    snprintf(log->last_model, sizeof(log->last_model), "%s", model);
}
static void play(void* user, int32_t sequence) {
    HostLog* log = (HostLog*)user;
    assert(sequence >= 1000);
    ++log->plays;
}
static void fov(void* user, int32_t degrees, float duration) {
    HostLog* log = (HostLog*)user;
    assert(duration > 0.0f);
    ++log->fov_calls;
    log->last_fov = degrees;
}
static const SinWeaponHostCallbacks callbacks = { resolve, bind, play, fov };
static const SinWeaponSettings settings = { "models/test.mdl", {0.25f,0.0f,0.0f}, 55, 6, -1 };

static void test_vr_round_trip(void) {
    HostLog log = {0};
    SinWeaponFrameResult result = {0};
    SinWeaponAmmo ammo = {0};
    int32_t primary = -1;
    int32_t secondary = -1;
    SinWeaponHandle* h = sin_weapon_create(SIN_WEAPON_MAGNUM,&settings,&callbacks,&log,1);
    assert(h);
    assert(sin_weapon_abi_version() == SIN_WEAPON_BRIDGE_ABI_VERSION);
    assert(sin_weapon_set_ammo(h,2,9,0,0));
    assert(sin_weapon_frame(h,0x80000u,1,1,0,&result));
    assert(result.entered_zoom);
    assert(log.resolves > 0 && log.binds > 0 && log.plays > 0);
    assert(strcmp(log.last_model,"models/test.mdl") == 0);
    assert(log.fov_calls == 0);
    assert(sin_weapon_begin_reload(h,-1,-1));
    assert(sin_weapon_frame(h,0,1,1,0,&result));
    assert(result.was_interrupted);
    assert(sin_weapon_reload_transfer_event(h,0,&primary,&secondary));
    assert(primary == 0 && secondary == 0);
    assert(sin_weapon_reload_transfer_event(h,1,&primary,&secondary));
    assert(primary == 4 && secondary == 0);
    assert(sin_weapon_get_ammo(h,&ammo));
    assert(ammo.clip == 6 && ammo.reserve == 5 && ammo.reload_active);
    assert(sin_weapon_finish_reload(h));
    assert(sin_weapon_holster(h));
    assert(sin_weapon_get_ammo(h,&ammo) && ammo.holstered && !ammo.zoom_active);
    assert(log.fov_calls == 0);
    sin_weapon_destroy(h);
}
static void test_flatscreen_and_bad_inputs(void) {
    HostLog log = {0};
    SinWeaponFrameResult result = {0};
    SinWeaponSettings bad = settings;
    SinWeaponHandle* h;
    bad.origin_offset[0] = NAN;
    assert(!sin_weapon_create(0,&bad,&callbacks,&log,1));
    bad = settings;
    assert(!sin_weapon_create(9,&bad,&callbacks,&log,1));
    assert(!sin_weapon_create(0,&bad,0,&log,1));
    assert(!sin_weapon_get_ammo(0,0));
    h = sin_weapon_create(0,&bad,&callbacks,&log,0);
    assert(h);
    assert(!sin_weapon_set_ammo(h,-1,5,0,0));
    assert(sin_weapon_frame(h,0x80000u,1,1,0,&result));
    assert(log.fov_calls == 1 && log.last_fov == 55);
    assert(sin_weapon_frame(h,0,1,1,0,&result));
    assert(sin_weapon_frame(h,0x80000u,1,1,0,&result));
    assert(log.fov_calls == 2 && log.last_fov == 0);
    sin_weapon_destroy(h);
    sin_weapon_destroy(0);
}
int main(void) {
    test_vr_round_trip();
    test_flatscreen_and_bad_inputs();
    puts("C ABI weapon bridge tests passed");
    return 0;
}
