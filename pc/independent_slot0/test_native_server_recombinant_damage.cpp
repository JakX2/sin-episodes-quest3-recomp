#include "native_server_recombinant_damage.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <string>
using namespace sin_port::native_weapon_tables;
struct F {
    alignas(4) unsigned char actor[0xc50]{},source[4]{},info[0x44]{};
    sin_independent::EntityHandleEntry entries[2]{};
    float draws[2]{};
    unsigned used=0;int source_class=1,expected_health=100;
    std::string calls;
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T>T get(void* p,unsigned off){T v{};std::memcpy(&v,static_cast<unsigned char*>(p)+off,sizeof(v));return v;}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    const auto get=reinterpret_cast<GetServerRecombinantDamageApi>(GetProcAddress(server,server_recombinant_damage_export));
    assert(get&&!GetProcAddress(client,server_recombinant_damage_export)&&!get(2,sizeof(ServerRecombinantDamageApi)));
    const auto* api=get(1,sizeof(ServerRecombinantDamageApi));assert(api);
    F f;f.put(f.actor,0,recombinant_major_vtable);f.put(f.actor,0x98,100);f.put(f.actor,0x9c,100);f.put(f.actor,0x878,10);
    f.entries[1].entity=reinterpret_cast<CBaseEntity*>(f.source);f.entries[1].serial=7;
    ServerRecombinantPolicyBindings policy{};policy.entries=f.entries;policy.entry_count=2;
    ServerRecombinantDamageBindings b{&policy,&f};
    b.extent=[](void* p,void* a,unsigned* n) noexcept ->int{auto& f=*static_cast<F*>(p);*n=a==f.source?sizeof(f.source):0;return *n!=0;};
    b.classification=[](void* p,void* a,int* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.source);f.calls+='C';*out=f.source_class;return 1;};
    b.color_fade=[](void* p,void* a,int x,int y) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor&&x==5&&y==50);f.calls+='F';return 1;};
    b.parent_damage=[](void* p,void* a,const void* info,unsigned n,int* out) noexcept ->int{
        auto& f=*static_cast<F*>(p);assert(a==f.actor&&info==f.info&&n==sizeof(f.info));f.calls+='D';*out=7;return 1;
    };
    b.parent_ignite=[](void* p,void* a,const ServerRecombinantIgniteRequest* in) noexcept ->int{
        auto& f=*static_cast<F*>(p);assert(a==f.actor&&in->lifetime==100.f&&in->mode==1&&!in->arg2&&!in->arg3&&!in->arg4&&!in->arg5);f.calls+='I';
        f.actor[0xbf]|=8;return 1;
    };
    b.random_float=[](void* p,float lo,float hi,float* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(lo==-2&&hi==2&&f.used<2);f.calls+='R';*out=f.draws[f.used++];return 1;};
    b.health_changed=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor&&f.get<int>(f.actor,0x9c)==f.expected_health);f.calls+='H';return 1;};
    b.pending_activity=[](void* p,void* a,int v) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor&&v==(f.get<int>(a,0x878)==10?113:f.get<int>(a,0x878)==6?112:111));f.calls+='A';return 1;};
    ServerRecombinantDamageResult o{};ServerRecombinantDamageExpiryResult ex{};
    f.put(f.info,0x24,0x7001u);f.put(f.info,0x38,8u);f.put(f.info,0x2c,4.f);
    f.put(f.source,0,recombinant_major_vtable);
    assert(api->damage_major(f.actor,sizeof(f.actor),f.info,sizeof(f.info),10.f,&b,&o)&&o.immune&&o.source_class==10&&!o.parent_called&&f.calls.empty());
    f.put(f.source,0,0x2249aaa4u);f.source_class=10;
    assert(api->damage_major(f.actor,sizeof(f.actor),f.info,sizeof(f.info),10.f,&b,&o)&&o.immune&&f.calls=="C");
    f.source_class=1;f.calls.clear();
    assert(api->damage_major(f.actor,sizeof(f.actor),f.info,sizeof(f.info),10.f,&b,&o)&&o.accumulated&&o.accumulated_damage==4.f&&o.deadline==15.f&&!o.threshold_reached&&o.parent_called&&o.damage_result==7&&f.calls=="CFD");
    assert(api->expire_major(f.actor,sizeof(f.actor),14.99f,&ex)&&!ex.cleared&&ex.prior==4.f);
    assert(api->expire_major(f.actor,sizeof(f.actor),15.f,&ex)&&ex.cleared&&f.get<float>(f.actor,0xc3c)==0.f&&f.get<float>(f.actor,0xc40)==15.f);
    f.calls.clear();f.put(f.info,0x24,0x8001u);f.put(f.info,0x2c,5.f);
    assert(api->damage_major(f.actor,sizeof(f.actor),f.info,sizeof(f.info),16.f,&b,&o)&&!o.source_live&&o.accumulated_damage==5.f&&!o.threshold_reached&&f.calls=="FD");
    f.calls.clear();f.draws[0]=-2.f;f.draws[1]=2.f;
    assert(api->damage_major(f.actor,sizeof(f.actor),f.info,sizeof(f.info),17.f,&b,&o)&&o.accumulated_damage==10.f&&o.deadline==22.f&&o.threshold_reached&&o.ignited&&o.ignition.random_calls==2&&o.ignition.new_health==60&&o.ignition.health_written&&o.ignition.activity==113&&f.calls=="FIRRHAD");
    f.calls.clear();assert(api->damage_major(f.actor,sizeof(f.actor),f.info,sizeof(f.info),18.f,&b,&o)&&o.immune&&!o.color_called&&!o.accumulated&&f.calls.empty());
    f.put(f.info,0x38,0x20000008u);
    assert(api->damage_major(f.actor,sizeof(f.actor),f.info,sizeof(f.info),18.f,&b,&o)&&!o.immune&&o.color_called&&!o.accumulated&&f.calls=="FD");
    f.calls.clear();f.put(f.info,0x38,0u);
    assert(api->damage_major(f.actor,sizeof(f.actor),f.info,sizeof(f.info),19.f,&b,&o)&&!o.color_called&&!o.accumulated&&f.calls=="D");
    ServerRecombinantIgniteRequest req{100.f,1,0,0,0,0};ServerRecombinantIgniteResult ir{};
    f.put(f.actor,0x9c,39);f.put(f.actor,0x878,6);f.calls.clear();f.used=0;f.draws[0]=-2;
    assert(api->ignite_major(f.actor,sizeof(f.actor),&req,&b,&ir)&&ir.random_calls==1&&!ir.health_written&&ir.new_health==39&&ir.activity==112&&f.calls=="IRA");
    // First threshold equality takes a fresh draw, which may even raise health.
    f.put(f.actor,0x9c,40);f.expected_health=40;f.put(f.actor,0x878,1);f.calls.clear();f.used=0;f.draws[1]=1;
    assert(api->ignite_major(f.actor,sizeof(f.actor),&req,&b,&ir)&&ir.random_calls==2&&ir.new_health==55&&ir.health_written&&ir.activity==111&&f.calls=="IRRHA");
    f.put(f.actor,0x9c,30);f.put(f.actor,0x878,7);f.calls.clear();f.used=0;
    assert(api->ignite_major(f.actor,sizeof(f.actor),&req,&b,&ir)&&!ir.pending_called&&ir.activity==-1&&f.calls=="IR");
    f.put(f.actor,0xc40,0.f);f.put(f.actor,0xc3c,12.f);
    assert(api->expire_major(f.actor,sizeof(f.actor),99.f,&ex)&&!ex.cleared&&ex.prior==12.f);
    f.put(f.actor,0,recombinant_minor_vtable);
    assert(!api->damage_major(f.actor,sizeof(f.actor),f.info,sizeof(f.info),20.f,&b,&o)&&!o.completed);
    FreeLibrary(server);FreeLibrary(client);return 0;
}
