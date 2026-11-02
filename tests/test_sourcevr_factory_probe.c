#include <assert.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* A factory probe must never return a fake interface pointer. */
typedef void* (*FactoryFn)(const char*, int*);
typedef unsigned int (*CountFn)(void);

static void* symbol(void* library, const char* name) {
    dlerror();
    void* p = dlsym(library, name);
    const char* error = dlerror();
    if (error || !p) {
        fprintf(stderr, "Missing %s: %s\n", name, error ? error : "null");
        return NULL;
    }
    return p;
}

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    void* module = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!module) {
        fprintf(stderr, "dlopen failed: %s\n", dlerror());
        return 1;
    }
    FactoryFn factory = NULL;
    CountFn count = NULL;
    void* p = symbol(module, "CreateInterface");
    if (!p) return 1;
    memcpy(&factory, &p, sizeof(factory));
    p = symbol(module, "SinFactoryProbeRequestCount");
    if (!p) return 1;
    memcpy(&count, &p, sizeof(count));
    int result = -1;
    assert(factory("VClient017", &result) == NULL && result == 1);
    assert(factory("ServerGameDLL010", NULL) == NULL);
    assert(count() == 2);
    assert(dlclose(module) == 0);
    puts("PASS: Source factory diagnostic returns failure safely");
    return 0;
}
