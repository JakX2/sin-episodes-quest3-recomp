/*
 * Phase203: native Win32 x86 probe of the existing original-DLL forwarding
 * boundary. Default: LoadLibrary/exports ONLY. Optional --query-factories
 * invokes the original 2006 factories with their exact interface names.
 *
 * This is not an original engine executable and is not a playable game.
 * No original SiN binaries or assets should be copied into this repository.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

typedef void *(__cdecl *SinCreateInterfaceFn)(const char *, int *);
static const char *const grenade_exports[] = {
    "?Detonate@CGrenadeHomer@@UAEXXZ",
    "?Detonate@CGrenadePathfollower@@UAEXXZ",
    "?Detonate@CGrenadeSpitMutant@@UAEXXZ",
    "?Detonate@CGrenadeSpitPoison@@UAEXXZ"
};

static int sibling_path(const wchar_t *name, wchar_t *dest, size_t capacity) {
    DWORD n = GetModuleFileNameW(NULL, dest, (DWORD)capacity);
    wchar_t *sep;
    size_t tail;
    if (n == 0 || (size_t)n >= capacity)
        return 0;
    sep = wcsrchr(dest, L'\\');
    if (sep == NULL)
        return 0;
    tail = wcslen(name);
    if ((size_t)(sep - dest + 1) + tail >= capacity)
        return 0;
    memcpy(sep + 1, name, (tail + 1U) * sizeof(wchar_t));
    return 1;
}

static int probe_module(const wchar_t *module_name,
                        const wchar_t *original_name,
                        const char *interface_name,
                        const char *forbidden_name,
                        int server,
                        int query) {
    wchar_t absolute[4096];
    HMODULE module;
    FARPROC entry;
    size_t k;
    if (!sibling_path(module_name, absolute,
                      sizeof(absolute)/sizeof(absolute[0]))) {
        fprintf(stderr, "FAIL: unable to form executable-adjacent module path\n");
        return 2;
    }
    module = LoadLibraryExW(absolute, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (module == NULL) {
        fprintf(stderr, "FAIL: LoadLibraryExW failed for %ls (Win32=%lu)\n",
                module_name, (unsigned long)GetLastError());
        return 3;
    }
    entry = GetProcAddress(module, "CreateInterface");
    if (entry == NULL) {
        fprintf(stderr, "FAIL: %ls has no undecorated CreateInterface (Win32=%lu)\n",
                module_name, (unsigned long)GetLastError());
        FreeLibrary(module);
        return 4;
    }
    if (server) {
        for (k = 0; k < sizeof(grenade_exports)/sizeof(grenade_exports[0]); ++k) {
            if (GetProcAddress(module, grenade_exports[k]) == NULL) {
                fprintf(stderr, "FAIL: server native decorated export %s unresolved (Win32=%lu)\n",
                        grenade_exports[k], (unsigned long)GetLastError());
                FreeLibrary(module);
                return 5;
            }
        }
    }
    printf("PASS: %ls PE32 module loaded; CreateInterface%s resolved\n",
           module_name, server ? " and 4 grenade forwarders" : "");
    if (query) {
        SinCreateInterfaceFn factory = (SinCreateInterfaceFn)entry;
        int code = 99;
        void *found = factory(interface_name, &code);
        if (found == NULL || code != 0) {
            fprintf(stderr, "FAIL: %ls %s factory returned object=%d status=%d\n",
                    module_name, interface_name, found != NULL, code);
            FreeLibrary(module);
            return 6;
        }
        printf("PASS: %ls %s original factory returned non-null object, status=%d\n",
               module_name, interface_name, code);
        code = 99;
        found = factory(forbidden_name, &code);
        if (found != NULL || code != 1) {
            fprintf(stderr, "FAIL: %ls newer interface %s unexpectedly accepted or status=%d\n",
                    module_name, forbidden_name, code);
            FreeLibrary(module);
            return 7;
        }
        printf("PASS: %ls rejected incompatible newer interface %s\n",
               module_name, forbidden_name);
        if (GetModuleHandleW(original_name) == NULL) {
            fprintf(stderr, "FAIL: owner-original sibling %ls not loaded by proxy\n",
                    original_name);
            FreeLibrary(module);
            return 8;
        }
    }
    FreeLibrary(module);
    return 0;
}

int main(int argc, char **argv) {
    int query = 0;
    int rc;
    if (argc == 2 && strcmp(argv[1], "--query-factories") == 0)
        query = 1;
    else if (argc != 1) {
        fprintf(stderr, "Usage: sin_pc_owner_module_probe.exe [--query-factories]\n");
        fprintf(stderr, "Default is exports-only. --query-factories explicitly calls original SiN factory objects.\n");
        return 1;
    }
    printf("SiN 2006 Win32 x86 original-module boundary probe (%s)\n",
           query ? "factory queries requested" : "exports-only; no factory calls");
    rc = probe_module(L"server.dll", L"sin_original_server.dll",
                      "ServerGameDLL003", "ServerGameDLL010", 1, query);
    if (rc != 0) return rc;
    rc = probe_module(L"client.dll", L"sin_original_client.dll",
                      "VClient011", "VClient017", 0, query);
    if (rc != 0) return rc;
    puts("PASS: original Win32 module-export boundary (not a real game-engine launch)");
    return 0;
}
