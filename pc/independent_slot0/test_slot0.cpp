// Native MSVC x86: LoadLibrary, ABI-correct Init/DLLInit and SDK2004
// candidate slot 1. Does not invoke any real retail/game engine methods.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstdint>

#if !defined(_M_IX86)
#error MSVC Win32 x86 required
#endif

using Factory = void* (__cdecl *)(const char*, int*);

#if defined(SIN_ROLE_client)
constexpr char kName[] = "VClient011";
using Slot0 = int (__thiscall *)(void*, Factory, Factory, void*);
using Slot1 = void (__thiscall *)(void*);
#else
constexpr char kName[] = "ServerGameDLL003";
using Slot0 = bool (__thiscall *)(void*, Factory, Factory, Factory, void*);
using Slot1 = bool (__thiscall *)(void*);
#endif

static int factory_calls = 0;
static void* __cdecl dummy(const char*, int* status) {
    ++factory_calls;
    if (status) *status = 1;
    return nullptr;
}

static bool owned(HMODULE module, const void* address) {
    HMODULE found = nullptr;
    return GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        static_cast<LPCSTR>(address), &found) && found == module;
}

int main(int argc, char** argv) {
    if (argc != 2) return 1;
    HMODULE dll = LoadLibraryA(argv[1]);
    if (!dll) {
        std::fprintf(stderr, "LoadLibrary failed %lu\n", GetLastError());
        return 2;
    }

    auto exported = GetProcAddress(dll, "CreateInterface");
    if (!exported || !owned(dll, reinterpret_cast<const void*>(exported)))
        return 3;
    auto factory = reinterpret_cast<Factory>(exported);
    int status = -1;
    void* object = factory(kName, &status);
    if (!object || status != 0 || factory(kName, nullptr) != object) return 4;
    const char* unknown =
#if defined(SIN_ROLE_client)
        "ServerGameDLL003";
#else
        "VClient011";
#endif
    status = -1;
    if (factory(unknown, &status) != nullptr || status != 1) return 5;
    status = -1;
    if (factory(nullptr, &status) != nullptr || status != 1) return 6;

    auto table = *reinterpret_cast<void***>(object);
    if (!table || !owned(dll, table) || !owned(dll, table[0]) ||
        !owned(dll, table[1]) || table[0] == table[1])
        return 7;

    // The third CLIENT argument is a noncallable engine-global pointer.
    // The test deliberately passes data rather than a function pointer.
    unsigned long fakeGlobals = 0x53494E45UL;
    void* globals = &fakeGlobals;
    auto init = reinterpret_cast<Slot0>(table[0]);
    std::uintptr_t before = 0, after = 0;
    int result = -1;

#if defined(SIN_ROLE_client)
    // SDK 2004 candidate: int Init(factory, physicsFactory, globals*);
    // verified SiN x86 cleanup: RET 12
    __asm {
        mov eax, esp
        mov before, eax
        push globals
        push offset dummy
        push offset dummy
        mov ecx, object
        mov eax, init
        call eax
        mov result, eax
        mov eax, esp
        mov after, eax
    }
#else
    // SDK 2004 candidate: bool DLLInit(engine, physics, filesystem, globals*);
    // verified SiN x86 cleanup: RET 16
    __asm {
        mov eax, esp
        mov before, eax
        push globals
        push offset dummy
        push offset dummy
        push offset dummy
        mov ecx, object
        mov eax, init
        call eax
        movzx eax, al
        mov result, eax
        mov eax, esp
        mov after, eax
    }
#endif
    if (before != after || result != 0 || factory_calls != 0)
        return 8;

    auto method1 = reinterpret_cast<Slot1>(table[1]);
    // Slot one is an independently implemented fail-closed cleanup / game
    // method. Test no extra stack arguments and no callback invocation.
    std::uintptr_t before1 = 0, after1 = 0;
#if defined(SIN_ROLE_client)
    __asm {
        mov eax, esp
        mov before1, eax
        mov ecx, object
        mov eax, method1
        call eax
        mov eax, esp
        mov after1, eax
    }
#else
    int gameStarted = -1;
    __asm {
        mov eax, esp
        mov before1, eax
        mov ecx, object
        mov eax, method1
        call eax
        movzx eax, al
        mov gameStarted, eax
        mov eax, esp
        mov after1, eax
    }
    if (gameStarted != 0) return 10;
#endif
    if (before1 != after1 || factory_calls != 0) return 11;

    if (!FreeLibrary(dll)) return 9;
    std::puts("PASS independent PE32 SDK2004 candidate: factory, Init/DLLInit globals pointer, slot1, x86 thiscall, fail-closed");
    return 0;
}
