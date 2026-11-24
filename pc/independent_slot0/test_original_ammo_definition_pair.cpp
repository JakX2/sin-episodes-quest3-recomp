#include "original_ammo_definitions.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <cstdio>
namespace d=sin_port::original_ammo;
int main(int argc,char** argv){
    assert(argc==3);
    const auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);
    assert(client&&server);
    const auto gc=reinterpret_cast<d::GetApi>(GetProcAddress(client,d::export_name));
    const auto gs=reinterpret_cast<d::GetApi>(GetProcAddress(server,d::export_name));
    assert(gc&&gs&&!gc(0,sizeof(d::Api))&&!gs(1,0));
    const auto* c=gc(1,sizeof(d::Api));const auto* s=gs(1,sizeof(d::Api));
    assert(c&&s&&c->definition_count()==17&&s->definition_count()==17);
    const char* names[]={"MAG","AR","BUCK","SR","ROCKET","ARNADE","GRENADE","FLARE","DUAG","FLAK",
        "CHAIN","PULSE","HELIGUN","HELIMISL","TURRET1","TURRET2","TURRET3"};
    const int maxima[]={80,330,30,30,5,3,5,30,8,10,750,10,320,320,320,320,320};
    for(int i=1;i<=17;++i){
        d::Definition x{},y{};
        assert(c->definition(i,&x)&&s->definition(i,&y));
        assert(std::memcmp(&x,&y,sizeof(x))==0&&std::strcmp(x.name,names[i-1])==0);
        assert(x.max_carry==maxima[i-1]&&c->max_carry(i)==x.max_carry&&s->max_carry(i)==x.max_carry);
        assert(c->index(x.name)==i&&s->index(y.name)==i&&x.min_splash==4&&x.max_splash==8);
    }
    assert(c->index("duag")==9&&s->index("aRnAdE")==6&&c->index(nullptr)==-1);
    d::Definition unchanged{};unchanged.max_carry=123;
    assert(!c->definition(0,&unchanged)&&!s->definition(18,&unchanged)&&unchanged.max_carry==123);
    assert(!s->definition(1,nullptr)&&c->max_carry(0)==0&&s->max_carry(99)==0);
    assert(FreeLibrary(server)&&FreeLibrary(client));
    std::puts("PASS actual native client/server ammo definition parity (17/17)");
}
