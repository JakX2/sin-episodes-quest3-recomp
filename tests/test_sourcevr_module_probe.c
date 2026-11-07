/* C11/Android Bionic independent dynamic-loader smoke of TWO inert modules.
 * No SourceVR engine is loaded. Path handling is a controlled simulation of
 * the proven APP_MOD_LIB / APP_LIB_PATH and lib*.so naming conventions.
 */
#define _POSIX_C_SOURCE 200809L
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "sin_compat/sourcevr_module_probe.h"
#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void *(*probe_factory_fn)(const char *, int *);
_Static_assert(sizeof(probe_factory_fn) == sizeof(void *),
               "POSIX dlsym test needs pointer-sized C function addresses");

static void *try_load_name(const char *name) {
    char filename[128];
    char fullpath[4096];
    const char *suffix = ".so";
    if (!name || strchr(name, '/') || !*name) return NULL;
    const size_t n = strlen(name);
    const int already_so = n >= 3 && strcmp(name + n - 3, suffix) == 0;
    const int already_lib = n >= 3 && strncmp(name, "lib", 3) == 0;
    const int wrote = snprintf(filename, sizeof filename, "%s%s%s",
                                already_lib ? "" : "lib", name,
                                already_so ? "" : suffix);
    if (wrote <= 0 || (size_t)wrote >= sizeof filename) return NULL;
    const char *dirs[] = {getenv("APP_MOD_LIB"), getenv("APP_LIB_PATH")};
    for (size_t i = 0; i < 2; ++i) {
        const char *base = dirs[i];
        if (!base || !*base) continue;
        const int pathsize = snprintf(fullpath, sizeof fullpath,
                                      "%s/%s", base, filename);
        if (pathsize <= 0 || (size_t)pathsize >= sizeof fullpath) continue;
        void *module = dlopen(fullpath, RTLD_NOW | RTLD_LOCAL);
        if (module) return module;
    }
    return NULL;
}

static void exercise_module(const char *source_name, uint32_t role) {
    void *module = try_load_name(source_name);
    if (!module) {
        fprintf(stderr, "probe dlopen failed for %s: %s\n",
                source_name, dlerror());
    }
    assert(module);
    void *symbol = dlsym(module, "CreateInterface");
    assert(symbol);
    probe_factory_fn factory = NULL;
    memcpy(&factory, &symbol, sizeof factory);
    int code = -19;
    const char *invalid[] = {
        "ServerGameDLL010", "ServerGameDLL009", "ServerGameDLL008",
        "ServerGameDLL007", "ServerGameDLL006", "ServerGameDLL005",
        "ServerGameDLL004", "ServerGameDLL003",
        "VClient017", "VClient011", "SiNProbeModule002",
        "sinprobemodule001", ""
    };
    for (size_t i = 0; i < sizeof invalid/sizeof invalid[0]; ++i) {
        code = -19;
        assert(factory(invalid[i], &code) == NULL);
        assert(code == SIN_SOURCEVR_PROBE_IFACE_FAILED);
    }
    code = -19;
    assert(factory(NULL, &code) == NULL);
    assert(code == SIN_SOURCEVR_PROBE_IFACE_FAILED);
    assert(factory("ServerGameDLL010", NULL) == NULL);
    assert(factory(SIN_SOURCEVR_PROBE_INTERFACE, NULL) != NULL);
    code = -19;
    const sin_sourcevr_probe_descriptor_v1 *descriptor =
        (const sin_sourcevr_probe_descriptor_v1 *)
        factory(SIN_SOURCEVR_PROBE_INTERFACE, &code);
    assert(descriptor && code == 0);
    assert(descriptor->magic == SIN_SOURCEVR_PROBE_MAGIC);
    assert(descriptor->version == SIN_SOURCEVR_PROBE_VERSION);
    assert(descriptor->module_role == role);
    assert(descriptor->pointer_bits == sizeof(void *)*8u);
    assert(dlclose(module) == 0);
}

int main(int argc, char **argv) {
    assert(argc == 2);
    const char *staged = argv[1];
    assert(staged && *staged);
    assert(setenv("APP_MOD_LIB", staged, 1) == 0);
    assert(setenv("APP_LIB_PATH", "/nonexistent/sin-probe-lib-path", 1) == 0);
    exercise_module("server.so", SIN_SOURCEVR_PROBE_SERVER);
    exercise_module("client", SIN_SOURCEVR_PROBE_CLIENT);

    /* Simulate fallback to APP_LIB_PATH after a mod-dir miss. This tests
     * only the independent harness; never claims native SourceVR execution.
     */
    assert(setenv("APP_MOD_LIB", "/nonexistent/sin-probe-mod-path", 1) == 0);
    assert(setenv("APP_LIB_PATH", staged, 1) == 0);
    exercise_module("libserver.so", SIN_SOURCEVR_PROBE_SERVER);
    exercise_module("libclient.so", SIN_SOURCEVR_PROBE_CLIENT);

    assert(setenv("APP_LIB_PATH", "/nonexistent/sin-probe-lib-path", 1) == 0);
    assert(try_load_name("server.so") == NULL);
    assert(try_load_name("client") == NULL);
    assert(try_load_name("/absolute/path.so") == NULL);
    puts("PASS: two native libserver/libclient CreateInterface probes dlopen via simulated GAMEBIN paths; real game interfaces fail closed");
    return 0;
}
