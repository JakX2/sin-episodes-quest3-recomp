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

/* Both trace channels are explicitly opt-in and never modify factory
 * behavior. File tracing is useful for real engine launches where no debugger
 * is attached, unlike OutputDebugStringA alone.
 *
 * The log is always in the user's Win32 TEMP directory, named
 * sin-pc-proxy-<PID>.log. No arbitrary path or original binary bytes are read
 * from the environment. No logging is performed in DllMain.
 */
static int sin_trace_enabled(const char *variable) {
    char value[2] = {0, 0};
    DWORD n = GetEnvironmentVariableA(variable, value, (DWORD)sizeof(value));
    return n == 1 && value[0] == '1';
}

static void sin_trace_file(const char *line) {
    wchar_t path[MAX_PATH];
    wchar_t leaf[80];
    DWORD n = GetTempPathW((DWORD)(sizeof(path) / sizeof(path[0])), path);
    size_t used, extra;
    int formatted;
    HANDLE file;
    DWORD written;
    if (n == 0 || n >= (DWORD)(sizeof(path) / sizeof(path[0])))
        return;
    formatted = swprintf(leaf, sizeof(leaf) / sizeof(leaf[0]),
                         L"sin-pc-proxy-%lu.log",
                         (unsigned long)GetCurrentProcessId());
    if (formatted <= 0 || (size_t)formatted >= sizeof(leaf)/sizeof(leaf[0]))
        return;
    used = wcslen(path);
    extra = wcslen(leaf);
    if (used + extra >= sizeof(path)/sizeof(path[0]))
        return;
    memcpy(path + used, leaf, (extra + 1U) * sizeof(wchar_t));
    file = CreateFileW(path, FILE_APPEND_DATA,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return;
    (void)WriteFile(file, line, (DWORD)strlen(line), &written, NULL);
    CloseHandle(file);
}

/* 'detail' is Win32 GetLastError for load failures or the CreateInterface
 * return status. No original function pointers, addresses or filesystem paths
 * are written. Raw interface names are sanitized to prevent forged log lines.
 */
static void sin_trace_event(const char *phase, const char *name, DWORD detail) {
    char safe_name[121];
    char line[360];
    size_t i;
    int debug_enabled = sin_trace_enabled("SIN_PC_PROXY_TRACE");
    int file_enabled = sin_trace_enabled("SIN_PC_PROXY_TRACE_FILE");
    if (!debug_enabled && !file_enabled)
        return;
    if (name == NULL)
        name = "<none>";
    for (i = 0; i < sizeof(safe_name)-1U && name[i] != '\0'; ++i) {
        unsigned char ch = (unsigned char)name[i];
        safe_name[i] = (ch >= 32U && ch <= 126U) ? (char)ch : '?';
    }
    safe_name[i] = '\0';
    (void)snprintf(line, sizeof(line),
                   "[SiN PC proxy] role=%s phase=%s name=%s detail=%lu\n",
                   SIN_ROLE_NAME, phase, safe_name, (unsigned long)detail);
    if (debug_enabled)
        OutputDebugStringA(line);
    if (file_enabled)
        sin_trace_file(line);
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
