#include "native_server_weapon_script_view.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <cstdio>
using namespace sin_port::native_weapon_tables;
struct F {
    alignas(4) unsigned char rows[32]{},info[0x724]{},fallback[0x724]{},weapon[0x579]{};
    unsigned calls=0;
    template<class T> void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto g=reinterpret_cast<GetServerWeaponScriptViewApi>(GetProcAddress(server,server_weapon_script_view_export));
    assert(g&&!GetProcAddress(client,server_weapon_script_view_export)&&!g(0,sizeof(ServerWeaponScriptViewApi))&&!g(1,0));
    const auto* api=g(1,sizeof(ServerWeaponScriptViewApi));assert(api);
    F f;f.put(f.rows,28,0x1234u);f.put(f.weapon,0x51c,std::uint16_t{1});
    f.put(f.info,0x160,30);f.put(f.info,0x164,-1);f.put(f.info,0x174,std::uint8_t{7});
    f.put(f.info,0x720,0x8000);f.put(f.fallback,0x160,-1);
    ServerWeaponScriptView v{f.rows,sizeof(f.rows),2,f.fallback,sizeof(f.fallback),&f,
        [](void* p,std::uint32_t word,const void** out,unsigned* bytes) noexcept -> int {
            auto& f=*static_cast<F*>(p);++f.calls;if(word!=0x1234)return 0;*out=f.info;*bytes=sizeof(f.info);return 1;}};
    ServerWeaponScriptRecord rec{};int n=0;
    assert(api->weapon(&v,f.weapon,sizeof(f.weapon),&rec)&&rec.data==f.info&&!rec.used_default&&f.calls==1);
    assert(api->value(&v,f.weapon,sizeof(f.weapon),0x404,&n)&&n==30);
    assert(api->value(&v,f.weapon,sizeof(f.weapon),0x408,&n)&&n==-1);
    assert(api->value(&v,f.weapon,sizeof(f.weapon),0x438,&n)&&n==1);
    assert(api->value(&v,f.weapon,sizeof(f.weapon),0x43c,&n)&&n==0);
    assert(api->value(&v,f.weapon,sizeof(f.weapon),0x418,&n)&&n==7);
    assert(api->value(&v,f.weapon,sizeof(f.weapon),0x4a0,&n)&&n==0x8000);
    const unsigned prior=f.calls;
    assert(api->resolve(&v,2,&rec)&&rec.data==f.fallback&&rec.used_default);
    assert(api->resolve(&v,0xffff,&rec)&&rec.used_default&&f.calls==prior);
    assert(!api->resolve(&v,0,&rec)&&!rec.data);
    f.put(f.weapon,0x51c,std::uint16_t{0xffff});
    assert(api->value(&v,f.weapon,sizeof(f.weapon),0x404,&n)&&n==-1);
    v.fallback_bytes=0x160;assert(!api->value(&v,f.weapon,sizeof(f.weapon),0x404,&n));v.fallback_bytes=sizeof(f.fallback);
    v.bytes=31;assert(!api->resolve(&v,0xffff,&rec));v.bytes=32;
    const std::uint32_t expected[]{0,0x3d99999a,0x3d99999a,0,0x3ecccccd,0x3f19999a,0x3ecccccd,0x3f333333,0x3dcccccd};
    for(unsigned k=0;k<9;++k){double d=0;assert(api->interval(k,&d));float x=static_cast<float>(d);std::uint32_t raw=0;std::memcpy(&raw,&x,4);assert(raw==expected[k]&&d==static_cast<double>(x));}
    assert(api->burst(f.weapon,sizeof(f.weapon),WeaponActivityAssault,&n)&&n==3);
    f.weapon[0x578]=1;assert(api->burst(f.weapon,sizeof(f.weapon),WeaponActivityAssault,&n)&&n==1);
    assert(api->burst(nullptr,0,WeaponActivitySmg,&n)&&n==3);
    assert(api->burst(nullptr,0,WeaponActivityChaingun,&n)&&n==3);
    assert(!api->burst(f.weapon,sizeof(f.weapon),WeaponActivityGrenade,&n));
    FreeLibrary(server);FreeLibrary(client);std::puts("PASS weapon script indexed view, default, original getter values and class timing constants");
}
