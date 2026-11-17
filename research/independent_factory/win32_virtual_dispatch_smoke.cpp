// Load an independently built Win32 x86 DLL and CALL virtual methods via
// genuine MSVC __thiscall dispatch, verifying method ownership and lifetime.
// This is NOT a retail-engine or original SiN virtual-table compatibility test.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "virtual_interface_contract.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>

using Factory = void* (__cdecl *)(const char*, int*);
using Interface = sin_synthetic_x86::ICanary;

static_assert(sizeof(void*) == 4u, "The ABI smoke must use Win32 PE32");

static int fail(const char* reason, HMODULE module = nullptr) {
    std::fprintf(stderr, "FAIL synthetic x86 virtual dispatch: %s (win32=%lu)\n",
                 reason, static_cast<unsigned long>(GetLastError()));
    if (module != nullptr) FreeLibrary(module);
    return 1;
}

static bool owned_by(HMODULE expected, const void* address) {
    HMODULE owner = nullptr;
    return GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                              GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                              static_cast<LPCSTR>(address), &owner) != 0 &&
           owner == expected;
}

int main(int argc, char** argv) {
    if (argc != 2) return fail("expected path to synthetic DLL");
    for (int cycle = 0; cycle < 16; ++cycle) {
        HMODULE module = LoadLibraryA(argv[1]);
        if (!module) return fail("LoadLibraryA");
        FARPROC address = GetProcAddress(module, "CreateInterface");
        if (!address) return fail("undecorated CreateInterface export", module);
        if (!owned_by(module, reinterpret_cast<const void*>(address)))
            return fail("factory points outside its DLL", module);
        auto factory = reinterpret_cast<Factory>(address);
        int status = -99;
        void* raw = factory(sin_synthetic_x86::kInterfaceName, &status);
        if (raw == nullptr || status != 0)
            return fail("synthetic factory negotiation", module);
        if (factory(sin_synthetic_x86::kInterfaceName, nullptr) != raw)
            return fail("synthetic object not stable", module);
        status = -99;
        if (factory("VClient011", &status) != nullptr || status != 1)
            return fail("incorrectly returned retail-like interface name", module);
        status = -99;
        if (factory(nullptr, &status) != nullptr || status != 1)
            return fail("incorrectly accepted null name", module);

        auto* object = static_cast<Interface*>(raw);
        // Inspect exactly three deliberately defined synthetic virtual slots.
        // This check detects a plain sentinel object masquerading as a vtable.
        void* const* const vtable = *reinterpret_cast<void* const* const*>(raw);
        if (!vtable || !owned_by(module, vtable) ||
            !owned_by(module, vtable[0]) ||
            !owned_by(module, vtable[1]) ||
            !owned_by(module, vtable[2]))
            return fail("vtable or methods not owned by independent DLL", module);

        if (object->signature() != sin_synthetic_x86::kSignature)
            return fail("virtual slot 2 thiscall signature", module);
        if (object->read_value() != 13)
            return fail("virtual slot 0 thiscall initial state", module);
        if (object->add_value(7) != 20 || object->read_value() != 20)
            return fail("virtual slot 1 thiscall argument/register dispatch", module);
        if (object->add_value(-11) != 9 || object->read_value() != 9)
            return fail("negative argument or object state incorrect", module);

        HMODULE duplicate = LoadLibraryA(argv[1]);
        if (!duplicate || duplicate != module)
            return fail("second load or module identity", module);
        if (!FreeLibrary(duplicate))
            return fail("second FreeLibrary", module);
        if (object->add_value(4) != 13)
            return fail("virtual object invalidated after partial unload", module);
        if (!FreeLibrary(module)) return fail("final FreeLibrary");
    }
    std::puts("PASS synthetic Win32 x86: independent DLL 3-slot MSVC thiscall vtable, 16 reloads");
    return 0;
}
