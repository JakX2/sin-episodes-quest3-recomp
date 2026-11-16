// Synthetic Win32 x86 DLL lifecycle and factory ABI regression. No retail forwarding.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
using Factory = void* (__cdecl *)(const char*, int*);
struct Sentinel { std::uint32_t abi_version; std::uint32_t magic; };
static_assert(sizeof(void*) == 4, "Native x86 test required");
static_assert(sizeof(Sentinel) == 8, "Synthetic ABI layout drift");
static int failure(const char* why, HMODULE mod = nullptr) {
    std::fprintf(stderr, "%s (GetLastError=%lu)\n", why, GetLastError());
    if (mod) FreeLibrary(mod);
    return 1;
}
int main(int argc, char** argv) {
    if (argc != 2) return failure("Expected path to independent synthetic DLL");
    // Negative loader case must fail without accidentally resolving a retail gameplay DLL.
    if (LoadLibraryA("sin_reconstruction_nonexistent_01.dll"))
        return failure("Unexpected synthetic missing-DLL load success");
    for (int cycle = 0; cycle < 32; ++cycle) {
        HMODULE mod = LoadLibraryA(argv[1]);
        if (!mod) return failure("LoadLibrary failed");
        auto factory = reinterpret_cast<Factory>(GetProcAddress(mod, "CreateInterface"));
        if (!factory) return failure("Missing undecorated CreateInterface", mod);
        HMODULE owning_module = nullptr;
        if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                reinterpret_cast<LPCSTR>(factory), &owning_module) ||
            owning_module != mod)
            return failure("Factory function belongs to wrong module", mod);
        int code = -99;
        void* client = factory("SIN_RECON_TEST_CLIENT_001", &code);
        if (!client || code != 0) return failure("Client factory negotiation", mod);
        const Sentinel c = *static_cast<const Sentinel*>(client);
        if (c.abi_version != 1u || c.magic != 0x53494e43u)
            return failure("Client sentinel ABI mismatch", mod);
        void* server = factory("SIN_RECON_TEST_SERVER_001", &code);
        if (!server || server == client || code != 0)
            return failure("Server factory negotiation", mod);
        const Sentinel s = *static_cast<const Sentinel*>(server);
        if (s.abi_version != 1u || s.magic != 0x53494e53u)
            return failure("Server sentinel ABI mismatch", mod);
        if (factory("SIN_RECON_TEST_CLIENT_001", nullptr) != client)
            return failure("Client object identity drift", mod);
        if (factory("VClient017", &code) != nullptr || code != 1)
            return failure("Unsupported retail-like name accepted", mod);
        if (factory(nullptr, &code) != nullptr || code != 1)
            return failure("Null interface name accepted", mod);
        // The second handle must refer to the same module, and releasing it must
        // leave the first handle usable. This checks loader refcount behavior.
        HMODULE second = LoadLibraryA(argv[1]);
        if (!second || second != mod) return failure("Second module handle mismatch", mod);
        if (!FreeLibrary(second)) return failure("Second FreeLibrary failed", mod);
        if (factory("SIN_RECON_TEST_SERVER_001", &code) != server || code != 0)
            return failure("Factory unusable after partial unload", mod);
        if (!FreeLibrary(mod)) return failure("Final FreeLibrary failed");
    }
    std::puts("Win32 x86 synthetic CreateInterface: 32 load/unload and refcount cycles passed");
    return 0;
}
