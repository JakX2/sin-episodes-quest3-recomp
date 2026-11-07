/* Independently-authored Android ELF64 game-module LOAD PROBE.
 * This file is compiled twice with distinct native game module names:
 * libserver.so and libclient.so. No original game code, NPC classes, or
 * native SourceVR vtables are linked, and real engine interface requests
 * always fail closed.
 */
#include "sin_compat/sourcevr_module_probe.h"
#include <stddef.h>
#include <string.h>

#if defined(SIN_SOURCEVR_PROBE_BUILD_SERVER) == defined(SIN_SOURCEVR_PROBE_BUILD_CLIENT)
#error "Define exactly one sourcevr probe module role"
#endif

#if defined(SIN_SOURCEVR_PROBE_BUILD_SERVER)
#define SIN_SOURCEVR_CURRENT_ROLE SIN_SOURCEVR_PROBE_SERVER
#else
#define SIN_SOURCEVR_CURRENT_ROLE SIN_SOURCEVR_PROBE_CLIENT
#endif

static sin_sourcevr_probe_descriptor_v1 descriptor = {
    SIN_SOURCEVR_PROBE_MAGIC,
    SIN_SOURCEVR_PROBE_VERSION,
    SIN_SOURCEVR_CURRENT_ROLE,
    (uint32_t)(sizeof(void*) * 8u)
};

void *CreateInterface(const char *name, int *return_code) {
    if (return_code != NULL) *return_code = SIN_SOURCEVR_PROBE_IFACE_FAILED;
    /* The probe name is private and cannot be mistaken for a real game API.
     * A real SourceVR engine will only request the usual game interfaces,
     * which return NULL. We never dispatch a guessed C++ vtable method.
     */
    if (name == NULL || strcmp(name, SIN_SOURCEVR_PROBE_INTERFACE) != 0)
        return NULL;
    if (return_code != NULL) *return_code = 0;
    return &descriptor;
}
