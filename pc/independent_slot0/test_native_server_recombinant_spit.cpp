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
    alignas(4) unsigned char actor[0xc50]{},enemy[0x2d0]{};
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

    // Original 2237c3c3 calls +1d4 on its ENEMY. Only a verified
    // Recombinant Minor enemy may use the recovered 2237b420 override;
    // every other entity keeps its inherited target-point service.
    const auto get_class=reinterpret_cast<GetServerRecombinantSpitClassPointApi>(
        GetProcAddress(server,server_recombinant_spit_class_point_export));
    assert(get_class&&!GetProcAddress(client,server_recombinant_spit_class_point_export)&&
        !get_class(2,sizeof(ServerRecombinantSpitClassPointApi))&&
        !get_class(1,sizeof(ServerRecombinantSpitClassPointApi)-1));
    const auto* compose=get_class(1,sizeof(ServerRecombinantSpitClassPointApi));
    assert(compose);
    struct Host {F* fixture;bool short_extent=false,fail_refresh=false;};
    Host host{&f};
    ServerRecombinantMinorOriginPointBindings minor_point{&host,
        [](void* p,void* entity) noexcept ->int{
            auto& host=*static_cast<Host*>(p);
            auto& f=*host.fixture;assert(entity==f.enemy);
            f.calls+='M';
            if(host.fail_refresh)return 0;
            const float origin[]{10.f,20.f,24.f};
            std::memcpy(f.enemy+0x2bc,origin,sizeof(origin));
            return 1;
        }};
    ServerRecombinantSpitClassPointBindings cb{&b,&minor_point,&host,
        [](void* p,void* entity,unsigned* extent) noexcept ->int{
            auto& host=*static_cast<Host*>(p);auto& f=*host.fixture;
            assert(entity==f.enemy&&extent);f.calls+='Q';
            *extent=host.short_extent?3u:sizeof(f.enemy);
            return 1;
        }};
    ServerRecombinantSpitClassPointResult co{};
    f.put(0,recombinant_minor_vtable);f.put(0xb8,0u);
    f.live=true;f.alternate=b.alternate=true;b.gravity=600.f;
    auto enemy_put=[&](unsigned off,const auto& v){
        std::memcpy(f.enemy+off,&v,sizeof(v));
    };
    enemy_put(0,recombinant_minor_vtable);
    enemy_put(0xb8,0x800u);
    enemy_put(0x2bc,0.f);enemy_put(0x2c0,0.f);enemy_put(0x2c4,0.f);
    f.calls.clear();f.draws=0;
    assert(compose->execute(f.actor,sizeof(f.actor),40.f,&cb,&co)&&
        co.spit.completed&&co.spit.projectiles&&co.spit.effects&&
        co.point_calls==1&&co.minor_calls==1&&!co.delegated_calls&&
        co.minor_point.completed&&co.minor_point.refresh_calls==1&&
        co.minor_point.point[0]==10.f&&co.minor_point.point[2]==30.f&&
        co.spit.target[0]==10.f&&co.spit.target[1]==20.f&&co.spit.target[2]==20.f&&
        f.calls=="EAEPEQMBNRRRJXR");

    // Non-Minor enemy uses the original class-dependent provider.
    enemy_put(0,0x2249aaa4u);
    f.calls.clear();f.draws=0;
    assert(compose->execute(f.actor,sizeof(f.actor),41.f,&cb,&co)&&
        co.spit.completed&&co.point_calls==1&&!co.minor_calls&&
        co.delegated_calls==1&&f.calls=="EAEPEQTBNRRRJXR");

    // An undersized actor must not be dereferenced to read its vtable.
    enemy_put(0,recombinant_minor_vtable);
    host.short_extent=true;f.calls.clear();f.draws=0;
    assert(!compose->execute(f.actor,sizeof(f.actor),42.f,&cb,&co)&&
        !co.spit.completed&&!co.spit.projectiles&&co.point_calls==1&&
        !co.minor_calls&&!co.delegated_calls&&f.calls=="EAEPEQ");
    host.short_extent=false;
    // Dirty-origin provider rejection stops the projectile before any
    // ballistic computation; it never falls back to the generic point.
    enemy_put(0xb8,0x800u);
    host.fail_refresh=true;f.calls.clear();f.draws=0;
    assert(!compose->execute(f.actor,sizeof(f.actor),43.f,&cb,&co)&&
        !co.spit.completed&&!co.spit.projectiles&&co.minor_calls==1&&
        !co.delegated_calls&&co.minor_point.refresh_calls==1&&
        f.calls=="EAEPEQM");
    host.fail_refresh=false;f.calls.clear();
    auto invalid=cb;invalid.extent=nullptr;
    assert(!compose->execute(f.actor,sizeof(f.actor),44.f,&invalid,&co)&&
        !co.spit.completed&&f.calls.empty());
    f.put(0,recombinant_major_vtable);
    assert(!compose->execute(f.actor,sizeof(f.actor),44.f,&cb,&co)&&
        !co.spit.completed&&f.calls.empty());
    f.put(0,recombinant_minor_vtable);
    assert(!compose->execute(f.actor,0xc4f,44.f,&cb,&co)&&
        !co.spit.completed&&f.calls.empty());
    assert(!compose->execute(f.actor,sizeof(f.actor),44.f,&cb,nullptr));
    FreeLibrary(server);FreeLibrary(client);return 0;
}
