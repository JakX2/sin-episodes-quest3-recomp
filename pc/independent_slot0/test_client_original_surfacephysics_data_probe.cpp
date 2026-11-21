// Original-free native MSVC x86 physics GetSurfaceData vslot5 fixture.
// Server.dll 0x2220D9CF and client.dll 0x24184F62 authenticate +0x14;
// SDK2004 specifies the first five floats of surfacedata_t.
#include "client_original_surfacephysics_data_probe.h"
#include <cstdio>
#include <limits>
#include <map>
#include <string>
namespace p=sin_independent::original_surfacephysics_data;
struct OriginalFreeProps {
    int count=94;
    int parseCalls=0,indexCalls=0,dataCalls=0;
    std::map<std::string,int> known{
      {"default",0},{"solidmetal",1},{"metalvehicle",85}};
    p::Physics fake[3]{
      {0.8f,0.2f,1000.f,0.f,0.f},
      {0.3f,0.1f,7800.f,0.2f,0.f},
      {0.4f,0.4f,3900.f,0.3f,0.f}
    };
    bool nullData=false;
    virtual ~OriginalFreeProps()=default;
    virtual int ParseSurfaceData(const char*,const char*) {++parseCalls;return 0;}
    virtual int SurfacePropCount(){return count;}
    virtual int GetSurfaceIndex(const char* name) {
        ++indexCalls;
        const auto it=known.find(name?name:"");
        return it==known.end()?-1:it->second;
    }
    virtual const char* GetPropName(int){return "fixture";}
    virtual const p::Physics* GetSurfaceData(int n) {
        ++dataCalls;
        if(nullData)return nullptr;
        if(n==0)return &fake[0];
        if(n==1)return &fake[1];
        if(n==85)return &fake[2];
        return nullptr;
    }
};
static int Fail(int x){std::fprintf(stderr,"native physics data test %d\n",x);return x;}
int main(){
    static_assert(sizeof(void*)==4,"Win32 x86 ABI required");
    auto r=p::ReadOnly(nullptr);
    if(r.outcome!=p::Outcome::InvalidSurface)return Fail(1);
    OriginalFreeProps originalFree{};
    r=p::ReadOnly(&originalFree);
    if(!r.verified()||r.count!=94||r.indices[2]!=85||
       r.sampled!=3||r.values[0].friction!=0.8f||
       r.values[2].density!=3900.f||
       originalFree.parseCalls!=0||originalFree.dataCalls!=3)return Fail(2);
    originalFree.known.erase("solidmetal");
    r=p::ReadOnly(&originalFree);
    if(r.outcome!=p::Outcome::IndexRefused||r.sampled!=0||
       originalFree.parseCalls!=0)return Fail(3);
    originalFree.known["solidmetal"]=1;
    originalFree.nullData=true;
    r=p::ReadOnly(&originalFree);
    if(r.outcome!=p::Outcome::NullData ||r.sampled!=0)return Fail(4);
    originalFree.nullData=false;
    originalFree.fake[1].friction=std::numeric_limits<float>::quiet_NaN();
    r=p::ReadOnly(&originalFree);
    if(r.outcome!=p::Outcome::NonFiniteData||r.sampled!=1)return Fail(5);
    originalFree.fake[1].friction=0.3f;
    originalFree.count=0;
    r=p::ReadOnly(&originalFree);
    if(r.outcome!=p::Outcome::IndexRefused)return Fail(6);
    if(originalFree.parseCalls!=0)return Fail(7);
    std::puts("PASS original 2006 SurfaceData native: authentic x86 slots2/3/5, three fabricated 20-byte physics tuples, no parser, negative index/null/NaN");
    return 0;
}
