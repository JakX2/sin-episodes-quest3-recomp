/*
 * Real MSVC Win32 x86 test of the opt-in owner startup factory trace.
 * Synthetic original DLLs only. No retail game files or method calls.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

typedef void *(__cdecl *Factory)(const char *, int *);

static int trace_path(wchar_t *out, size_t capacity) {
    wchar_t leaf[80];
    DWORD n = GetTempPathW((DWORD)capacity, out);
    int length;
    size_t used;
    if (n == 0 || (size_t)n >= capacity)
        return 0;
    length = swprintf(leaf, sizeof(leaf)/sizeof(leaf[0]),
                     L"sin-pc-proxy-%lu.log",
                     (unsigned long)GetCurrentProcessId());
    if (length <= 0 || (size_t)length >= sizeof(leaf)/sizeof(leaf[0]))
        return 0;
    used = wcslen(out);
    if (used + (size_t)length >= capacity)
        return 0;
    memcpy(out + used, leaf, ((size_t)length+1U)*sizeof(wchar_t));
    return 1;
}

static int test_factory(const wchar_t *dll_name, const char *accepted,
                        const char *rejected) {
    HMODULE dll = LoadLibraryW(dll_name);
    Factory factory;
    int code = -11;
    void *found;
    if (dll == NULL) return 2;
    factory = (Factory)GetProcAddress(dll, "CreateInterface");
    if (factory == NULL) return 3;
    found = factory(accepted, &code);
    if (found == NULL || code != 0) return 4;
    code = -11;
    if (factory(rejected, &code) != NULL || code != 1) return 5;
    code = -11;
    if (factory(NULL, &code) != NULL || code != 1) return 6;
    if (factory(accepted, NULL) != found) return 7;
    if (!FreeLibrary(dll)) return 8;
    return 0;
}

static int contains(const char *text, const char *expected) {
    if (strstr(text, expected) != NULL)
        return 1;
    fprintf(stderr, "Missing file-trace event: %s\n", expected);
    return 0;
}

int main(void) {
    wchar_t path[MAX_PATH];
    char log[32768];
    HANDLE file;
    DWORD bytes = 0, length, error;
    int rc;
    if (!trace_path(path, sizeof(path)/sizeof(path[0])))
        return 9;
    if (!SetEnvironmentVariableA("SIN_PC_PROXY_TRACE_FILE", "0"))
        return 10;
    /* No false positives from another execution: only delete this PID file. */
    if (!DeleteFileW(path)) {
        error = GetLastError();
        if (error != ERROR_FILE_NOT_FOUND) return 11;
    }
    rc = test_factory(L".\\server.dll", "ServerGameDLL003", "ServerGameDLL999");
    if (rc != 0) return 20 + rc;
    if (GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES) {
        fprintf(stderr, "Factory tracing must be opt-in\n");
        return 12;
    }
    if (!SetEnvironmentVariableA("SIN_PC_PROXY_TRACE_FILE", "1"))
        return 13;
    rc = test_factory(L".\\server.dll", "ServerGameDLL003", "ServerGameDLL999");
    if (rc != 0) return 30 + rc;
    rc = test_factory(L".\\client.dll", "VClient011", "VClient017");
    if (rc != 0) return 40 + rc;
    if (!SetEnvironmentVariableA("SIN_PC_PROXY_TRACE_FILE", "0"))
        return 14;
    file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return 15;
    length = GetFileSize(file, NULL);
    if (length == INVALID_FILE_SIZE || length == 0 || length >= sizeof(log)) {
        CloseHandle(file);
        return 16;
    }
    if (!ReadFile(file, log, length, &bytes, NULL) || bytes != length) {
        CloseHandle(file);
        return 17;
    }
    CloseHandle(file);
    log[bytes] = '\0';
    if (!contains(log, "role=server phase=request name=ServerGameDLL003 detail=0") ||
        !contains(log, "role=server phase=result-object name=ServerGameDLL003 detail=0") ||
        !contains(log, "role=server phase=result-null name=ServerGameDLL999 detail=1") ||
        !contains(log, "role=server phase=null-factory-name name=<none> detail=1") ||
        !contains(log, "role=client phase=original-load-ok name=<none> detail=0") ||
        !contains(log, "role=client phase=request name=VClient011 detail=0") ||
        !contains(log, "role=client phase=result-object name=VClient011 detail=0") ||
        !contains(log, "role=client phase=result-null name=VClient017 detail=1")) {
        DeleteFileW(path);
        return 18;
    }
    if (!DeleteFileW(path)) return 19;
    puts("PASS native Win32 x86: owner startup trace is opt-in, both factory roles record status without original method invocation");
    return 0;
}
