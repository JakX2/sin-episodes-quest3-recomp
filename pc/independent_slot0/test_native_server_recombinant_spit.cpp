#include "native_server_recombinant_spit.h"
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
    bool live=true,alternate=false;unsigned draws=0;std::string calls;
    template<class T>void put(unsigned off,T v){std::memcpy(actor+off,&v,sizeof(v));}
    template<class T>T get(unsigned off){T v{};std::memcpy(&v,actor+off,sizeof(v));return v;}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    const auto get=reinterpret_cast<GetServerRecombinantSpitApi>(GetProcAddress(server,server_recombinant_spit_export));
    assert(get&&!GetProcAddress(client,server_recombinant_spit_export)&&!get(2,sizeof(ServerRecombinantSpitApi)));
    const auto* api=get(1,sizeof(ServerRecombinantSpitApi));assert(api);
    F f;f.put(0,recombinant_minor_vtable);f.put(0xc44,.3f);f.put(0xc48,91u);f.put(0xc4c,92u);f.put(0xb8,0x800u);
    ServerRecombinantEventBindings e{};e.context=&f;
    e.enemy=[](void* p,void*,void** out) noexcept ->int{auto& f=*static_cast<F*>(p);f.calls+='E';*out=f.live?f.enemy:nullptr;return 1;};
    e.target_point=[](void* p,void* a,float v[3]) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.enemy);f.calls+='T';v[0]=10;v[1]=20;v[2]=30;return 1;};
    e.random=[](void* p,float lo,float hi,float* out) noexcept ->int{auto& f=*static_cast<F*>(p);f.calls+='R';unsigned i=f.draws++;assert(i<4);if(i==3){assert(lo==1.5f&&hi==2);*out=1.75f;}else{assert(lo==-.05f&&hi==(i==2?0.f:.05f));*out=i==1?.05f:0.f;}return 1;};
    ServerRecombinantSpitBindings b{&e,&f,.5f,17.5f,false};
    b.attachment=[](void* p,void*,const char* name,float v[3]) noexcept ->int{auto& f=*static_cast<F*>(p);assert(!std::strcmp(name,"Acid_spit"));f.calls+='A';v[0]=1;v[1]=2;v[2]=3;return 1;};
    b.refresh_origin=[](void* p,void*) noexcept ->int{auto& f=*static_cast<F*>(p);f.calls+='U';f.put(0x2bc,4.f);return 1;};
    b.aim_point=[](void* p,void* a,const float origin[3],int mode,float v[3]) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.enemy&&origin[0]==4&&mode==1);f.calls+='P';v[0]=5;v[1]=2;v[2]=3;return 1;};
    b.ballistic=[](void* p,const float origin[3],const float target[3],float g,float mode,float v[3]) noexcept ->int{auto& f=*static_cast<F*>(p);assert(origin[0]==1&&origin[2]==3&&target[0]==10&&target[2]==20&&g==(f.alternate?600.f:1.f)&&mode==1);f.calls+='B';v[0]=30;v[1]=40;v[2]=50;return 1;};
    b.normalize=[](void* p,float v[3]) noexcept ->int{auto& f=*static_cast<F*>(p);assert(v[0]==4&&!v[1]&&!v[2]);f.calls+='N';v[0]=1;return 1;};
    b.projectile=[](void* p,void*,const ServerRecombinantSpitRequest* q) noexcept ->int{auto& f=*static_cast<F*>(p);assert(q->damage==17.5f&&q->alternate==f.alternate&&q->muzzle[2]==3&&!q->angular[0]&&!q->angular[1]&&!q->angular[2]);if(q->alternate)assert(q->velocity[0]==30&&q->velocity[2]==50);else assert(q->velocity[0]==900&&q->velocity[1]==45&&!q->velocity[2]);f.calls+='J';return 1;};
    b.effects=[](void* p,void*,const ServerRecombinantSpitEffect* q) noexcept ->int{auto& f=*static_cast<F*>(p);assert(q->muzzle[0]==1&&q->ballistic[0]==30&&q->first_token==91&&q->second_token==92);f.calls+='X';return 1;};
    ServerRecombinantSpitResult o{};
    assert(api->execute(f.actor,sizeof(f.actor),10,&b,&o)&&o.projectiles==1&&o.effects==1&&o.random_calls==4&&o.gravity==1&&o.deadline==11.75f&&o.probability==.5f&&f.calls=="EAEUPETBNRRRJXR");
    f.calls.clear();f.draws=0;f.alternate=b.alternate=true;b.gravity=600;f.put(0xb8,0u);
    assert(api->execute(f.actor,sizeof(f.actor),20,&b,&o)&&o.request.alternate&&o.deadline==21.75f&&o.probability==.6f&&f.calls=="EAEPETBNRRRJXR");
    f.calls.clear();f.draws=0;f.live=false;assert(api->execute(f.actor,sizeof(f.actor),30,&b,&o)&&!o.enemy_live&&!o.projectiles&&!o.deadline_written&&f.get<float>(0xc3c)==21.75f&&f.calls=="E");
    f.put(0,recombinant_major_vtable);assert(!api->execute(f.actor,sizeof(f.actor),30,&b,&o)&&!o.completed);
    FreeLibrary(server);FreeLibrary(client);return 0;
}
