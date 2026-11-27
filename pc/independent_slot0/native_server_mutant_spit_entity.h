#pragma once
#include "native_server_recombinant_spit.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_mutant_spit_entity_export[]="SiN_GetServerMutantSpitEntityApi";
inline constexpr unsigned mutant_spit_vtable=0x224a1914u;
struct ServerMutantSpitImpact {
    float origin[3],velocity[3],damage;
    unsigned damage_type;
};
struct ServerMutantSpitEntityBindings {
    void* context;
    int (__cdecl *create)(void*,const char*,int,void**,unsigned*) noexcept;
    int (__cdecl *dispatch_spawn)(void*,void*) noexcept;
    int (__cdecl *precache)(void*,void*) noexcept;
    int (__cdecl *movement)(void*,void*,int,int) noexcept;
    int (__cdecl *particle_name)(void*,void*,const char*) noexcept; // original 22138160, name only
    int (__cdecl *solid)(void*,void*,int) noexcept;
    int (__cdecl *model)(void*,void*,const char*) noexcept;
    int (__cdecl *bounds)(void*,void*,const float[3],const float[3]) noexcept;
    int (__cdecl *collision_group)(void*,void*,int) noexcept;
    int (__cdecl *relocate)(void*,void*,const float[3],int) noexcept;
    int (__cdecl *refresh_origin)(void*,void*) noexcept;
    int (__cdecl *refresh_velocity)(void*,void*) noexcept;
    int (__cdecl *angles)(void*,void*,const float[3]) noexcept;
    int (__cdecl *velocity)(void*,void*,const float[3]) noexcept;
    int (__cdecl *owner)(void*,void*,void*) noexcept;
    int (__cdecl *contact_flags)(void*,void*,unsigned*) noexcept;
    int (__cdecl *random_float)(void*,float,float,float*) noexcept;
    int (__cdecl *random_int)(void*,int,int,int*) noexcept;
    int (__cdecl *sound)(void*,void*,const char*,int,int) noexcept;
    // Owner executes the inherited trace/decal or damage-info services.
    int (__cdecl *world_impact)(void*,void*,const ServerMutantSpitImpact*,float,unsigned,int,const char*) noexcept;
    int (__cdecl *damage_impact)(void*,void*,void*,const ServerMutantSpitImpact*,const char*,float) noexcept;
    int (__cdecl *remove)(void*,void*) noexcept;
};
struct ServerMutantSpitEntityResult {
    unsigned completed,created,spawned,damage_written,movement_set,bounds_set,group_set,
        relocated,origin_refreshes,velocity_refreshes,angles_set,velocity_set,owner_set,
        ignored,random_calls,sounds,world_impact,damage_impact,removed;
    void* projectile;unsigned bytes;
    float pitch_draw;int sound_draw;
    ServerMutantSpitImpact impact;
};
struct ServerMutantSpitEntityApi {
    unsigned version,size;
    int (__cdecl *spawn)(void*,unsigned,const ServerMutantSpitEntityBindings*,ServerMutantSpitEntityResult*) noexcept;
    int (__cdecl *create_normal)(void*,unsigned,const ServerRecombinantSpitRequest*,const ServerMutantSpitEntityBindings*,ServerMutantSpitEntityResult*) noexcept;
    int (__cdecl *touch)(void*,unsigned,void*,unsigned,const ServerMutantSpitEntityBindings*,ServerMutantSpitEntityResult*) noexcept;
};
using GetServerMutantSpitEntityApi=const ServerMutantSpitEntityApi* (__cdecl *)(unsigned,unsigned) noexcept;
}
