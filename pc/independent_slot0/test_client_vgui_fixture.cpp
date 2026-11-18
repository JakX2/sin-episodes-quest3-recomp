// Fixture-only VGUI factory. No real GUI objects or methods.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstring>
#include <cstdlib>
#pragma comment(linker, "/EXPORT:CreateInterface=_CreateInterface")
static int input=1,systemObject=2;
static void __cdecl DummyMethod() noexcept {}
static void* inputTable[71]{};
static void* systemTable[33]{};
struct Object {void** vtable;};
static Object inputObject{inputTable},systemInterfaceObject{systemTable};
static void PrepareTables(int mode) {
    for(auto& method:inputTable)method=reinterpret_cast<void*>(&DummyMethod);
    for(auto& method:systemTable)method=reinterpret_cast<void*>(&DummyMethod);
    if(mode==35)inputTable[56]=nullptr;
    if(mode==36)systemTable[24]=nullptr;
}
static unsigned requests=0;
extern "C" __declspec(dllexport) void __cdecl ResetVGuiFixture() noexcept {requests=0;}
extern "C" void* __cdecl CreateInterface(const char* name,int* status) {
    char mode[8]={};
    GetEnvironmentVariableA("SIN_TEST_VGUI_FIXTURE_MODE",mode,8);
    const int selected=std::atoi(mode);
    if(!name || status || (selected<27 || (selected>31 && (selected<34 || selected>37))))ExitProcess(140);
    if(++requests==1){
        if(std::strcmp(name,"VGUI_InputInternal001"))ExitProcess(141);
        if(selected>=34) {
            PrepareTables(selected);
            return selected==37?static_cast<void*>(&input):static_cast<void*>(&inputObject);
        }
        return selected==29?nullptr:&input;
    }
    if(requests!=2 || std::strcmp(name,"VGUI_System010") || selected==29)ExitProcess(142);
    if(selected>=34)return &systemInterfaceObject;
    return selected==30?nullptr:selected==31?&input:&systemObject;
}
