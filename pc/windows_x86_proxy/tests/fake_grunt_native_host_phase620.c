/* Phase620: synthetic DLLs exist ONLY to verify fail-open factory
 * forwarding when the separate native host ABI is rejected.
 * These never allocate sessions, entities, or any Source object.
 */
#include <stdint.h>
uint32_t __cdecl sin_grunt_native_abi_version_v1(void) {
#if defined(SIN_NATIVE_FAKE_MISSING_EXPORT)
    return 1u;
#else
    return 2u;
#endif
}
#if !defined(SIN_NATIVE_FAKE_MISSING_EXPORT)
void* __cdecl sin_grunt_native_create_v1(void) {return 0;}
int32_t __cdecl sin_grunt_native_destroy_v1(void) {return -1;}
int32_t __cdecl sin_grunt_native_attach_v1(void) {return -1;}
int32_t __cdecl sin_grunt_native_step_v1(void) {return -1;}
int32_t __cdecl sin_grunt_native_snapshot_v1(void) {return -1;}
int32_t __cdecl sin_grunt_native_detach_v1(void) {return -1;}
#endif
