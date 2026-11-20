// Phase 479: native Win32 compiled VClient011 slot33 fixture.
// The first DLL is built ONLY for this synthetic fixture with an
// explicit trusted-reader installer; the second is the standard
// fail-closed independently shipped client.dll. NO real game launch.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "client_bf_read_candidate_layout.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#if !defined(_M_IX86)
#error This test requires Win32 MSVC x86 thiscall
#endif

namespace a=sin_independent::bf_read_candidate;
using Factory=void* (__cdecl*)(const char*,int*);
using InitFixture=bool (__cdecl*)();
using InstallReader=void* (__cdecl*)(const a::Layout*,const std::uint8_t*,std::size_t);
using StaticBits=std::uint32_t (__cdecl*)(unsigned,unsigned);
using DynamicBits=std::uint32_t (__cdecl*)(unsigned,const char*,std::size_t);
using Count=std::size_t (__cdecl*)();
using Shutdown=void (__thiscall*)(void*);

static int Fail(int code){
    std::fprintf(stderr,"VClient011 slot33 owned fixture failed=%d\n",code);
    return code;
}
template<std::size_t N>
static void Put(std::array<std::uint8_t,N>& b,std::size_t at,
                unsigned width,std::uint32_t raw){
    for(unsigned i=0;i<width;++i){
        auto mask=static_cast<std::uint8_t>(1u<<((at+i)&7u));
        if((raw>>i)&1u)b[(at+i)>>3]|=mask;
        else b[(at+i)>>3]&=static_cast<std::uint8_t>(~mask);
    }
}
// Real PE32 vtable slot entry and explicit inline x86 stack balance.
// True result is meaningful only for the separate *synthetic test DLL*.
static bool CallSlot33(void* object,void* method,int messageId,
                       void* reader,bool expectedResult){
    std::uintptr_t before=0,after=0;
    unsigned actual=0xffu;
    __asm {
        mov eax, esp
        mov before, eax
        push reader
        push messageId
        mov ecx, object
        mov eax, method
        call eax
        movzx eax, al
        mov actual, eax
        mov eax, esp
        mov after, eax
    }
    return before==after && actual==static_cast<unsigned>(expectedResult);
}
static bool OwnedBy(HMODULE module,const void* address) {
    HMODULE found=nullptr;
    return !!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                static_cast<LPCSTR>(address),&found) &&
           found==module;
}
int main(int argc,char** argv){
    if(argc!=3)return Fail(1);
    HMODULE fixture=LoadLibraryA(argv[1]);
    HMODULE production=LoadLibraryA(argv[2]);
    if(!fixture || !production || fixture==production)return Fail(2);
    auto factory=reinterpret_cast<Factory>(
        GetProcAddress(fixture,"CreateInterface"));
    auto productionFactory=reinterpret_cast<Factory>(
        GetProcAddress(production,"CreateInterface"));
    auto begin=reinterpret_cast<InitFixture>(
        GetProcAddress(fixture,"SiNTestInitializeOwnedMessageRuntime"));
    auto install=reinterpret_cast<InstallReader>(
        GetProcAddress(fixture,"SiNTestInstallOwnedReader"));
    auto staticBits=reinterpret_cast<StaticBits>(
        GetProcAddress(fixture,"SiNTestStaticFieldBits"));
    auto dynamicBits=reinterpret_cast<DynamicBits>(
        GetProcAddress(fixture,"SiNTestDynamicFieldBits"));
    auto dynamicCount=reinterpret_cast<Count>(
        GetProcAddress(fixture,"SiNTestDynamicCount"));
    auto resetCount=reinterpret_cast<Count>(
        GetProcAddress(fixture,"SiNTestResetCount"));
    if(!factory || !productionFactory || !begin || !install ||
       !staticBits || !dynamicBits || !dynamicCount || !resetCount ||
       GetProcAddress(production,"SiNTestInitializeOwnedMessageRuntime") ||
       GetProcAddress(production,"SiNTestInstallOwnedReader"))
        return Fail(3);
    int status=17,regularStatus=17;
    void* object=factory("VClient011",&status);
    void* regular=productionFactory("VClient011",&regularStatus);
    if(!object || !regular || status!=0 || regularStatus!=0)return Fail(4);
    void* method=(*reinterpret_cast<void***>(object))[33];
    void* regularMethod=(*reinterpret_cast<void***>(regular))[33];
    void* shutdownMethod=(*reinterpret_cast<void***>(object))[1];
    if(!method || !regularMethod || !shutdownMethod ||
       !OwnedBy(fixture,method) || !OwnedBy(production,regularMethod))
        return Fail(5);
    if(!begin())return Fail(6);

    alignas(4) std::array<std::uint8_t,256> bytes{};
    a::Layout native{};
    native.data=bytes.data();
    native.data_bytes=static_cast<std::int32_t>(bytes.size());
    native.current_bit=5;
    native.data_bits=53;
    native.assert_on_overflow=1;
    native.debug_name=reinterpret_cast<const char*>(
        static_cast<std::uintptr_t>(0x00000001u));
    Put(bytes,5,16,2);
    Put(bytes,21,32,0x40000000u);
    void* reader=install(&native,bytes.data(),bytes.size());
    if(!reader || reader==&native)return Fail(7);
    if(!CallSlot33(object,method,27,reader,true) ||
       staticBits(2,0)!=0x40000000u ||
       native.current_bit!=5)return Fail(8);

    // Installer copies the layout; modifying the external struct after
    // installation cannot silently alter the reader passed to slot33.
    native.current_bit=0;
    if(!CallSlot33(object,method,27,reader,true) ||
       !CallSlot33(object,method,29,reader,false) ||
       !CallSlot33(object,method,27,reinterpret_cast<void*>(1),false) ||
       staticBits(2,0)!=0x40000000u)return Fail(9);
    // Standard DLL remains protected and refuses all message dispatch.
    if(!CallSlot33(regular,regularMethod,27,reader,false))
        return Fail(10);

    native.current_bit=5;
    native.data_bits=52;
    reader=install(&native,bytes.data(),bytes.size());
    if(!reader || !CallSlot33(object,method,27,reader,false) ||
       staticBits(2,0)!=0x40000000u)return Fail(11);
    native.data_bits=53;
    native.data=bytes.data()+4;
    reader=install(&native,bytes.data(),bytes.size());
    if(!reader || !CallSlot33(object,method,27,reader,false))
        return Fail(12);
    native.data=bytes.data();

    // A named dynamic statistic: ID 55, NUL string, float32 payload.
    bytes.fill(0);
    std::size_t end=3;
    Put(bytes,end,16,55);end+=16;
    for(unsigned char c:std::string("pistol")){
        Put(bytes,end,8,c);end+=8;
    }
    Put(bytes,end,8,0);end+=8;
    Put(bytes,end,32,0x3f800000u);end+=32;
    native.current_bit=3;
    native.data_bits=static_cast<std::int32_t>(end);
    reader=install(&native,bytes.data(),bytes.size());
    if(!reader || !CallSlot33(object,method,28,reader,true) ||
       dynamicCount()!=1 || dynamicBits(55,"pistol",6)!=0x3f800000u)
        return Fail(13);

    // ResetHUD 5 from a bit-unaligned one-byte payload.
    bytes.fill(0);
    Put(bytes,7,8,0xa5);
    native.current_bit=7;
    native.data_bits=15;
    reader=install(&native,bytes.data(),bytes.size());
    if(!reader || !CallSlot33(object,method,5,reader,true) ||
       resetCount()!=1 || dynamicCount()!=0 || staticBits(2,0)!=0)
        return Fail(14);

    // Calling Shutdown via actual slot 1 disables the separate
    // synthetic stats session. Actual client Init was never enabled.
    reinterpret_cast<Shutdown>(shutdownMethod)(object);
    if(!CallSlot33(object,method,5,reader,false))return Fail(15);
    if(!begin() || resetCount()!=0 || dynamicCount()!=0 ||
       staticBits(2,0)!=0)return Fail(16);

    FreeLibrary(production);
    FreeLibrary(fixture);
    std::puts("PASS PE32 actual VClient011 slot33 thiscall ret8: trusted fake 27/28/5 updates, malformed refusal, client.dll production fail closed");
    return 0;
}
