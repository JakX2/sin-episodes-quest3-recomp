// Synthetic MSVC Win32 x86 vtable test for SDK2004 physics surface readback.
// NO retail executable and NO original game assets are used in public CI.
#include "client_original_surfaceprops_readback.h"
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
namespace probe=sin_independent::original_surfaceprops_readback;
static int Fail(int n) {
    std::fprintf(stderr,"surface readback native fixture failed %d\n",n);
    return n;
}
struct Props {
    int count=93;
    std::map<std::string,int> map{
        {"default",0},{"solidmetal",1},{"metalvehicle",84}};
    int calls[3]{};
    virtual ~Props()=default;
    virtual int ParseSurfaceData(const char*,const char*) {
        return -100; // never called by this read-only probe
    }
    virtual int SurfacePropCount() {
        ++calls[0];
        return count;
    }
    virtual int GetSurfaceIndex(const char* name) {
        ++calls[1];
        if(!name){++calls[2];return -1;}
        const auto it=map.find(name);
        return it==map.end()?-1:it->second;
    }
};
static_assert(sizeof(void*)==4,"MSVC 32-bit binary interface required");
int main() {
    auto none=probe::Query(nullptr);
    if(none.outcome!=probe::Outcome::InvalidObject||none.all_found())
        return Fail(1);
    Props props{};
    auto r=probe::Query(&props);
    if(r.outcome!=probe::Outcome::ThreeInRange||
       !r.all_found()||r.count!=93||
       r.indices[0]!=0||r.indices[1]!=1||r.indices[2]!=84||
       props.calls[0]!=1||props.calls[1]!=3||
       props.calls[2]!=0)return Fail(2);

    props.map.erase("metalvehicle");
    r=probe::Query(&props);
    if(r.outcome!=probe::Outcome::SomeNotFound||
       r.all_found()||r.indices[2]!=-1)return Fail(3);
    props.map["metalvehicle"]=84;

    props.map["solidmetal"]=93;
    r=probe::Query(&props);
    if(r.outcome!=probe::Outcome::IndexInvalid||
       r.all_found())return Fail(4);
    props.map["solidmetal"]=1;

    props.map["metalvehicle"]=-2;
    r=probe::Query(&props);
    if(r.outcome!=probe::Outcome::IndexInvalid||
       r.all_found())return Fail(5);
    props.map["metalvehicle"]=84;

    const int prior=props.calls[1];
    props.count=-3;
    r=probe::Query(&props);
    if(r.outcome!=probe::Outcome::CountInvalid||
       r.all_found()||props.calls[1]!=prior)return Fail(6);

    props.count=20000;
    r=probe::Query(&props);
    if(r.outcome!=probe::Outcome::CountInvalid||
       r.all_found()||props.calls[1]!=prior)return Fail(7);

    props.count=0;
    props.map.clear();
    r=probe::Query(&props);
    if(r.outcome!=probe::Outcome::SomeNotFound||
       r.count!=0||r.all_found())return Fail(8);

    std::puts("PASS SDK2004 physics surface readback: x86 slots 2/3, authentic name metadata, bounded counts, valid/absent/out-of-range indices; no original process or mutation");
    return 0;
}
