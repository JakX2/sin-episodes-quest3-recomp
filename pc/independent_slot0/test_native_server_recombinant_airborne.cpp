#include "native_server_recombinant_airborne.h"
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
struct F{
    alignas(4) unsigned char actor[0xc50]{},desc[0x48]{};
    std::string calls;float scheduled=0;unsigned marker=0;
    const ServerNpcConditionApi* ca=nullptr;ServerNpcConditionMapBindings conditions{};
    template<class T>void put(void* a,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(a)+off,&v,sizeof(v));}
    template<class T>T get(void* a,unsigned off){T v{};std::memcpy(&v,static_cast<unsigned char*>(a)+off,sizeof(v));return v;}
    bool has(int id){ServerNpcConditionResult r{};assert(ca->test(actor,sizeof(actor),id,NpcConditionCurrent,&conditions,&r)&&r.mapped);return r.value!=0;}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    const auto get=reinterpret_cast<GetServerRecombinantAirborneApi>(GetProcAddress(server,server_recombinant_airborne_export));
    const auto gc=reinterpret_cast<GetServerNpcConditionApi>(GetProcAddress(server,server_npc_condition_export));
    assert(get&&gc&&!GetProcAddress(client,server_recombinant_airborne_export)&&!get(2,sizeof(ServerRecombinantAirborneApi)));
    const auto* api=get(1,sizeof(ServerRecombinantAirborneApi));F f;f.ca=gc(1,sizeof(ServerNpcConditionApi));assert(api&&f.ca);
    f.put(f.actor,0,recombinant_minor_vtable);f.put(f.actor,0x360,4.f);f.put(f.actor,0x364,5.f);f.put(f.actor,0x368,6.f);
    f.put(f.actor,0x36c,10.f);f.put(f.actor,0x370,20.f);f.put(f.actor,0x374,30.f);
    f.put(f.desc,0x34,npc_global_condition_base);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    f.conditions.context=&f;f.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept ->int{auto& f=*static_cast<F*>(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    ServerRecombinantPolicyBindings policy{};policy.conditions=f.conditions;
    ServerRecombinantDamageBindings d{&policy,&f};ServerRecombinantMinorBindings m{&d,&f,1};
    m.set_velocity=[](void* p,void* a,const float v[3]) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor&&f.has(58));f.calls+='V';std::memcpy(f.actor+0x250,v,12);return 1;};
    m.schedule_touch_think=[](void* p,void* a,float when,int context) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor&&!context);f.calls+='T';f.scheduled=when;return 1;};
    ServerRecombinantMinorTouchBindings touch{&m,&f};
    touch.install_think=[](void* p,void* a,unsigned marker,int x,int y) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor&&!x&&!y);if(marker==recombinant_airborne_think_marker)assert(f.actor[0xc41]);f.marker=marker;f.calls+='I';return 1;};
    ServerRecombinantAirborneBindings b{&touch,&f,.5f};
    b.forward=[](void* p,const float a[3],float v[3]) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a[0]==10&&a[1]==20&&a[2]==30);f.calls+='F';v[0]=1;v[1]=0;v[2]=0;return 1;};
    b.ballistic=[](void* p,const float start[3],const float end[3],float gravity,float mode,float v[3]) noexcept ->int{
        auto& f=*static_cast<F*>(p);assert(start[0]==7&&end[0]==100&&gravity==1&&mode==3);f.calls+='B';v[0]=40;v[1]=50;v[2]=60;return 1;
    };
    b.clear_ground=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor&&f.has(58)&&f.get<unsigned>(a,0x7c)==recombinant_minor_contact_marker);f.calls+='C';f.actor[0xbc]=0;return 1;};
    b.relocate=[](void* p,void* a,const float v[3],int mode) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor&&v[0]==4&&v[1]==5&&v[2]==7&&!mode);f.calls+='L';f.put(a,0xb8,0x800u);return 1;};
    b.refresh_origin=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor);f.calls+='R';f.put(a,0x2bc,7.f);f.put(a,0x2c0,8.f);f.put(a,0x2c4,9.f);f.put(a,0xb8,0u);return 1;};
    b.ai_update=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor);f.calls+='A';return 1;};
    ServerRecombinantAirborneResult o{};
    f.put(f.actor,0xb8,0x800u);float target[]{100,200,300};
    assert(api->leap(f.actor,sizeof(f.actor),10.f,false,target,&b,&o)&&o.ballistic_calls==1&&o.origin_refreshes==2&&o.velocity[2]==60&&o.contact_deadline==10.5f&&o.snapshot[2]==9&&f.calls=="RBCLVRIT");
    assert(f.get<float>(f.actor,0xc2c)==9&&f.get<unsigned char>(f.actor,0xc41)==1&&f.marker==recombinant_airborne_think_marker&&f.scheduled==10);
    f.calls.clear();assert(api->leap(f.actor,sizeof(f.actor),20.f,true,nullptr,&b,&o)&&o.forward_calls==1&&o.velocity[0]==350&&o.contact_deadline==20.5f&&f.calls=="FCLVRIT");
    f.put(f.actor,0xc34,20.f);f.calls.clear();
    assert(api->think(f.actor,sizeof(f.actor),20.f,&b,&o)&&!o.ai_updates&&!o.think_installed&&f.calls=="T"&&f.scheduled==20);
    f.calls.clear();assert(api->think(f.actor,sizeof(f.actor),20.01f,&b,&o)&&o.ai_updates==1&&f.calls=="AT"&&f.get<float>(f.actor,0xc34)==o.ai_deadline);
    f.actor[0xbc]=1;f.calls.clear();assert(api->think(f.actor,sizeof(f.actor),o.ai_deadline,&b,&o)&&!o.ai_updates&&o.grounded&&o.think_marker==recombinant_default_think_marker&&f.calls=="IT"&&f.scheduled>20.1f);
    f.put(f.actor,0,recombinant_major_vtable);assert(!api->leap(f.actor,sizeof(f.actor),20,true,nullptr,&b,&o));
    FreeLibrary(server);FreeLibrary(client);return 0;
}
