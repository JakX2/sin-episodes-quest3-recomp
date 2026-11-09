#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>

typedef void *(__cdecl *SinFactoryFn)(const char *, int *);

static const char *const server_det_exports[] = {
    "?Detonate@CGrenadeHomer@@UAEXXZ",
    "?Detonate@CGrenadePathfollower@@UAEXXZ",
    "?Detonate@CGrenadeSpitMutant@@UAEXXZ",
    "?Detonate@CGrenadeSpitPoison@@UAEXXZ"
};

static int run_case(const wchar_t *proxy_path, const wchar_t *original_name,
                    const char *expected_name, const char *other_name,
                    int is_server) {
    HMODULE proxy = LoadLibraryW(proxy_path);
    HMODULE original;
    SinFactoryFn proxy_factory, original_factory;
    void *actual, *expected;
    int result_code, original_code;
    size_t i;

    if (proxy == NULL) {
        fprintf(stderr, "proxy LoadLibrary failed: %lu\n", (unsigned long)GetLastError());
        return 1;
    }
    proxy_factory = (SinFactoryFn)GetProcAddress(proxy, "CreateInterface");
    if (proxy_factory == NULL) {
        fprintf(stderr, "missing undecorated CreateInterface export\n");
        return 2;
    }

    /* First call exercises load-by-absolute-sibling-path inside the proxy. */
    result_code = 99;
    actual = proxy_factory(expected_name, &result_code);
    if (actual == NULL || result_code != 0) {
        fprintf(stderr, "proxy failed original factory lookup: %d\n", result_code);
        return 3;
    }
    original = GetModuleHandleW(original_name);
    if (original == NULL) {
        fprintf(stderr, "proxy did not load the expected original sibling\n");
        return 4;
    }
    original_factory = (SinFactoryFn)GetProcAddress(original, "CreateInterface");
    if (original_factory == NULL)
        return 5;
    original_code = 99;
    expected = original_factory(expected_name, &original_code);
    if (actual != expected || result_code != original_code)
        return 6;

    /* Unknown requests cannot silently alias a newer SourceVR interface. */
    result_code = 99;
    if (proxy_factory(other_name, &result_code) != NULL || result_code != 1)
        return 7;
    result_code = 99;
    if (proxy_factory(NULL, &result_code) != NULL || result_code != 1)
        return 8;
    if (proxy_factory(expected_name, NULL) != expected)
        return 9;

    if (is_server) {
        for (i = 0; i < sizeof(server_det_exports) / sizeof(server_det_exports[0]); ++i) {
            FARPROC forwarded = GetProcAddress(proxy, server_det_exports[i]);
            FARPROC direct = GetProcAddress(original, server_det_exports[i]);
            if (forwarded == NULL || direct == NULL || forwarded != direct) {
                fprintf(stderr, "failed original thiscall export forwarding: %s\n",
                        server_det_exports[i]);
                return 10;
            }
        }
    }
    FreeLibrary(proxy);
    return 0;
}

int main(void) {
    int rc = run_case(L".\\server.dll", L"sin_original_server.dll",
                      "ServerGameDLL003", "ServerGameDLL010", 1);
    if (rc != 0)
        return rc;
    rc = run_case(L".\\client.dll", L"sin_original_client.dll",
                  "VClient011", "VClient017", 0);
    if (rc != 0)
        return rc;
    puts("PASS: PE32 Win32 DLL CreateInterface delegates exact original 2006 APIs; server grenade exports forward unchanged");
    return 0;
}
