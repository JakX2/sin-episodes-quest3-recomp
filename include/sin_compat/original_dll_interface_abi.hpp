#pragma once
// Phase177: original 2006 Win32 module CreateInterface behavior.
// The 32-bit metadata layout, registration order and return-code behavior
// are verified from native server.dll/client.dll. The host model is NOT an
// original Source engine object, x86 vtable or playable replacement DLL.
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace sin_compat::original_dll_interface_abi {

// Original registration constructors store three little-endian DWORDs:
// thunk pointer, interface-name pointer, prior registration-list head.
struct NativeRegistration32 {
    std::uint32_t factory_va;
    std::uint32_t interface_name_va;
    std::uint32_t next_va;
};
static_assert(sizeof(NativeRegistration32) == 12,
              "original 2006 x86 registry node is exactly three DWORDs");
static_assert(std::is_standard_layout<NativeRegistration32>::value,
              "native interface registration is a simple 3-word record");

using Factory = void* (*)();
struct Entry {
    Factory factory;
    const char* name;
    Entry* next;
};

class Registry {
public:
    void prepend(Entry& entry, Factory factory, const char* name) noexcept {
        // Native 0x221FB5D0 (server) and 0x24164160 (client): newest-first.
        entry.factory = factory;
        entry.name = name;
        entry.next = head_;
        head_ = &entry;
    }

    Entry* head() const noexcept { return head_; }

    void* create_interface(const char* requested_name, int* return_code) const {
        // Native entry points 0x221FB5F0 / 0x24164180.
        // Like the original, the nonempty-registry path assumes a non-null
        // name, registered interface names and factory function pointers.
        for (Entry* node = head_; node != nullptr; node = node->next) {
            if (std::strcmp(node->name, requested_name) != 0) continue;
            if (return_code) *return_code = 0; // before factory invocation
            return node->factory(); // even null factory result counts as match
        }
        if (return_code) *return_code = 1;
        return nullptr;
    }
private:
    Entry* head_ = nullptr;
};

inline constexpr std::uint32_t kOriginalServerCreateInterfaceVA = 0x221FB5F0u;
inline constexpr std::uint32_t kOriginalClientCreateInterfaceVA = 0x24164180u;
inline constexpr std::uint32_t kOriginalServerRegistryHeadVA = 0x22573C5Cu;
inline constexpr std::uint32_t kOriginalClientRegistryHeadVA = 0x24369378u;
inline constexpr std::uint32_t kOriginalServerRegistrationCtorVA = 0x221FB5D0u;
inline constexpr std::uint32_t kOriginalClientRegistrationCtorVA = 0x24164160u;

} // namespace sin_compat::original_dll_interface_abi