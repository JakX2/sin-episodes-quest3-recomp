#include "native_server_recombinant_events.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <cmath>
#include <string>
using namespace sin_port::native_weapon_tables;
struct F{
    alignas(4) unsigned char actor[0xc50]{},enemy[4]{};
    sin_independent::EntityHandleEntry entries[2]{};
    bool live=true,special=true;unsigned draws=0;std::string calls;
    template<class T>void put(unsigned off,T v){std::memcpy(actor+off,&v,sizeof(v));}
    template<class T>T get(unsigned off){T v{};std::memcpy(&v,actor+off,sizeof(v));return v;}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    const auto get=reinterpret_cast<GetServerRecombinantEventsApi>(GetProcAddress(server,server_recombinant_events_export));
    assert(get&&!GetProcAddress(client,server_recombinant_events_export)&&!get(2,sizeof(ServerRecombinantEventsApi)));
    const auto* api=get(1,sizeof(ServerRecombinantEventsApi));assert(api);
    ServerEventDictionaryEntry names[16]{};ServerEventDictionary d{names,16,0,1,23};ServerRecombinantEventRegistration reg{};
    assert(api->register_minor(&d,&reg)&&api->valid(&reg)&&d.count==5);
    for(int i=0;i<5;++i)assert(reg.ids[i]==24+i);
    ServerRecombinantEventRegistration again{};assert(api->register_minor(&d,&again)&&d.count==5);
    F f;f.put(0,recombinant_minor_vtable);f.put(0xc44,.7f);f.entries[1].entity=reinterpret_cast<CBaseEntity*>(f.enemy);f.entries[1].serial=7;
    ServerRecombinantPolicyBindings policy{};policy.entries=f.entries;policy.entry_count=2;
    ServerRecombinantEventBindings b{&policy,&reg,&f};
    b.enemy=[](void* p,void*,void** out) noexcept ->int{auto& f=*static_cast<F*>(p);f.calls+='E';*out=f.live?f.enemy:nullptr;return 1;};
    b.special=[](void* p,void*,bool* out) noexcept ->int{auto& f=*static_cast<F*>(p);f.calls+='S';*out=f.special;return 1;};
    b.target_point=[](void* p,void*,float v[3]) noexcept ->int{static_cast<F*>(p)->calls+='T';v[0]=1;v[1]=2;v[2]=3;return 1;};
    b.center=[](void* p,void*,float v[3]) noexcept ->int{static_cast<F*>(p)->calls+='C';v[0]=4;v[1]=5;v[2]=6;return 1;};
    b.player_probe=[](void* p,void*) noexcept ->int{static_cast<F*>(p)->calls+='P';return 1;};
    b.launch=[](void* p,void*,int mode,const float v[3],int arg) noexcept ->int{auto& f=*static_cast<F*>(p);assert(!arg);if(mode)assert(!v[0]&&!v[1]&&!v[2]);f.calls+='L';return 1;};
    b.pounce_slash=[](void* p,void*,void* e) noexcept ->int{auto& f=*static_cast<F*>(p);assert(e==f.enemy);f.calls+='A';return 1;};
    b.random=[](void* p,float lo,float hi,float* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(lo==5&&hi==10);f.calls+='R';*out=++f.draws==1?6.f:9.f;return 1;};
    b.angular_effect=[](void* p,void* e,const float v[3]) noexcept ->int{auto& f=*static_cast<F*>(p);assert(e==f.enemy&&v[0]==9&&v[1]==6&&v[2]==0);f.calls+='G';return 1;};
    b.spit=[](void* p,void*) noexcept ->int{static_cast<F*>(p)->calls+='I';return 1;};
    b.pounce_off=[](void* p,void*) noexcept ->int{static_cast<F*>(p)->calls+='O';return 1;};
    b.parent=[](void* p,void*,const void* e,unsigned n) noexcept ->int{assert(n==4&&*static_cast<const int*>(e)==99);static_cast<F*>(p)->calls+='B';return 1;};
    ServerRecombinantEventResult o{};
    auto call=[&](int i){f.calls.clear();int event=i<0?99:reg.ids[i];return api->minor(f.actor,sizeof(f.actor),&event,4,&b,&o);};
    assert(call(1)&&o.cache_written&&o.enemy_queries==3&&o.point[2]==3&&f.get<unsigned char>(0xc40)&&f.calls=="EEESTP");
    assert(call(0)&&o.cache_consumed&&o.point[0]==1&&o.probability==.5f&&!f.get<unsigned char>(0xc40)&&f.calls=="EL");
    assert(call(0)&&!o.cache_consumed&&o.probability==.4f&&f.calls=="ETL");
    f.special=false;assert(call(1)&&o.point[2]==16&&f.calls=="EEESCP");
    f.live=false;assert(call(0)&&o.launch_calls&&f.get<unsigned char>(0xc40)&&f.calls=="EL");
    assert(call(1)&&!o.cache_written&&f.calls=="E");f.live=true;
    f.put<unsigned char>(0xc41,1);assert(call(0)&&o.attached_blocked&&!o.launch_calls&&f.calls.empty());f.put<unsigned char>(0xc41,0);
    f.put(0xc30,0x7001u);assert(call(3)&&o.slash_calls&&o.random_calls==2&&o.angular_calls&&f.calls=="ARRG");
    f.put(0xc30,0x8001u);assert(call(3)&&!o.target_live&&!o.random_calls&&f.calls.empty());
    assert(call(2)&&o.spit_calls&&f.calls=="I");assert(call(4)&&o.off_calls&&f.calls=="O");assert(call(-1)&&o.delegated&&f.calls=="B");
    ++d.generation;assert(!call(0)&&!o.completed);
    FreeLibrary(server);FreeLibrary(client);return 0;
}
