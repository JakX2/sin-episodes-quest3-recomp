#ifndef SIN_COMPAT_SOURCEVR_MODULE_PROBE_H
#define SIN_COMPAT_SOURCEVR_MODULE_PROBE_H
/* The only supported interface in this deliberately inert game-module
 * loading probe. NOT IServerGameDLL, IBaseClientDLL, a game factory, or a
 * playable Quest port. Compile separately as libserver.so/libclient.so.
 *
 * The SourceVR engine has been observed to request ServerGameDLL010..004
 * or VClient017; all of those MUST return NULL on this probe build.
 */
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define SIN_SOURCEVR_PROBE_INTERFACE "SiNProbeModule001"
enum { SIN_SOURCEVR_PROBE_MAGIC = 0x53494e50u,
       SIN_SOURCEVR_PROBE_VERSION = 1u,
       SIN_SOURCEVR_PROBE_SERVER = 1u,
       SIN_SOURCEVR_PROBE_CLIENT = 2u,
       SIN_SOURCEVR_PROBE_IFACE_FAILED = 1 };
typedef struct sin_sourcevr_probe_descriptor_v1 {
    uint32_t magic;
    uint32_t version;
    uint32_t module_role;
    uint32_t pointer_bits;
} sin_sourcevr_probe_descriptor_v1;

/* The one ABI symbol exported by each prototype native game module.
 * The C signature matches the public name of the Source factory, but not
 * any unverified game-specific virtual interface implementation.
 * Writes *return_code=0 on our private probe name, 1 otherwise.
 */
#if defined(__GNUC__) || defined(__clang__)
__attribute__((visibility("default")))
#endif
void *CreateInterface(const char *name, int *return_code);

#ifdef __cplusplus
}
#endif
#endif
