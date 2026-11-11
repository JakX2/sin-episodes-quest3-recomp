// Synthetic Windows x86 DLL for validating the native undecorated export
// and cdecl loader contract. NOT the original SiN server.dll/client.dll.
#include <cstring>
static int kDemoInterface = 0x177;
extern "C" void* __cdecl CreateInterface(const char* name, int* status) {
    if (name && std::strcmp(name, "SiNABIProbe001") == 0) {
        if (status) *status = 0;
        return &kDemoInterface;
    }
    if (status) *status = 1;
    return nullptr;
}