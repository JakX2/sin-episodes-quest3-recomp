#include "sin_compat/original_dll_interface_abi.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
namespace abi = sin_compat::original_dll_interface_abi;
namespace {
int a_calls = 0;
int b_calls = 0;
int zero_calls = 0;
int a_object = 41;
int b_object = 52;
void* factory_a() { ++a_calls; return &a_object; }
void* factory_b() { ++b_calls; return &b_object; }
void* factory_null() { ++zero_calls; return nullptr; }
}
int main() {
    static_assert(sizeof(abi::NativeRegistration32) == 12, "native x86 ABI");
    assert(abi::kOriginalServerCreateInterfaceVA == 0x221fb5f0u);
    assert(abi::kOriginalClientCreateInterfaceVA == 0x24164180u);
    abi::Registry r;
    int rc = -77;
    assert(r.create_interface("ServerGameDLL003", &rc) == nullptr);
    assert(rc == 1);
    // Original head insertion is LIFO and preserves the prior next pointer.
    abi::Entry first{}, second{}, third{};
    r.prepend(first, &factory_a, "ServerGameDLL003");
    assert(r.head() == &first && first.next == nullptr);
    r.prepend(second, &factory_b, "ServerGameClients003");
    assert(r.head() == &second && second.next == &first);
    r.prepend(third, &factory_null, "ServerGameDLL003");
    assert(r.head() == &third && third.next == &second);
    // Duplicate name checks latest registration first and does not fall back
    // when the chosen factory itself returns nullptr.
    rc = -77;
    assert(r.create_interface("ServerGameDLL003", &rc) == nullptr);
    assert(rc == 0 && zero_calls == 1 && a_calls == 0);
    assert(r.create_interface("ServerGameClients003", nullptr) == &b_object);
    assert(b_calls == 1);
    rc = -77;
    assert(r.create_interface("servergamedll003", &rc) == nullptr);
    assert(rc == 1 && zero_calls == 1 && b_calls == 1);
    rc = -77;
    assert(r.create_interface("ServerGameDLL004", &rc) == nullptr);
    assert(rc == 1);
    assert(r.create_interface("NoSuchVersion001", nullptr) == nullptr);
    // Complete equality, not prefix or case-folding. Generated request
    // strings exercise the behavior without relying on game-specific names.
    for (int i=0; i<64; ++i) {
        std::string input = "ServerGameClients003";
        if (i != 0) input[static_cast<std::size_t>(i)%input.size()] ^= 1;
        rc = -77;
        void* result = r.create_interface(input.c_str(), &rc);
        assert((result != nullptr) == (input == "ServerGameClients003"));
        assert(rc == (result ? 0 : 1));
    }
    // A separate host registry models client.dll's independent registration
    // list. It cannot resolve server-only names, even when they match strings.
    abi::Registry client;
    abi::Entry client_entry{};
    client.prepend(client_entry, &factory_a, "VClient011");
    rc = -77;
    assert(client.create_interface("VClient011", &rc) == &a_object);
    assert(rc == 0 && a_calls == 1);
    assert(client.create_interface("ServerGameDLL003", nullptr) == nullptr);
    assert(r.create_interface("VClient011", nullptr) == nullptr);
    return 0;
}