#include "native_server_weapon_activity.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <cstdio>
#include <initializer_list>
using namespace sin_port::native_weapon_tables;
struct F {
    alignas(4) unsigned char actor[0x8f0]{},weapon[4]{};
    unsigned kind=WeaponActivityAssault,calls=0;
    sin_independent::EntityHandleEntry slots[2]{};
    template<class T> void put(unsigned off,T v){std::memcpy(actor+off,&v,sizeof(v));}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto g=reinterpret_cast<GetServerWeaponActivityApi>(GetProcAddress(server,server_weapon_activity_export));
    assert(g&&!GetProcAddress(client,server_weapon_activity_export)&&!g(0,sizeof(ServerWeaponActivityApi))&&!g(1,0));
    auto* api=g(1,sizeof(ServerWeaponActivityApi));assert(api);
    const unsigned counts[]{0,60,46,1,28,2,28,46,37};
    unsigned cases=0;
    for(unsigned k=0;k<9;++k){
        const ServerWeaponActivityRow* rows=nullptr;unsigned n=0;
        assert(api->table(k,&rows,&n)&&n==counts[k]&&(!n||rows));
        for(int i=-1;i<=921;++i)for(bool original:{false,true}){
            bool expected=original;int value=i;
            for(unsigned j=0;j<n;++j)if(rows[j].input==i){expected=rows[j].required;value=rows[j].output;break;}
            bool required=original;int out=-99;
            assert(api->translate(k,i,&required,&out)&&required==expected&&out==value);++cases;
        }
    }
    bool required=false;int out=-1;
    assert(api->translate(WeaponActivityAssault,208,&required,&out)&&required&&out==219);
    assert(api->translate(WeaponActivitySmg,208,&required,&out)&&required&&out==208);
    assert(api->translate(WeaponActivityChaingun,1,&required,&out)&&required&&out==1);
    assert(api->translate(WeaponActivityScatter,1,&required,&out)&&required&&out==345);
    assert(api->translate(WeaponActivityMagnum,67,&required,&out)&&!required&&out==397);
    assert(api->translate(WeaponActivityGrenade,16,nullptr,&out)&&out==316);
    F f;f.slots[1].serial=7;f.slots[1].entity=reinterpret_cast<CBaseEntity*>(f.weapon);
    ServerWeaponActivityBindings b{f.slots,2,&f,
        [](void* p,void* w,unsigned* out) noexcept -> int {auto& f=*static_cast<F*>(p);assert(w==f.weapon);++f.calls;*out=f.kind;return 1;},
        [](void*,void*,unsigned* out) noexcept -> int {*out=0x08000000u;return 1;}
    };
    f.put(0x78c,0x7001u);f.put(0x8ec,4u);
    assert(api->actor_translate(f.actor,sizeof(f.actor),16,&b,&required,&out)&&required&&out==309);
    assert(api->actor_translate(f.actor,sizeof(f.actor),999,&b,&required,&out)&&required&&out==999);
    unsigned caps=0;assert(api->capabilities(f.actor,sizeof(f.actor),&b,&caps)&&caps==0x2004u);
    f.kind=WeaponActivityMelee;assert(api->capabilities(f.actor,sizeof(f.actor),&b,&caps)&&caps==0x8004u);
    f.kind=WeaponActivityBase;assert(api->capabilities(f.actor,sizeof(f.actor),&b,&caps)&&caps==0x08000004u);
    const unsigned before=f.calls;
    f.put(0x78c,0x8001u);assert(api->actor_translate(f.actor,sizeof(f.actor),16,&b,&required,&out)&&!required&&out==16&&f.calls==before);
    assert(api->capabilities(f.actor,sizeof(f.actor),&b,&caps)&&caps==4u&&f.calls==before);
    f.put(0x78c,0xffffffffu);required=true;
    assert(api->actor_translate(f.actor,sizeof(f.actor),16,&b,&required,&out)&&!required);
    FreeLibrary(server);FreeLibrary(client);std::printf("PASS weapon activity tables and serial dispatch: %u exhaustive flag cases\n",cases);
}
