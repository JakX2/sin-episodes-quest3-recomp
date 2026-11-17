// Real MSVC Win32 x86 lifecycle and callback ABI test, synthetic only.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "lifecycle_contract.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
using namespace sin_synthetic_lifecycle;
using Factory = void* (__cdecl *)(const char*, int*);
struct Recorder {
    std::uint32_t attached = 0;
    std::uint32_t detached = 0;
    std::uint32_t invalid = 0;
    IRuntime* runtime = nullptr;
};
static void __cdecl notify(void* context, std::uint32_t event) {
    auto& rec = *static_cast<Recorder*>(context);
    // Reentrant vtable calls must observe the committed lifecycle state.
    if (event == kAttach) {
        ++rec.attached;
        if (!rec.runtime->initialized()) ++rec.invalid;
    } else if (event == kDetach) {
        ++rec.detached;
        if (rec.runtime->initialized()) ++rec.invalid;
    } else ++rec.invalid;
}
static int fail(const char* msg, HMODULE dll = nullptr) {
    std::fprintf(stderr, "FAIL synthetic lifecycle: %s (win32=%lu)\n",
                 msg, static_cast<unsigned long>(GetLastError()));
    if (dll) FreeLibrary(dll);
    return 1;
}
static bool belongs(HMODULE expected, const void* address) {
    HMODULE actual = nullptr;
    return GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        static_cast<LPCSTR>(address), &actual) && actual == expected;
}
int main(int argc, char** argv) {
    if (argc != 2) return fail("expected independent DLL path");
    for (int cycle = 0; cycle < 16; ++cycle) {
        HMODULE dll = LoadLibraryA(argv[1]);
        if (!dll) return fail("LoadLibrary");
        auto address = GetProcAddress(dll, "CreateInterface");
        if (!address || !belongs(dll, reinterpret_cast<const void*>(address)))
            return fail("factory export ownership", dll);
        auto factory = reinterpret_cast<Factory>(address);
        int status = -7;
        auto* api = static_cast<IRuntime*>(factory(kName, &status));
        if (!api || status != 0 || factory(kName, nullptr) != api)
            return fail("factory negotiation or singleton identity", dll);
        status = -7;
        if (factory("ServerGameDLL003", &status) || status != 1)
            return fail("retail interface name incorrectly accepted", dll);
        status = -7;
        if (factory(nullptr, &status) || status != 1)
            return fail("null interface name accepted", dll);
        // A synthetic DLL must never advertise retail engine interfaces.
        // Failed lookups must not disturb the valid singleton or success status.
        constexpr const char* retail_names[] = {
            "VClient011", "ServerGameDLL003",
            "VClient017", "ServerGameDLL005"
        };
        for (const char* retail : retail_names) {
            status = -7;
            if (factory(retail, &status) != nullptr || status != 1 ||
                factory(kName, &status) != api || status != 0)
                return fail("retail ABI rejection or singleton corruption", dll);
        }
        auto* const* slots = *reinterpret_cast<void* const* const*>(api);
        if (!slots || !belongs(dll, slots) ||
            !belongs(dll, slots[0]) || !belongs(dll, slots[1]) ||
            !belongs(dll, slots[2]) || !belongs(dll, slots[3]))
            return fail("vtable or four virtual methods not DLL-owned", dll);
        Recorder rec{};
        rec.runtime = api;
        HostCallbacks valid{sizeof(HostCallbacks), &notify, &rec};
        HostCallbacks invalid{sizeof(HostCallbacks)-4u, &notify, &rec};
        if (api->initialized() || api->generation() != 0u ||
            api->initialize(nullptr) || api->initialize(&invalid) ||
            api->initialized() || api->generation() != 0u)
            return fail("invalid initialization must fail without side effects", dll);
        HostCallbacks null_notify{sizeof(HostCallbacks), nullptr, &rec};
        HostCallbacks null_context{sizeof(HostCallbacks), &notify, nullptr};
        if (api->initialize(&null_notify) || api->initialize(&null_context) ||
            api->initialized() || api->generation() != 0u ||
            rec.attached || rec.detached || rec.invalid)
            return fail("invalid callback pointers must be side-effect free", dll);
        if (!api->initialize(&valid) || !api->initialized() ||
            api->generation() != 1u || rec.attached != 1u || rec.invalid)
            return fail("initialization callback or state", dll);
        if (api->initialize(&valid) || rec.attached != 1u ||
            api->generation() != 1u)
            return fail("double initialization must be rejected", dll);
        HMODULE second = LoadLibraryA(argv[1]);
        if (!second || second != dll) return fail("duplicate load", dll);
        if (!FreeLibrary(second)) return fail("partial unload", dll);
        if (!api->initialized()) return fail("state lost after partial unload", dll);
        api->shutdown();
        api->shutdown();
        if (api->initialized() || rec.detached != 1u || rec.invalid)
            return fail("shutdown idempotence and reentrant callback", dll);
        if (!api->initialize(&valid) || !api->initialized() ||
            api->generation() != 2u || rec.attached != 2u)
            return fail("second lifecycle generation", dll);
        api->shutdown();
        if (rec.detached != 2u || rec.invalid || api->initialized())
            return fail("second shutdown callback", dll);
        // All callback pointers are now cleared before the host's stack ends.
        if (!FreeLibrary(dll)) return fail("final unload");
    }
    std::puts("PASS synthetic Win32 x86: 16 DLL reloads, 32 init/shutdown pairs, 64 callbacks");
    return 0;
}
