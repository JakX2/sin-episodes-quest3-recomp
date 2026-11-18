// Tests the independent original-sequence adapter against an owned
// MSVC x86 native object; no proprietary game libraries are loaded.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include "client_vgui_controls_bootstrap.h"
namespace vc=sin_independent::vgui_controls;
static int missing=-1,calls=0,initCalls=0;
static bool returns=true;
static int token=1;
static int lastFactoryCount=0;
static std::string trace;
class Core {
public:
    virtual ~Core()=default;
    virtual bool __thiscall Init(vc::Factory* factories,int count) noexcept {
        ++initCalls;
        if(!factories || factories[0]!=&Query || count!=1)std::abort();
        lastFactoryCount=count;
        return returns;
    }
    static void* __cdecl Query(const char* name,int* rc);
};
static Core core;
static void* __cdecl Query(const char* name,int* rc) {
    if(rc || calls>=8 || std::strcmp(name,vc::kNames[calls]))std::abort();
    return calls++==missing?nullptr:
           calls==1?static_cast<void*>(&core):static_cast<void*>(&token);
}
void* __cdecl Core::Query(const char* n,int* rc){return ::Query(n,rc);}
static void __cdecl Trace(const char* phase,const char* value) noexcept {
    trace+=phase;trace+=':';trace+=value;trace+=';';
}
static bool Run(int missingIndex,bool initValue,vc::Result expected,int expectedInit) {
    missing=missingIndex;returns=initValue;
    calls=initCalls=lastFactoryCount=0;trace.clear();
    auto result=vc::Start(&Query,&Trace);
    if(result!=expected || calls!=8 || initCalls!=expectedInit)return false;
    if(expectedInit && lastFactoryCount!=1)return false;
    if(trace.find("vgui-controls-request:VGUI_ivgui006;")==std::string::npos)return false;
    if(expectedInit && trace.find("vgui-controls-init-returned:")==std::string::npos)return false;
    return true;
}
int main() {
    if(vc::Start(nullptr,&Trace)!=vc::Result::MissingFactory)return 1;
    if(!Run(-1,true,vc::Result::InitReturnedTrue,1))return 2;
    if(!Run(-1,false,vc::Result::InitReturnedFalse,1))return 3;
    for(int i=0;i<8;++i)
        if(!Run(i,true,vc::Result::MissingInterface,0))return 4;
    if(!vc::Completed(vc::Result::InitReturnedFalse) ||
       vc::Completed(vc::Result::MissingInterface))return 5;
    std::puts("PASS original VGUI 8-interface controls preflight and IVGui.Init(factory*,1)");
    return 0;
}
