// Native Win32 PE32 DLL with a REAL MSVC x86 thiscall vtable.
// Never forwards to, links, loads, or returns an object from retail SiN DLLs.
#include "virtual_interface_contract.hpp"
#include <cstring>

namespace {
class SyntheticCanary final : public sin_synthetic_x86::ICanary {
public:
    int __thiscall read_value() noexcept override { return value_; }
    int __thiscall add_value(int delta) noexcept override {
        value_ += delta; // Test arguments stay inside signed 32-bit range.
        return value_;
    }
    std::uint32_t __thiscall signature() noexcept override {
        return sin_synthetic_x86::kSignature;
    }
private:
    int value_ = 13;
};
static SyntheticCanary instance;
static_assert(sizeof(SyntheticCanary) == 8u, "Expected one 32-bit vptr plus one 32-bit field");
}

extern "C" __declspec(dllexport) void* __cdecl CreateInterface(
    const char* name, int* return_code) noexcept {
    // Deliberately expose ONLY a synthetic name. This is not the original
    // game's interface namespace and must not be used in a retail game.
    const bool found = name != nullptr &&
        std::strcmp(name, sin_synthetic_x86::kInterfaceName) == 0;
    if (return_code) *return_code = found ? 0 : 1;
    return found ? static_cast<sin_synthetic_x86::ICanary*>(&instance) : nullptr;
}
