/* Headless, read-only runtime loader test for our own SiN C ABI.
 * It does not load, patch or replace SourceVR's game/engine binaries. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sin_compat/weapon_c_bridge.h"

typedef uint32_t (*AbiVersionFn)(void);
typedef SinWeaponHandle* (*CreateFn)(int32_t, const SinWeaponSettings*,
                                    const SinWeaponHostCallbacks*, void*, int32_t);
typedef void (*DestroyFn)(SinWeaponHandle*);
typedef int32_t (*FrameFn)(SinWeaponHandle*, uint32_t, int32_t,
                           int32_t, int32_t, SinWeaponFrameResult*);
typedef int32_t (*SetAmmoFn)(SinWeaponHandle*, int32_t, int32_t, int32_t, int32_t);
typedef int32_t (*GetAmmoFn)(const SinWeaponHandle*, SinWeaponAmmo*);
typedef int32_t (*BeginReloadFn)(SinWeaponHandle*, int32_t, int32_t);
typedef int32_t (*TransferFn)(SinWeaponHandle*, int32_t, int32_t*, int32_t*);
typedef int32_t (*FinishReloadFn)(SinWeaponHandle*);
typedef int32_t (*HolsterFn)(SinWeaponHandle*);

typedef struct Log {
    int resolves, binds, plays, fov_calls, fov, activity;
} Log;

static int32_t resolve(void* user, int32_t activity) {
    Log* log = (Log*)user;
    ++log->resolves;
    log->activity = activity;
    return activity + 1024;
}
static void bind(void* user, const char* model, const float offset[3]) {
    Log* log = (Log*)user;
    assert(strcmp(model, "models/weapons/v_magnum/v_magnum.mdl") == 0);
    assert(offset[0] == -0.052f);
    ++log->binds;
}
static void play(void* user, int32_t sequence) {
    Log* log = (Log*)user;
    assert(sequence >= 1024);
    ++log->plays;
}
static void fov(void* user, int32_t degrees, float seconds) {
    Log* log = (Log*)user;
    assert(seconds > 0.0f);
    log->fov = degrees;
    ++log->fov_calls;
}

static void* required_symbol(void* module, const char* name) {
    dlerror();
    void* p = dlsym(module, name);
    const char* e = dlerror();
    if (e || !p) {
        fprintf(stderr, "Missing required symbol %s: %s\n", name, e ? e : "null");
        exit(2);
    }
    return p;
}
/* POSIX dlsym function pointers are handled by memcpy to avoid pedantic casts. */
#define LOAD(fn, name) do { void* sym = required_symbol(module, name); \
    assert(sizeof(fn) == sizeof(sym)); memcpy(&(fn), &sym, sizeof(sym)); } while (0)

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s /path/to/libsin_weapon_bridge.so\n", argv[0]);
        return 2;
    }
    void* module = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!module) {
        fprintf(stderr, "dlopen: %s\n", dlerror());
        return 1;
    }
    AbiVersionFn abi_version = NULL;
    CreateFn create = NULL;
    DestroyFn destroy = NULL;
    FrameFn frame = NULL;
    SetAmmoFn set_ammo = NULL;
    GetAmmoFn get_ammo = NULL;
    BeginReloadFn begin_reload = NULL;
    TransferFn transfer = NULL;
    FinishReloadFn finish_reload = NULL;
    HolsterFn holster = NULL;
    LOAD(abi_version, "sin_weapon_abi_version");
    LOAD(create, "sin_weapon_create");
    LOAD(destroy, "sin_weapon_destroy");
    LOAD(frame, "sin_weapon_frame");
    LOAD(set_ammo, "sin_weapon_set_ammo");
    LOAD(get_ammo, "sin_weapon_get_ammo");
    LOAD(begin_reload, "sin_weapon_begin_reload");
    LOAD(transfer, "sin_weapon_reload_transfer_event");
    LOAD(finish_reload, "sin_weapon_finish_reload");
    LOAD(holster, "sin_weapon_holster");
    assert(abi_version() == SIN_WEAPON_BRIDGE_ABI_VERSION);

    const SinWeaponSettings magnum = {
        "models/weapons/v_magnum/v_magnum.mdl", {-0.052f, 0.0f, -0.1f}, 50, 20, -1
    };
    const SinWeaponHostCallbacks callbacks = {resolve, bind, play, fov};
    SinWeaponFrameResult result = {0};
    SinWeaponAmmo ammo = {0};
    int32_t primary = 0, secondary = 0;
    Log vr_log = {0};
    SinWeaponHandle* vr = create(SIN_WEAPON_MAGNUM, &magnum, &callbacks, &vr_log, 1);
    assert(vr && set_ammo(vr, 4, 25, 0, 0));
    assert(frame(vr, 0x00080000u, 1, 1, 0, &result));
    assert(result.entered_zoom && vr_log.activity == 0xc0);
    assert(vr_log.resolves == 1 && vr_log.binds == 1 &&
           vr_log.plays == 1 && vr_log.fov_calls == 0);
    assert(begin_reload(vr, -1, -1));
    assert(frame(vr, 0u, 1, 1, 0, &result));
    assert(result.exited_zoom && result.was_interrupted);
    assert(transfer(vr, 1, &primary, &secondary));
    assert(primary == 16 && secondary == 0);
    assert(get_ammo(vr, &ammo) && ammo.clip == 20 && ammo.reserve == 9);
    assert(finish_reload(vr) && holster(vr));
    assert(get_ammo(vr, &ammo) && ammo.holstered && vr_log.fov_calls == 0);
    destroy(vr);

    Log flat_log = {0};
    SinWeaponHandle* flat = create(SIN_WEAPON_MAGNUM, &magnum, &callbacks,
                                   &flat_log, 0);
    assert(flat);
    assert(frame(flat, 0x00080000u, 1, 1, 0, &result));
    assert(flat_log.fov_calls == 1 && flat_log.fov == 50);
    assert(frame(flat, 0u, 1, 1, 0, &result));
    assert(frame(flat, 0x00080000u, 1, 1, 0, &result));
    assert(flat_log.fov_calls == 2 && flat_log.fov == 0);
    destroy(flat);
    if (dlclose(module)) {
        fprintf(stderr, "dlclose: %s\n", dlerror());
        return 1;
    }
    puts("PASS: dlopen + C ABI + zoom/reload/VR-FOV callbacks");
    return 0;
}
