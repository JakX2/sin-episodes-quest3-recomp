// Native MSVC Win32 x86 independent DLL loader and SDK2004 first dependency
// preflight (synthetic engine factories only, no original game binaries).
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstdint>
#include <cstring>

#if !defined(_M_IX86)
#error MSVC Win32 x86 required
#endif

using Factory = void* (__cdecl *)(const char*, int*);
#if defined(SIN_ROLE_client)
constexpr char kName[] = "VClient011";
constexpr const char* kExpected[] = {"VEngineClient012", "VEngineModel012"};
using Slot0 = int (__thiscall *)(void*, Factory, Factory, void*);
using Slot1 = void (__thiscall *)(void*);
#else
constexpr char kName[] = "ServerGameDLL003";
constexpr const char* kExpected[] = {"VEngineServer021", "VoiceServer001", "VEngineCvar002"};
using Slot0 = bool (__thiscall *)(void*, Factory, Factory, Factory, void*);
using Slot1 = bool (__thiscall *)(void*);
#endif

constexpr int kQueries = static_cast<int>(sizeof(kExpected)/sizeof(kExpected[0]));
static int g_callCount = 0;
static int g_availableCount = 0;
static int g_invalid = 0;
static int g_unexpectedFactory = 0;
static unsigned long g_fakeGlobals = 0x53494E45UL;

static void* __cdecl expectedFactory(const char* name, int* status) {
    // The SDK2004 initializer passes NULL as the optional status pointer.
    if (g_callCount >= kQueries || name == nullptr ||
        std::strcmp(name, kExpected[g_callCount]) != 0 || status != nullptr) {
        ++g_invalid;
    }
    const bool found = g_callCount < g_availableCount;
    ++g_callCount;
    return found ? static_cast<void*>(&g_fakeGlobals) : nullptr;
}
static void* __cdecl unusedFactory(const char*, int*) {
    ++g_unexpectedFactory;
    return nullptr;
}
static bool owned(HMODULE module, const void* address) {
    HMODULE found = nullptr;
    return GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        static_cast<LPCSTR>(address), &found) && found == module;
}

static bool invokeInit(void* object, void* function, bool validGlobals) {
    void* globalData = validGlobals ? static_cast<void*>(&g_fakeGlobals) : nullptr;
    Factory app = &expectedFactory;
    Factory other = &unusedFactory;
    std::uintptr_t before = 0, after = 0;
    int result = -1;
#if defined(SIN_ROLE_client)
    __asm {
        mov eax, esp
        mov before, eax
        push globalData
        push other
        push app
        mov ecx, object
        mov eax, function
        call eax
        mov result, eax
        mov eax, esp
        mov after, eax
    }
#else
    __asm {
        mov eax, esp
        mov before, eax
        push globalData
        push other
        push other
        push app
        mov ecx, object
        mov eax, function
        call eax
        movzx eax, al
        mov result, eax
        mov eax, esp
        mov after, eax
    }
#endif
    return before == after && result == 0;
}
static bool invokeSecond(void* object, void* function) {
    std::uintptr_t before = 0, after = 0;
#if defined(SIN_ROLE_client)
    __asm {
        mov eax, esp
        mov before, eax
        mov ecx, object
        mov eax, function
        call eax
        mov eax, esp
        mov after, eax
    }
    return before == after;
#else
    int result = -1;
    __asm {
        mov eax, esp
        mov before, eax
        mov ecx, object
        mov eax, function
        call eax
        movzx eax, al
        mov result, eax
        mov eax, esp
        mov after, eax
    }
    return before == after && result == 0;
#endif
}
static bool getLogPath(char* path, size_t capacity) {
    char temp[MAX_PATH] = {};
    DWORD used = GetTempPathA(MAX_PATH, temp);
    if (used == 0 || used >= MAX_PATH) return false;
    char session[33] = {};
    DWORD sessionLength =
        GetEnvironmentVariableA("SIN_INDEPENDENT_TRACE_SESSION", session,
                                static_cast<DWORD>(sizeof(session)));
    if (sessionLength && sessionLength != 32) return false;
    int written = sessionLength == 32
        ? std::snprintf(path, capacity, "%ssin-independent-%s-%lu.log",
                        temp, session,
                        static_cast<unsigned long>(GetCurrentProcessId()))
        : std::snprintf(path, capacity, "%ssin-independent-%lu.log",
                        temp, static_cast<unsigned long>(GetCurrentProcessId()));
    return written > 0 && static_cast<size_t>(written) < capacity;
}
static bool logContains(const char* path, const char* needle) {
    FILE* file = nullptr;
    if (fopen_s(&file, path, "rb") != 0 || !file) return false;
    char buffer[8192] = {};
    size_t count = std::fread(buffer, 1, sizeof(buffer)-1, file);
    std::fclose(file);
    buffer[count] = '\0';
    if (std::strstr(buffer, needle)) return true;
    std::fprintf(stderr, "missing diagnostic event: %s\n", needle);
    return false;
}
static int fail(int code, const char* message) {
    std::fprintf(stderr, "FAIL independent preflight %d: %s\n", code, message);
    return code;
}

int main(int argc, char** argv) {
    if (argc != 2) return fail(1, "expected path");
    // Keep the existing unscoped synthetic diagnostics deterministic even
    // when invoked by an owner terminal with a session variable set.
    if (!SetEnvironmentVariableA("SIN_INDEPENDENT_TRACE_SESSION", nullptr))
        return fail(24, "clear inherited session");
    if (!SetEnvironmentVariableA("SIN_INDEPENDENT_TRACE_FILE", "0"))
        return fail(34, "disable inherited traces before factory checks");
    HMODULE dll = LoadLibraryA(argv[1]);
    if (!dll) return fail(2, "LoadLibrary");
    auto exported = GetProcAddress(dll, "CreateInterface");
    if (!exported || !owned(dll, reinterpret_cast<const void*>(exported)))
        return fail(3, "CreateInterface export not DLL-owned");
    auto create = reinterpret_cast<Factory>(exported);
    int status = -1;
    void* object = create(kName, &status);
    if (!object || status != 0 || create(kName, nullptr) != object)
        return fail(4, "retail factory negotiation");
#if defined(SIN_ROLE_client)
    constexpr char otherName[] = "ServerGameDLL003";
#else
    constexpr char otherName[] = "VClient011";
#endif
    status = -1;
    if (create(otherName, &status) != nullptr || status != 1)
        return fail(5, "wrong cross-module interface accepted");
    status = -1;
    if (create(nullptr, &status) != nullptr || status != 1)
        return fail(6, "null name accepted");
    auto slots = *reinterpret_cast<void***>(object);
    if (!slots || !owned(dll, slots) || !owned(dll, slots[0]) ||
        !owned(dll, slots[1]) || slots[0] == slots[1])
        return fail(7, "two DLL-owned native virtual methods");

    // Strict off-by-default: diagnostic log is never produced by ordinary
    // native factory tests, avoiding side effects in normal host sessions.
    char path[MAX_PATH] = {};
    if (!getLogPath(path, sizeof(path))) return fail(8, "temporary trace path");
    if (!SetEnvironmentVariableA("SIN_INDEPENDENT_TRACE_FILE", "0"))
        return fail(9, "disable file traces");
    (void)DeleteFileA(path);
    g_callCount = g_invalid = g_unexpectedFactory = 0;
    g_availableCount = 0;
    if (!invokeInit(object, slots[0], true) || g_callCount != 1 ||
        g_invalid || g_unexpectedFactory)
        return fail(10, "missing first interface should stop after one query");
    if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES)
        return fail(11, "trace written even though tracing is disabled");

    if (!SetEnvironmentVariableA("SIN_INDEPENDENT_TRACE_FILE", "1"))
        return fail(12, "enable diagnostic traces");
    g_callCount = 0;
    g_availableCount = 1;
    if (!invokeInit(object, slots[0], true) || g_callCount != 2 ||
        g_invalid || g_unexpectedFactory)
        return fail(13, "missing second interface should stop after two queries");
#if defined(SIN_ROLE_server)
    // The third server callback (engine cvar) must also be observed as a
    // distinct startup blocker when the earlier two interfaces are present.
    g_callCount = 0;
    g_availableCount = 2;
    if (!invokeInit(object, slots[0], true) || g_callCount != 3 ||
        g_invalid || g_unexpectedFactory)
        return fail(22, "third server interface must be probed exactly once");
#endif
    g_callCount = 0;
    g_availableCount = kQueries;
    if (!invokeInit(object, slots[0], true) ||
        g_callCount != kQueries || g_invalid || g_unexpectedFactory)
        return fail(14, "all available still must fail closed");
    g_callCount = 0;
    if (!invokeInit(object, slots[0], false) || g_callCount != 0 ||
        g_invalid || g_unexpectedFactory)
        return fail(15, "invalid globals must refuse without factory calls");

    if (!invokeSecond(object, slots[1]) || g_unexpectedFactory)
        return fail(16, "second virtual slot must fail closed/no factory calls");
    if (!logContains(path, "phase=missing-interface") ||
        !logContains(path, "phase=found-interface") ||
        !logContains(path, "phase=sdk2004-preflight-only") ||
        !logContains(path, "phase=invalid-init-arguments"))
        return fail(17, "missing trace phase evidence");
    for (int i = 0; i < kQueries; ++i)
        if (!logContains(path, kExpected[i]))
            return fail(18, "missing expected SDK2004 version request evidence");
#if defined(SIN_ROLE_server)
    if (!logContains(path,
                     "phase=missing-interface version=VEngineCvar002"))
        return fail(23, "third server dependency has no specific missing log");
#endif
    constexpr char kSession[] = "0123456789abcdef0123456789abcdef";
    if (!SetEnvironmentVariableA("SIN_INDEPENDENT_TRACE_SESSION", kSession))
        return fail(25, "enable scoped trace session");
    char sessionPath[MAX_PATH] = {};
    if (!getLogPath(sessionPath, sizeof(sessionPath)))
        return fail(26, "scoped diagnostic path");
    (void)DeleteFileA(sessionPath);
    status = -1;
    if (create(kName, &status) != object || status != 0)
        return fail(31, "scoped expected factory result");
    status = -1;
    if (create(otherName, &status) != nullptr || status != 1)
        return fail(32, "scoped rejected factory result");
    if (!invokeSecond(object, slots[1]))
        return fail(27, "scoped second virtual slot call");
    if (!logContains(sessionPath, "role="))
        return fail(28, "missing run-scoped trace");
    if (!logContains(sessionPath, "phase=factory-accepted version=") ||
        !logContains(sessionPath, kName) ||
        !logContains(sessionPath, "phase=factory-rejected version=") ||
        !logContains(sessionPath, otherName))
        return fail(33, "missing native CreateInterface negotiation traces");
    if (!DeleteFileA(sessionPath))
        return fail(29, "remove run-scoped trace");
    if (!SetEnvironmentVariableA("SIN_INDEPENDENT_TRACE_SESSION", nullptr))
        return fail(30, "clear run-scoped trace session");
    if (!SetEnvironmentVariableA("SIN_INDEPENDENT_TRACE_FILE", "0"))
        return fail(19, "disable tracing after test");
    if (!DeleteFileA(path)) return fail(20, "clean synthetic diagnostic trace");
    if (!FreeLibrary(dll)) return fail(21, "FreeLibrary");
    std::puts("PASS independent PE32: x86 factory negotiation + SDK2004 bounded preflight, session traces, fail-closed");
    return 0;
}
