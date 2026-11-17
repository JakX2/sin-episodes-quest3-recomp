#pragma once
// Synthetic test contract only. NOT SiN's VClient011 / ServerGameDLL003 ABI.
// This header is shared by our two independently built Win32 x86 test modules
// to verify MSVC's real virtual-call mechanics across a PE32 DLL boundary.
#include <cstdint>

#if !defined(_WIN32) || !defined(_M_IX86)
#error "Synthetic virtual ABI smoke requires MSVC Windows x86"
#endif

namespace sin_synthetic_x86 {
inline constexpr char kInterfaceName[] = "SIN_RECON_VTABLE_TEST001";
inline constexpr std::uint32_t kSignature = 0x53494e58u;

struct ICanary {
    // NO virtual destructor. The three pure virtual slots are exactly the
    // three synthetic test operations. Do not delete through this pointer.
    virtual int __thiscall read_value() noexcept = 0;
    virtual int __thiscall add_value(int delta) noexcept = 0;
    virtual std::uint32_t __thiscall signature() noexcept = 0;
protected:
    ~ICanary() = default;
};

static_assert(sizeof(void*) == 4u, "The test is explicitly PE32, not x64");
} // namespace sin_synthetic_x86
