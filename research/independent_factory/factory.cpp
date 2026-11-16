// Synthetic independent DLL factory boundary. No original game code or forwarding.
#include <cstdint>
#include <cstring>
#if defined(_WIN32)
#define SIN_EXPORT extern "C" __declspec(dllexport)
#define SIN_CDECL __cdecl
#else
#define SIN_EXPORT extern "C" __attribute__((visibility("default")))
#define SIN_CDECL
#endif
namespace sin_reconstruction {
struct InterfaceSentinel { std::uint32_t abi_version; std::uint32_t magic; };
static InterfaceSentinel client{1u, 0x53494e43u};
static InterfaceSentinel server{1u, 0x53494e53u};
// Synthetic names deliberately do not claim retail ABI compatibility.
static void* lookup(const char* name) noexcept {
    if (!name) return nullptr;
    if (std::strcmp(name, "SIN_RECON_TEST_CLIENT_001") == 0) return &client;
    if (std::strcmp(name, "SIN_RECON_TEST_SERVER_001") == 0) return &server;
    return nullptr;
}
}
SIN_EXPORT void* SIN_CDECL CreateInterface(const char* name, int* return_code) noexcept {
    void* result = sin_reconstruction::lookup(name);
    if (return_code) *return_code = result ? 0 : 1;
    return result;
}
