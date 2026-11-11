// Synthetic Windows x86 DLL for validating the original undecorated
// CreateInterface export through our reconstructed list dispatcher.
// NOT the original SiN server.dll/client.dll or a playable engine module.
#include "sin_compat/original_dll_interface_abi.hpp"
namespace abi = sin_compat::original_dll_interface_abi;
namespace {
int kDemoInterface = 0x177;
void* create_demo() { return &kDemoInterface; }
struct StaticInterfaceRegistry {
    abi::Registry registry;
    abi::Entry demo;
    StaticInterfaceRegistry() {
        // Mirrors original startup registration constructor semantics.
        registry.prepend(demo, &create_demo, "SiNABIProbe001");
    }
};
StaticInterfaceRegistry state;
}
extern "C" void* __cdecl CreateInterface(const char* name, int* status) {
    return state.registry.create_interface(name, status);
}
