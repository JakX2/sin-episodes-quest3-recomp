#include "native_server_recombinant_major_events.h"
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
    alignas(4) unsigned char actor[0xc70]{},enemy[4]{};
    bool live=true,player=true,hit=true;unsigned draws=0;std::string calls;
    template<class T>void put(unsigned off,T v){std::memcpy(actor+off,&v,sizeof(v));}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    const auto get=reinterpret_cast<GetServerRecombinantMajorEventsApi>(GetProcAddress(server,server_recombinant_major_events_export));
    assert(get&&!GetProcAddress(client,server_recombinant_major_events_export)&&!get(2,sizeof(ServerRecombinantMajorEventsApi)));
    const auto* api=get(1,sizeof(ServerRecombinantMajorEventsApi));assert(api);
    ServerEventDictionaryEntry names[16]{};ServerEventDictionary d{names,16,0,1,23};ServerRecombinantMajorEventRegistration reg{};
    assert(api->register_major(&d,&reg)&&api->valid(&reg)&&d.count==7);
    for(int i=0;i<7;++i)assert(reg.ids[i]==24+i);
    F f;f.put(0,recombinant_major_vtable);f.put(0x36c,10.f);f.put(0x370,20.f);f.put(0x374,30.f);
    ServerRecombinantPolicyBindings policy{};policy.context=&f;
    policy.random=[](void* p,int lo,int hi,int* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(lo==-2&&hi==2);f.calls+='R';*out=++f.draws==1?-2:1;return 1;};
    ServerRecombinantMajorEventBindings b{&policy,&reg,&f,12.75f,27.5f};
    b.basis=[](void* p,const float a[3],float forward[3],float right[3]) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a[0]==10&&a[1]==20&&a[2]==30);f.calls+='B';forward[0]=1;forward[1]=0;forward[2]=0;right[0]=0;right[1]=-1;right[2]=0;return 1;};
    b.forward=[](void* p,const float a[3],float v[3]) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a[0]==10&&a[1]==20&&a[2]==30);f.calls+='F';v[0]=1;v[1]=0;v[2]=0;return 1;};
    b.enemy=[](void* p,void*,void** out) noexcept ->int{auto& f=*static_cast<F*>(p);f.calls+='E';*out=f.live?f.enemy:nullptr;return 1;};
    b.is_player=[](void* p,void* e,bool* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(e==f.enemy);f.calls+='P';*out=f.player;return 1;};
    b.melee=[](void* p,void* a,const ServerRecombinantMajorMeleeRequest* q,void** out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor&&q->reach==(f.live&&f.player?85:65)&&q->impulse[0]==200);f.calls+='M';*out=f.hit?f.enemy:nullptr;return 1;};
    b.parent=[](void* p,void*,const void* e,unsigned n) noexcept ->int{assert(n==4&&*static_cast<const int*>(e)==99);static_cast<F*>(p)->calls+='U';return 1;};
    ServerRecombinantMajorEventResult o{};
    auto call=[&](int i){f.calls.clear();int event=i<0?99:reg.ids[i];return api->major(f.actor,sizeof(f.actor),&event,4,&b,&o);};
    for(int i=0;i<6;++i){
        const bool right=(i%2)==0;
        assert(call(i)&&o.custom&&o.hit&&o.basis_calls==1&&!o.random_calls&&o.request.side==(right?1:0)&&o.request.damage==(i==2||i==3?27:12));
        assert(o.request.angular[0]==-3&&o.request.angular[1]==(right?-5:5)&&o.request.angular[2]==-2&&o.request.impulse[1]==(right?-100:100)&&f.calls=="BEPM");
    }
    assert(call(6)&&o.request.side==2&&o.request.damage==25&&o.request.angular[0]==5&&o.request.angular[1]==1&&o.request.angular[2]==-2&&o.request.impulse[1]==0&&f.calls=="RRFEPM");
    f.player=false;f.hit=false;assert(call(0)&&o.request.reach==65&&!o.hit&&f.calls=="BEPM");
    f.live=false;assert(call(0)&&o.request.reach==65&&!o.player_calls&&f.calls=="BEM");
    assert(call(-1)&&o.delegated&&f.calls=="U");
    ++d.generation;assert(!call(0)&&!o.completed);
    FreeLibrary(server);FreeLibrary(client);return 0;
}
