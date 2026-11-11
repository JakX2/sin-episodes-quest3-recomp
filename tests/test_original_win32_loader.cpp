// This is a SYNTHETIC DLL loading test, not native SiN engine execution.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <windows.h>
using Factory = void* (__cdecl *)(const char*, int*);
int main(int argc, char** argv) {
    assert(argc == 2);
    HMODULE dll = LoadLibraryA(argv[1]);
    assert(dll != nullptr);
    FARPROC address = GetProcAddress(dll, "CreateInterface");
    assert(address != nullptr); // EXACT undecorated PE export name
    auto factory = reinterpret_cast<Factory>(address);
    int status = -1;
    void* api = factory("SiNABIProbe001", &status);
    assert(api != nullptr && status == 0);
    assert(*static_cast<int*>(api) == 0x177);
    status = -1;
    assert(factory("SiNABIProbe002", &status) == nullptr && status == 1);
    status = -1;
    assert(factory("sinabiprobe001", &status) == nullptr && status == 1);
    assert(factory("SiNABIProbe001", nullptr) == api);
    assert(FreeLibrary(dll) != 0);
    return 0;
}