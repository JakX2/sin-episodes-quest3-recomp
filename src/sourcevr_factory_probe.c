/* Diagnostic replacement game module: factory call trace ONLY.
 * Returns failure on every interface request by design. NOT a Source game DLL.
 * Never install over stock libs; use only in a disposable isolated profile.
 */
#include <stddef.h>
#include <stdio.h>
#if defined(__ANDROID__)
#include <android/log.h>
#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "SiNFactoryProbe", __VA_ARGS__)
#else
#define LOG(...) do { fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } while (0)
#endif

#if defined(_WIN32)
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif

static unsigned int requests;

__attribute__((constructor)) static void on_load(void) {
    LOG("MODULE_LOADED: diagnostic factory only, no Source ABI implementation");
}

EXPORT void* CreateInterface(const char* name, int* return_code) {
    ++requests;
    LOG("INTERFACE_REQUEST: %s", name ? name : "(null)");
    if (return_code) *return_code = 1;  /* IFACE_FAILED */
    return NULL;  /* Never hand the engine an invented vtable. */
}

EXPORT unsigned int SinFactoryProbeRequestCount(void) {
    return requests;
}
