#include "native_server_recombinant_pounce.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <string>
#include <cmath>
using namespace sin_port::native_weapon_tables;
struct F{
    alignas(4) unsigned char actor[0xc50]{},enemy[4]{},desc[0x48]{};
    bool live=true;unsigned draws=0;std::string calls;
    template<class T>void put(void* a,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(a)+off,&v,sizeof(v));}
    template<class T>T get(void* a,unsigned off){T v{};std::memcpy(&v,static_cast<unsigned char*>(a)+off,sizeof(v));return v;}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    const auto get=reinterpret_cast<GetServerRecombinantPounceApi>(GetProcAddress(server,server_recombinant_pounce_export));
    assert(get&&!GetProcAddress(client,server_recombinant_pounce_export)&&!get(2,sizeof(ServerRecombinantPounceApi)));
    const auto* api=get(1,sizeof(ServerRecombinantPounceApi));assert(api);
    F f;f.put(f.actor,0,recombinant_minor_vtable);f.put(f.actor,0x908,91u);f.put(f.actor,0xc24,3.f);f.put(f.actor,0xc28,4.f);f.put(f.actor,0xc2c,5.f);
    f.put(f.desc,0x34,npc_global_condition_base);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    ServerRecombinantPolicyBindings policy{};policy.conditions.context=&f;
    policy.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept ->int{auto& f=*static_cast<F*>(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    ServerRecombinantDamageBindings d{&policy,&f};ServerRecombinantMinorBindings m{&d,&f,1};
    m.set_velocity=[](void* p,void* a,const float v[3]) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor);std::memcpy(f.actor+0x250,v,12);f.calls+='V';return 1;};
    m.schedule_touch_think=[](void* p,void*,float now,int mode) noexcept ->int{assert(now==10&&!mode);static_cast<F*>(p)->calls+='T';return 1;};
    m.refresh_velocity=[](void* p,void*) noexcept ->int{static_cast<F*>(p)->calls+='K';return 1;};
    ServerRecombinantMinorTouchBindings touch{&m,&f};
    touch.set_movement=[](void* p,void*,int type,int mode) noexcept ->int{assert(type==3&&!mode);static_cast<F*>(p)->calls+='M';return 1;};
    touch.install_think=[](void* p,void* a,unsigned marker,int x,int y) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor&&marker==recombinant_airborne_think_marker&&!x&&!y&&f.actor[0xc41]);f.calls+='I';return 1;};
    ServerRecombinantAirborneBindings air{&touch,&f,600.f};
    air.forward=[](void* p,const float[3],float v[3]) noexcept ->int{static_cast<F*>(p)->calls+='F';v[0]=1;v[1]=0;v[2]=0;return 1;};
    air.clear_ground=[](void* p,void*) noexcept ->int{static_cast<F*>(p)->calls+='G';return 1;};
    air.relocate=[](void* p,void*,const float[3],int) noexcept ->int{static_cast<F*>(p)->calls+='L';return 1;};
    air.refresh_origin=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<F*>(p);f.put(a,0x2bc,11.f);f.put(a,0x2c0,12.f);f.put(a,0x2c4,13.f);f.calls+='R';return 1;};
    ServerRecombinantEventBindings event{};event.policy=&policy;event.context=&f;
    event.enemy=[](void* p,void*,void** out) noexcept ->int{auto& f=*static_cast<F*>(p);f.calls+='E';*out=f.live?f.enemy:nullptr;return 1;};
    event.random=[](void* p,float lo,float hi,float* out) noexcept ->int{auto& f=*static_cast<F*>(p);f.calls+='D';if(!f.draws++){assert(lo==.5f&&hi==1);*out=.75f;}else{assert(lo==-1&&hi==1);*out=.5f;}return 1;};
    ServerRecombinantPounceBindings b{&air,&event,&f,12.5f};
    b.normalize=[](void* p,float v[3]) noexcept ->int{auto& f=*static_cast<F*>(p);assert(std::fabs(v[0]-.2f)<.00001f&&std::fabs(v[1]-1.1f)<.00001f&&v[2]==.75f);f.calls+='N';float len=std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);for(int i=0;i<3;++i)v[i]/=len;return 1;};
    b.yaw=[](void* p,const float[3],float* out) noexcept ->int{static_cast<F*>(p)->calls+='Y';*out=80;return 1;};
    b.motor_yaw=[](void* p,unsigned motor,float yaw,float dt) noexcept ->int{assert(motor==91&&yaw==-100&&dt==-1);static_cast<F*>(p)->calls+='A';return 1;};
    b.damage_info=[](void* p,void* a,float damage,unsigned type,void* info,unsigned n) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor&&damage==12.5f&&type==4&&n==0x48);f.put(info,0x2c,damage);f.calls+='P';return 1;};
    b.force=[](void* p,void* info,unsigned n,void* a,const float v[3],const float origin[3],float scale) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor&&n==0x48&&f.get<float>(info,0x2c)==12.5f&&v[0]==350&&origin[0]==11&&scale==1);f.calls+='B';return 1;};
    b.apply_damage=[](void* p,void* e,const void*,unsigned n) noexcept ->int{auto& f=*static_cast<F*>(p);assert(e==f.enemy&&n==0x48);f.calls+='X';return 1;};
    ServerRecombinantPounceResult o{};f.put(f.actor,0xb8,0x800u);
    assert(api->off(f.actor,sizeof(f.actor),10,&b,&o)&&o.enemy_live&&o.random_calls==2&&o.normalized&&o.yaw==-100&&o.deadline==10.5f&&o.snapshot[2]==13&&f.get<float>(f.actor,0xc18)==3&&f.get<unsigned>(f.actor,0x7c)==recombinant_minor_detached_contact_marker&&f.calls=="MEDDNYAGVRIT");
    f.live=false;f.calls.clear();
    assert(api->off(f.actor,sizeof(f.actor),10,&b,&o)&&!o.enemy_live&&o.fallback.completed&&o.fallback.velocity[0]==350&&f.get<unsigned>(f.actor,0x7c)==recombinant_minor_contact_marker&&f.calls=="MEFGLVRIT");
    f.calls.clear();f.put(f.actor,0xb8,0x1800u);
    assert(api->slash(f.actor,sizeof(f.actor),f.enemy,&b,&o)&&o.damage_prepared&&o.force_calls==1&&o.damage_calls==1&&o.velocity_refreshes==2&&o.origin_refreshes==1&&f.calls=="PKRKBX");
    f.calls.clear();assert(!api->slash(f.actor,sizeof(f.actor),nullptr,&b,&o)&&f.calls.empty());
    FreeLibrary(server);FreeLibrary(client);return 0;
}
