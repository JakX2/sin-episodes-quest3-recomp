// Read-only VGUI fixture checks, no original game binary loaded.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "client_vgui_vtable_probe.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
using Factory=void* (__cdecl *)(const char*,int*);
using Reset=void (__cdecl *)();
int main(int argc,char** argv) {
    if(argc!=2)return 1;
    HMODULE module=LoadLibraryA(argv[1]);
    if(!module)return 2;
    auto factory=reinterpret_cast<Factory>(GetProcAddress(module,"CreateInterface"));
    auto reset=reinterpret_cast<Reset>(GetProcAddress(module,"ResetVGuiFixture"));
    if(!factory || !reset)return 3;
    for(int mode=34;mode<=37;++mode) {
        const auto s=std::to_string(mode);
        if(!SetEnvironmentVariableA("SIN_TEST_VGUI_FIXTURE_MODE",s.c_str()))return 4;
        reset();
        auto input=factory("VGUI_InputInternal001",nullptr);
        auto system=factory("VGUI_System010",nullptr);
        if(!input || !system || input==system)return 5;
        const bool validInput=sin_independent::vgui_layout::Verify(module,input,56);
        const bool validSystem=sin_independent::vgui_layout::Verify(module,system,24);
        if(validInput!=(mode==34 || mode==36) ||
           validSystem!=(mode==34 || mode==35 || mode==37)) {
            std::fprintf(stderr,"FAIL fixture VGUI layout mode %d\n",mode);
            return 6;
        }
    }
    FreeLibrary(module);
    std::puts("PASS read-only VGUI slot 0/56 and 0/24 ownership, missing slots and invalid object");
    return 0;
}
