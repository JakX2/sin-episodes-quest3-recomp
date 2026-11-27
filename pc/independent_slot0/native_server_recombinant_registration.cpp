#include "native_server_recombinant_registration.h"
#include <cstring>
extern "C" const sin_port::native_weapon_tables::ServerActivityDictionaryApi* __cdecl SiN_GetServerActivityDictionaryApi(unsigned,unsigned) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
constexpr ServerRecombinantActivityDefinition major[]{
    {"ACT_FLY_ATTACK",0x2259d1d0u,0x2237a519u},
    {"ACT_FRENZY",0x2259d1ccu,0x2237a533u},
    {"ACT_BIG_SLASH",0x2259d1c8u,0x2237a54du},
    {"ACT_BITE",0x2259d1c4u,0x2237a567u},
    {"ACT_IDLE_BLUNT_WEAPON",0x2259d1c0u,0x2237a581u},
    {"ACT_WALK_BLUNT_WEAPON",0x2259d1bcu,0x2237a59eu},
    {"ACT_RUN_BLUNT_WEAPON",0x2259d1b8u,0x2237a5b8u},
};
constexpr ServerRecombinantActivityDefinition minor[]{
    {"ACT_LEAP_ATTACK",0x2259d45cu,0x2237df46u},
    {"ACT_FLY_ATTACK",0x2259d458u,0x2237df60u},
    {"ACT_FLY_LAND",0x2259d454u,0x2237df7au},
    {"ACT_HIT_FLYING",0x2259d450u,0x2237df94u},
    {"ACT_WALK_BACK",0x2259d44cu,0x2237dfaeu},
    {"ACT_SPIT",0x2259d448u,0x2237dfcbu},
    {"ACT_FLY_ATTACK_POUNCE_OFF",0x2259d444u,0x2237dfe5u},
    {"ACT_FLY_AWAY",0x2259d440u,0x2237dfffu},
    {"ACT_FLY_LAND_AWAY",0x2259d43cu,0x2237e019u},
    {"ACT_SIDESTEP",0x2259d438u,0x2237e033u},
};
int __cdecl definitions(unsigned kind,const ServerRecombinantActivityDefinition** out,unsigned* n) noexcept{
    if(!out||!n)return 0;*out=nullptr;*n=0;
    if(kind==RecombinantMajor){*out=major;*n=7;return 1;}
    if(kind==RecombinantMinor){*out=minor;*n=10;return 1;}return 0;
}
int __cdecl valid(const ServerRecombinantActivityRegistration* r) noexcept{
    if(!r||!r->ready||!r->owner||r->generation!=r->owner->names.generation)return 0;
    const auto* api=SiN_GetServerActivityDictionaryApi(1,sizeof(ServerActivityDictionaryApi));
    const ServerRecombinantActivityDefinition* defs=nullptr;unsigned count=0;
    if(!api||!definitions(r->kind,&defs,&count)||count!=r->count)return 0;
    for(unsigned i=0;i<count;++i){
        int id=-1;if(!api->lookup(r->owner,defs[i].name,&id)||id<0||id!=r->ids[i])return 0;
        // The native dictionary must retain private, not shared, identity.
        bool found=false;for(unsigned j=0;j<r->owner->names.count;++j){
            const auto& e=r->owner->names.entries[j];
            if(e.id==id){found=e.private_event;break;}
        }
        if(!found)return 0;
    }
    return 1;
}
int __cdecl register_activities(unsigned kind,ServerActivityDictionary* d,const ServerRecombinantRegistrationBindings* b,ServerRecombinantActivityRegistration* out) noexcept{
    if(!out)return 0;*out={};for(auto& id:out->ids)id=-1;
    const auto* api=SiN_GetServerActivityDictionaryApi(1,sizeof(ServerActivityDictionaryApi));
    const ServerRecombinantActivityDefinition* defs=nullptr;unsigned count=0;
    if(!api||!d||!definitions(kind,&defs,&count))return 0;
    out->kind=kind;out->owner=d;out->generation=d->names.generation;
    for(unsigned i=0;i<count;++i){
        if(!api->private_id(d,defs[i].name,&out->ids[i]))return 0;
        ++out->count;
        if(b&&b->publish)b->publish(b->context,defs+i,out->ids[i]);
        if(out->generation!=d->names.generation)return 0;
    }
    out->ready=1;if(!valid(out)){out->ready=0;return 0;}return 1;
}
int __cdecl bind_minor(const ServerRecombinantActivityRegistration* r,const ServerRecombinantMinorTaskBindings* b,ServerRecombinantMinorTaskBindings* out) noexcept{
    if(!out||!b||!r||r->kind!=RecombinantMinor||!valid(r)||!b->task||!b->task->decision||
       !b->task->decision->policy||b->task->decision->policy->activities!=r->owner)return 0;
    auto next=*b;
    next.custom_activity=r->ids[3];next.arrival_activity=r->ids[4];*out=next;return 1;
}
const ServerRecombinantRegistrationApi api{1,sizeof(ServerRecombinantRegistrationApi),&definitions,&register_activities,&valid,&bind_minor};

constexpr const char* major_sounds[]{
    "NPC_Leperkin.Idle",
    "NPC_Leperkin.Run",
    "NPC_Leperkin.WalkFast"
};
constexpr const char* minor_sounds[]{
    "NPC_gib_hopper.Idle",
    "NPC_gib_hopper.Walk",
    "NPC_gib_hopper.WalkBackwards",
    "NPC_gib_hopper.Run"
};
int __cdecl sound_precache(void* actor,unsigned bytes,const ServerRecombinantSoundPrecacheBindings* b,
    ServerRecombinantSoundPrecacheResult* out) noexcept {
    if(!out)return 0;*out={};out->failed_index=-1;
    if(!actor||bytes<4||!b||!b->precache)return 0;
    unsigned vt=0;std::memcpy(&vt,actor,4);
    const char* const* names=nullptr;
    unsigned count=0;
    if(vt==recombinant_major_vtable){
        out->kind=RecombinantMajor;
        names=major_sounds;count=sizeof(major_sounds)/sizeof(*major_sounds);
    }else if(vt==recombinant_minor_vtable){
        out->kind=RecombinantMinor;
        names=minor_sounds;count=sizeof(minor_sounds)/sizeof(*minor_sounds);
    }else return 0;
    for(unsigned i=0;i<count;++i){
        out->failed_index=static_cast<int>(i);
        ++out->attempted;
        if(!b->precache(b->context,actor,names[i]))return 0;
        ++out->registered;
    }
    out->failed_index=-1;out->completed=1;return 1;
}
const ServerRecombinantSoundPrecacheApi sounds_api{
    1,sizeof(ServerRecombinantSoundPrecacheApi),&sound_precache
};

}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantRegistrationApi=_SiN_GetServerRecombinantRegistrationApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantRegistrationApi* __cdecl SiN_GetServerRecombinantRegistrationApi(unsigned v,unsigned n) noexcept{
    using namespace sin_port::native_weapon_tables;return v==1&&n==sizeof(ServerRecombinantRegistrationApi)?&api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantSoundPrecacheApi=_SiN_GetServerRecombinantSoundPrecacheApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantSoundPrecacheApi* __cdecl
SiN_GetServerRecombinantSoundPrecacheApi(unsigned v,unsigned n) noexcept {
    using namespace sin_port::native_weapon_tables;
    return v==1&&n==sizeof(ServerRecombinantSoundPrecacheApi)?&sounds_api:nullptr;
}
