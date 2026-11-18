// Fixture-only VGUI factory. No real GUI objects or methods.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstring>
#include <cstdlib>
#pragma comment(linker, "/EXPORT:CreateInterface=_CreateInterface")
static int input=1,systemToken=2;
static unsigned requests=0;
extern "C" void* __cdecl CreateInterface(const char* name,int* status) {
    char mode[8]={};
    GetEnvironmentVariableA("SIN_TEST_VGUI_FIXTURE_MODE",mode,8);
    const int selected=std::atoi(mode);
    if(!name || status || selected<27 || selected>31)ExitProcess(140);
    if(++requests==1){
        if(std::strcmp(name,"VGUI_InputInternal001"))ExitProcess(141);
        return selected==29?nullptr:&input;
    }
    if(requests!=2 || std::strcmp(name,"VGUI_System010") || selected==29)ExitProcess(142);
    return selected==30?nullptr:selected==31?&input:&systemToken;
}
