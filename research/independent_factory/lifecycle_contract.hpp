#pragma once
// Test-only ABI: deliberately NOT the retail SiN engine interface layout.
#include <cstdint>
#if !defined(_WIN32) || !defined(_M_IX86)
#error "This lifecycle ABI smoke requires native MSVC Windows x86"
#endif
namespace sin_synthetic_lifecycle {
inline constexpr char kName[] = "SIN_RECON_LIFECYCLE_TEST001";
using HostNotify = void (__cdecl *)(void* context, std::uint32_t event);
struct HostCallbacks {
    std::uint32_t size;
    HostNotify notify;
    void* context;
};
inline constexpr std::uint32_t kAttach = 0x11u;
inline constexpr std::uint32_t kDetach = 0x22u;
struct IRuntime {
    virtual bool __thiscall initialize(const HostCallbacks* callbacks) noexcept = 0;
    virtual bool __thiscall initialized() noexcept = 0;
    virtual std::uint32_t __thiscall generation() noexcept = 0;
    virtual void __thiscall shutdown() noexcept = 0;
protected:
    ~IRuntime() = default;
};
static_assert(sizeof(void*) == 4u, "Win32 x86 ABI only");
static_assert(sizeof(HostCallbacks) == 12u, "Expected three DWORD callback structure");
} // namespace sin_synthetic_lifecycle
