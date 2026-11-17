// Real native MSVC x86 LoadLibrary and thiscall slot-zero regression.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cstdint>
#if !defined(_M_IX86)
#error MSVC Win32 x86 required
#endif
using Factory = void* (__cdecl *)(const char*, int*);
#if defined(SIN_ROLE_client)
constexpr char kName[] = "VClient011";
using Slot0 = bool (__thiscall *)(void*, Factory, Factory, Factory);
#else
constexpr char kName[] = "ServerGameDLL003";
using Slot0 = bool (__thiscall *)(void*, Factory, Factory, Factory, void*);
#endif
static void* __cdecl dummy(const char*, int* status) { if(status) *status=1; return nullptr; }
static bool owned(HMODULE module, const void* address) {
    HMODULE found=nullptr;
    return GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        static_cast<LPCSTR>(address), &found) && found==module;
}
int main(int argc, char** argv) {
    if(argc!=2) return 1;
    HMODULE dll=LoadLibraryA(argv[1]);
    if(!dll) { std::fprintf(stderr,"LoadLibrary failed %lu\n",GetLastError()); return 2; }
    auto exported=GetProcAddress(dll,"CreateInterface");
    if(!exported || !owned(dll,reinterpret_cast<const void*>(exported))) return 3;
    auto factory=reinterpret_cast<Factory>(exported);
    int status=-1;
    void* object=factory(kName,&status);
    if(!object || status!=0 || factory(kName,nullptr)!=object) return 4;
    const char* unknown=
#if defined(SIN_ROLE_client)
        "ServerGameDLL003";
#else
        "VClient011";
#endif
    status=-1;
    if(factory(unknown,&status)!=nullptr || status!=1) return 5;
    status=-1;
    if(factory(nullptr,&status)!=nullptr || status!=1) return 6;
    auto table=*reinterpret_cast<void***>(object);
    if(!table || !owned(dll,table) || !owned(dll,table[0])) return 7;
    auto init=reinterpret_cast<Slot0>(table[0]);
    // Stack must remain balanced after callee cleans 12 or 16 bytes.
    std::uintptr_t before=0,after=0;
    bool result=false;
#if defined(SIN_ROLE_client)
    __asm {
        mov eax, esp
        mov before, eax
        push offset dummy
        push offset dummy
        push offset dummy
        mov ecx, object
        mov eax, init
        call eax
        mov result, al
        mov eax, esp
        mov after, eax
    }
#else
    void* globals=reinterpret_cast<void*>(0x1000);
    __asm {
        mov eax, esp
        mov before, eax
        push globals
        push offset dummy
        push offset dummy
        push offset dummy
        mov ecx, object
        mov eax, init
        call eax
        mov result, al
        mov eax, esp
        mov after, eax
    }
#endif
    if(before!=after || result) return 8;
    if(!FreeLibrary(dll)) return 9;
    std::puts("PASS independent PE32 CreateInterface, DLL-owned vptr, slot0 callee cleanup, fail-closed");
    return 0;
}
