#include "native_server_mutant_spit_entity.h"
#include <cmath>
#include <cstring>
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T>void wr(void* p,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
bool finite(const float* v) noexcept{return std::isfinite(v[0])&&std::isfinite(v[1])&&std::isfinite(v[2]);}
bool identity(void* a,unsigned n) noexcept{return a&&n>=0x498&&rd<unsigned>(a,0)==mutant_spit_vtable;}
int __cdecl spawn(void* a,unsigned n,const ServerMutantSpitEntityBindings* b,ServerMutantSpitEntityResult* o) noexcept{
    if(!o)return 0;*o={};
    if(!identity(a,n)||!b)return 0;o->projectile=a;o->bytes=n;
    if(!b->precache||!b->precache(b->context,a)||!b->movement||!b->movement(b->context,a,4,0))return 0;
    o->movement_set=1;
    if(!b->particle_name||!b->particle_name(b->context,a,"spit_mutant")||
       !b->solid||!b->solid(b->context,a,2)||
       !b->model||!b->model(b->context,a,"models/Weapons/poison_spit/poison_spit.mdl"))return 0;
    const float mins[]{-2,-2,-2},maxs[]{2,2,2};
    if(!b->bounds||!b->bounds(b->context,a,mins,maxs))return 0;o->bounds_set=1;
    if(!b->collision_group||!b->collision_group(b->context,a,19))return 0;o->group_set=1;
    wr(a,0x494,0.f);o->damage_written=o->completed=1;return 1;
}
int __cdecl create_normal(void* a,unsigned n,const ServerRecombinantSpitRequest* q,const ServerMutantSpitEntityBindings* b,ServerMutantSpitEntityResult* o) noexcept{
    if(!o)return 0;*o={};
    if(!a||n<0x360||rd<unsigned>(a,0)!=recombinant_minor_vtable||!q||q->alternate||!b||
       !finite(q->muzzle)||!finite(q->velocity)||!std::isfinite(q->damage))return 0;
    if(!b->create||!b->create(b->context,"gibspit",-1,&o->projectile,&o->bytes)||!identity(o->projectile,o->bytes))return 0;
    o->created=1;void* e=o->projectile;
    if(!b->dispatch_spawn||!b->dispatch_spawn(b->context,e))return 0;o->spawned=1;
    if(!identity(e,o->bytes))return 0;
    wr(e,0x494,q->damage);o->damage_written=1;
    if(!b->relocate||!b->relocate(b->context,e,q->muzzle,0))return 0;o->relocated=1;
    if(rd<unsigned>(a,0xb8)&0x800u){
        if(!b->refresh_origin||!b->refresh_origin(b->context,a))return 0;++o->origin_refreshes;
    }
    float angles[3]{};std::memcpy(angles,static_cast<unsigned char*>(a)+0x354,12);
    if(!finite(angles)||!b->angles||!b->angles(b->context,e,angles))return 0;o->angles_set=1;
    if(!b->velocity||!b->velocity(b->context,e,q->velocity))return 0;o->velocity_set=1;
    if(!b->owner||!b->owner(b->context,e,a))return 0;o->owner_set=o->completed=1;return 1;
}
int __cdecl touch(void* a,unsigned n,void* other,unsigned other_n,const ServerMutantSpitEntityBindings* b,ServerMutantSpitEntityResult* o) noexcept{
    if(!o)return 0;*o={};o->sound_draw=-1;
    if(!identity(a,n)||!other||other_n<0x218||!b)return 0;
    o->projectile=a;o->bytes=n;
    unsigned flags=0;if(!b->contact_flags||!b->contact_flags(b->context,other,&flags))return 0;
    if((flags&8u)||rd<unsigned>(other,0x214)==19){o->ignored=o->completed=1;return 1;}
    // The original pitch draw is discarded, but advances the shared RNG.
    if(!b->random_float||!b->random_float(b->context,90,110,&o->pitch_draw)||!std::isfinite(o->pitch_draw)||o->pitch_draw<90||o->pitch_draw>110)return 0;
    ++o->random_calls;
    auto sound=[&](const char* name){if(!b->sound||!b->sound(b->context,a,name,0,0))return false;++o->sounds;return true;};
    if(!sound("NPC_BigMomma.SpitTouch1"))return 0;
    if(!b->random_int||!b->random_int(b->context,0,1,&o->sound_draw)||o->sound_draw<0||o->sound_draw>1)return 0;
    ++o->random_calls;if(!sound(o->sound_draw?"NPC_BigMomma.SpitHit2":"NPC_BigMomma.SpitHit1"))return 0;
    auto origin=[&](){if(!(rd<unsigned>(a,0xb8)&0x800u))return true;if(!b->refresh_origin||!b->refresh_origin(b->context,a))return false;++o->origin_refreshes;return true;};
    auto velocity=[&](){if(!(rd<unsigned>(a,0xb8)&0x1000u))return true;if(!b->refresh_velocity||!b->refresh_velocity(b->context,a))return false;++o->velocity_refreshes;return true;};
    const bool damageable=rd<unsigned>(other,0x94)!=0;
    if(damageable){
        o->impact.damage=rd<float>(a,0x494);o->impact.damage_type=2;
        if(!std::isfinite(o->impact.damage)||!origin()||!velocity())return 0;
    }else if(!velocity()||!origin()||!origin())return 0;
    std::memcpy(o->impact.origin,static_cast<unsigned char*>(a)+0x2bc,12);
    std::memcpy(o->impact.velocity,static_cast<unsigned char*>(a)+0x250,12);
    if(!finite(o->impact.origin)||!finite(o->impact.velocity))return 0;
    if(damageable){
        if(!b->damage_impact||!b->damage_impact(b->context,a,other,&o->impact,"BUCK",1.f))return 0;o->damage_impact=1;
    }else{
        if(!b->world_impact||!b->world_impact(b->context,a,&o->impact,10.f,0x200400bu,0,"MutantSpit"))return 0;o->world_impact=1;
    }
    if(!b->remove||!b->remove(b->context,a))return 0;o->removed=o->completed=1;return 1;
}
const ServerMutantSpitEntityApi api{1,sizeof(ServerMutantSpitEntityApi),&spawn,&create_normal,&touch};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerMutantSpitEntityApi=_SiN_GetServerMutantSpitEntityApi")
extern "C" const sin_port::native_weapon_tables::ServerMutantSpitEntityApi* __cdecl SiN_GetServerMutantSpitEntityApi(unsigned v,unsigned n) noexcept{
    using namespace sin_port::native_weapon_tables;return v==1&&n==sizeof(ServerMutantSpitEntityApi)?&api:nullptr;
}
