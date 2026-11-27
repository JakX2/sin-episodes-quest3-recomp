#include "native_server_mutant_spit_entity.h"
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
    alignas(4) unsigned char actor[0xc70]{},projectile[0x498]{},other[0x220]{};
    unsigned flags=0;int draw=0;std::string calls;
    template<class T>void put(void* a,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(a)+off,&v,sizeof(v));}
    template<class T>T get(void* a,unsigned off){T v{};std::memcpy(&v,static_cast<unsigned char*>(a)+off,sizeof(v));return v;}
    const ServerMutantSpitEntityApi* api=nullptr;const ServerMutantSpitEntityBindings* b=nullptr;
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    const auto get=reinterpret_cast<GetServerMutantSpitEntityApi>(GetProcAddress(server,server_mutant_spit_entity_export));
    assert(get&&!GetProcAddress(client,server_mutant_spit_entity_export)&&!get(2,sizeof(ServerMutantSpitEntityApi)));
    F f;f.api=get(1,sizeof(ServerMutantSpitEntityApi));assert(f.api);
    f.put(f.actor,0,recombinant_minor_vtable);f.put(f.projectile,0,mutant_spit_vtable);f.put(f.actor,0xb8,0x800u);
    ServerMutantSpitEntityBindings b{};b.context=&f;f.b=&b;
    b.create=[](void* p,const char* name,int index,void** out,unsigned* n) noexcept ->int{auto& f=*static_cast<F*>(p);assert(!std::strcmp(name,"gibspit")&&index==-1);f.calls+='C';*out=f.projectile;*n=sizeof(f.projectile);return 1;};
    b.dispatch_spawn=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.projectile);f.calls+='S';ServerMutantSpitEntityResult o{};return f.api->spawn(a,sizeof(f.projectile),f.b,&o);};
    b.precache=[](void* p,void*) noexcept ->int{static_cast<F*>(p)->calls+='P';return 1;};
    b.movement=[](void* p,void*,int type,int mode) noexcept ->int{assert(type==4&&!mode);static_cast<F*>(p)->calls+='M';return 1;};
    b.entity_name=[](void* p,void*,const char* name) noexcept ->int{assert(!std::strcmp(name,"spit_mutant"));static_cast<F*>(p)->calls+='N';return 1;};
    b.solid=[](void* p,void*,int type) noexcept ->int{assert(type==2);static_cast<F*>(p)->calls+='D';return 1;};
    b.model=[](void* p,void*,const char* name) noexcept ->int{assert(!std::strcmp(name,"models/Weapons/poison_spit/poison_spit.mdl"));static_cast<F*>(p)->calls+='L';return 1;};
    b.bounds=[](void* p,void*,const float a[3],const float b[3]) noexcept ->int{for(int i=0;i<3;++i)assert(a[i]==-2&&b[i]==2);static_cast<F*>(p)->calls+='B';return 1;};
    b.collision_group=[](void* p,void* a,int id) noexcept ->int{auto& f=*static_cast<F*>(p);assert(id==19);f.put(a,0x214,unsigned(id));f.calls+='G';return 1;};
    b.relocate=[](void* p,void* a,const float v[3],int mode) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.projectile&&v[0]==1&&v[2]==3&&!mode&&f.get<float>(a,0x494)==17.5f);std::memcpy(f.projectile+0x2bc,v,12);f.calls+='O';return 1;};
    b.refresh_origin=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<F*>(p);if(a==f.actor)f.put(a,0x354,40.f);f.calls+='R';return 1;};
    b.refresh_velocity=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.projectile);f.calls+='V';return 1;};
    b.angles=[](void* p,void*,const float v[3]) noexcept ->int{assert(v[0]==40);static_cast<F*>(p)->calls+='A';return 1;};
    b.velocity=[](void* p,void* a,const float v[3]) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.projectile&&v[0]==90&&v[2]==10);std::memcpy(f.projectile+0x250,v,12);f.calls+='J';return 1;};
    b.owner=[](void* p,void* a,void* owner) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.projectile&&owner==f.actor);f.calls+='W';return 1;};
    b.contact_flags=[](void* p,void*,unsigned* out) noexcept ->int{auto& f=*static_cast<F*>(p);f.calls+='F';*out=f.flags;return 1;};
    b.random_float=[](void* p,float lo,float hi,float* out) noexcept ->int{assert(lo==90&&hi==110);static_cast<F*>(p)->calls+='Q';*out=100;return 1;};
    b.random_int=[](void* p,int lo,int hi,int* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(!lo&&hi==1);f.calls+='I';*out=f.draw;return 1;};
    b.sound=[](void* p,void*,const char* name,int a,int b) noexcept ->int{auto& f=*static_cast<F*>(p);assert(!a&&!b);if(f.calls.back()=='Q')assert(!std::strcmp(name,"NPC_BigMomma.SpitTouch1"));else assert(!std::strcmp(name,f.draw?"NPC_BigMomma.SpitHit2":"NPC_BigMomma.SpitHit1"));f.calls+='H';return 1;};
    b.world_impact=[](void* p,void* a,const ServerMutantSpitImpact* q,float scale,unsigned mask,int group,const char* decal) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.projectile&&q->origin[0]==1&&q->velocity[0]==90&&scale==10&&mask==0x200400b&&!group&&!std::strcmp(decal,"MutantSpit"));f.calls+='E';return 1;};
    b.damage_impact=[](void* p,void* a,void* other,const ServerMutantSpitImpact* q,const char* ammo,float scale) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.projectile&&other==f.other&&q->damage==17.5f&&q->damage_type==2&&q->origin[0]==1&&q->velocity[2]==10&&!std::strcmp(ammo,"BUCK")&&scale==1);f.calls+='K';return 1;};
    b.remove=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.projectile);f.calls+='X';return 1;};
    ServerMutantSpitEntityResult o{};ServerRecombinantSpitRequest q{{1,2,3},{90,0,10},{0,0,0},17.5f,false};
    assert(f.api->create_normal(f.actor,sizeof(f.actor),&q,&b,&o)&&o.owner_set&&o.damage_written&&f.calls=="CSPMNDLBGORAJW");
    auto touch=[&](){f.calls.clear();return f.api->touch(f.projectile,sizeof(f.projectile),f.other,sizeof(f.other),&b,&o);};
    f.flags=8;assert(touch()&&o.ignored&&!o.removed&&f.calls=="F");
    f.flags=0;f.put(f.other,0x214,19u);assert(touch()&&o.ignored&&f.calls=="F");f.put(f.other,0x214,0u);
    f.put(f.projectile,0xb8,0x1800u);
    assert(touch()&&o.world_impact&&o.origin_refreshes==2&&o.velocity_refreshes==1&&o.removed&&f.calls=="FQHIHVRREX");
    f.draw=1;f.put(f.other,0x94,1u);
    assert(touch()&&o.damage_impact&&o.origin_refreshes==1&&o.velocity_refreshes==1&&o.removed&&f.calls=="FQHIHRVKX");
    f.calls.clear();q.alternate=true;assert(!f.api->create_normal(f.actor,sizeof(f.actor),&q,&b,&o)&&f.calls.empty());
    f.put(f.projectile,0,recombinant_minor_vtable);assert(!touch());
    FreeLibrary(server);FreeLibrary(client);return 0;
}
