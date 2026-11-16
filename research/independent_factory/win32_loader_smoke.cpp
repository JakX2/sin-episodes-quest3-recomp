// Synthetic-only Win32 x86 executable loader regression; no retail DLLs or forwarding.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

using Factory = void* (__cdecl *)(const char*, int*);
struct Sentinel { std::uint32_t abi_version; std::uint32_t magic; };

static int fail(const char* message, HMODULE module) {
    std::fprintf(stderr, "%s (Win32 error %lu)\n", message, GetLastError());
    if (module) FreeLibrary(module);
    return 1;
}
int main(int argc, char** argv) {
    static_assert(sizeof(void*) == 4, "This smoke must run as a native Win32 x86 process");
    if (argc != 2) return fail("Expected path to synthetic DLL", nullptr);
    HMODULE module = LoadLibraryA(argv[1]);
    if (!module) return fail("LoadLibraryA failed", nullptr);
    auto factory = reinterpret_cast<Factory>(GetProcAddress(module, "CreateInterface"));
    if (!factory) return fail("Missing undecorated CreateInterface export", module);
    int code = -99;
    void* client = factory("SIN_RECON_TEST_CLIENT_001", &code);
    if (!client || code != 0) return fail("Client factory negotiation failed", module);
    const Sentinel c = *static_cast<const Sentinel*>(client);
    if (c.abi_version != 1u || c.magic != 0x53494e43u)
        return fail("Client sentinel mismatch", module);
    void* server = factory("SIN_RECON_TEST_SERVER_001", &code);
    if (!server || server == client || code != 0) return fail("Server factory negotiation failed", module);
    const Sentinel s = *static_cast<const Sentinel*>(server);
    if (s.abi_version != 1u || s.magic != 0x53494e53u)
        return fail("Server sentinel mismatch", module);
    if (factory("SIN_RECON_TEST_CLIENT_001", nullptr) != client)
        return fail("Factory object identity changed", module);
    if (factory("VClient017", &code) != nullptr || code != 1)
        return fail("Unknown retail-like interface incorrectly accepted", module);
    if (factory(nullptr, &code) != nullptr || code != 1)
        return fail("Null name incorrectly accepted", module);
    if (!FreeLibrary(module)) return fail("FreeLibrary failed", nullptr);
    std::puts("Win32 x86 LoadLibrary/GetProcAddress/CreateInterface smoke passed (synthetic only)");
    return 0;
}
