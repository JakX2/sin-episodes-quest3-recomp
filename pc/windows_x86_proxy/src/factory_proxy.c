/*
 * Windows x86 PC engine-loading compatibility seam.
 *
 * Exports the real SiN 2006 factory name and delegates to an owner-provided
 * original DLL in the same directory. This deliberately does not fabricate
 * an IServerGameDLL / IBaseClientDLL vtable or pretend to rebuild gameplay.
 * Never call LoadLibrary from DllMain.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#if defined(SIN_PC_PROXY_SERVER) == defined(SIN_PC_PROXY_CLIENT)
#error "Specify exactly one PC proxy role"
#endif

#if defined(SIN_PC_PROXY_SERVER)
#define SIN_ORIGINAL_DLL L"sin_original_server.dll"
#define SIN_ROLE_NAME "server"
#else
#define SIN_ORIGINAL_DLL L"sin_original_client.dll"
#define SIN_ROLE_NAME "client"
#endif

typedef void *(__cdecl *SinCreateInterfaceFn)(const char *, int *);
static HMODULE sin_self = NULL;
static HMODULE sin_original = NULL;
static SinCreateInterfaceFn sin_original_factory = NULL;
static INIT_ONCE sin_initialize_once = INIT_ONCE_STATIC_INIT;
static void sin_trace_event(const char *phase, const char *name, DWORD detail);

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        sin_self = (HMODULE)instance;
        DisableThreadLibraryCalls(instance);
    }
    return TRUE;
}

static BOOL CALLBACK sin_initialize_original(PINIT_ONCE once, PVOID parameter,
                                              PVOID *context) {
    wchar_t dll_path[4096];
    wchar_t *separator;
    const wchar_t *original_name = SIN_ORIGINAL_DLL;
    DWORD length;
    size_t available, name_length;
    FARPROC entry;

    (void)once;
    (void)parameter;
    (void)context;

    length = GetModuleFileNameW(sin_self, dll_path,
                                (DWORD)(sizeof(dll_path) / sizeof(dll_path[0])));
    if (length == 0 || length >= (DWORD)(sizeof(dll_path) / sizeof(dll_path[0])))
        return FALSE;
    separator = wcsrchr(dll_path, L'\\');
    if (separator == NULL)
        return FALSE;
    name_length = wcslen(original_name);
    available = (size_t)((dll_path +
        (sizeof(dll_path) / sizeof(dll_path[0]))) - (separator + 1));
    if (available <= name_length)
        return FALSE;
    memcpy(separator + 1, original_name,
           (name_length + 1U) * sizeof(wchar_t));

    /* Absolute, sibling-file path avoids recursive loading of our own DLL.
     * LOAD_WITH_ALTERED_SEARCH_PATH keeps the original's dependencies local.
     */
    sin_original = LoadLibraryExW(dll_path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (sin_original == NULL) {
        sin_trace_event("original-load-failed", NULL, GetLastError());
        return FALSE;
    }
    entry = GetProcAddress(sin_original, "CreateInterface");
    if (entry == NULL) {
        sin_trace_event("original-factory-export-missing", NULL, GetLastError());
        return FALSE;
    }
    sin_original_factory = (SinCreateInterfaceFn)entry;
    sin_trace_event("original-load-ok", NULL, 0U);
    return TRUE;
}

/* Debug-output-only trace: no file writes or factory ABI changes.
 * 'detail' is Win32 GetLastError for load failures and factory return
 * code for results. A NULL result is always reported as 'result-null'.
 */
static void sin_trace_event(const char *phase, const char *name, DWORD detail) {
    char trace[2] = {0, 0};
    char line[420];
    DWORD n = GetEnvironmentVariableA("SIN_PC_PROXY_TRACE", trace,
                                      (DWORD)sizeof(trace));
    if (n != 1 || trace[0] != '1')
        return;
    (void)snprintf(line, sizeof(line),
                   "[SiN PC proxy] " SIN_ROLE_NAME " %s name=%.240s detail=%lu\\n",
                   phase, name != NULL ? name : "<none>",
                   (unsigned long)detail);
    OutputDebugStringA(line);
}

/* Exported with the undecorated name CreateInterface via the .def file.
 * Return values and success/failure codes belong to the actual original.
 */
void *__cdecl CreateInterface(const char *name, int *return_code) {
    void *instance;
    if (return_code != NULL)
        *return_code = 1; /* IFACE_FAILED */
    if (name == NULL) {
        sin_trace_event("null-factory-name", NULL, 1U);
        return NULL;
    }
    sin_trace_event("request", name, 0U);
    if (!InitOnceExecuteOnce(&sin_initialize_once,
                            sin_initialize_original, NULL, NULL)) {
        sin_trace_event("original-init-failed", name, 1U);
        return NULL;
    }
    if (sin_original_factory == NULL) {
        sin_trace_event("original-factory-unavailable", name, 1U);
        return NULL;
    }
    instance = sin_original_factory(name, return_code);
    sin_trace_event(instance != NULL ? "result-object" : "result-null",
                    name, return_code != NULL ? (DWORD)(*return_code) : 0xffffffffU);
    return instance;
}
