#include "native_server_event_dictionary.h"
#include <cstring>
#include <limits>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerStudioEventRegistrationApi* __cdecl
SiN_GetServerStudioEventRegistrationApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
bool valid(const ServerEventDictionary* d) noexcept {
    return d&&d->capacity<=65535&&d->count<=d->capacity&&(!d->capacity||d->entries)&&d->highest>=0;
}
bool name_ok(const char* s) noexcept {
    if(!s)return false;
    for(unsigned i=0;i<128;++i){const unsigned char c=s[i];if(!c)return true;if(c>=128)return false;}
    return false;
}
unsigned char fold(unsigned char c) noexcept {return c>='A'&&c<='Z'?static_cast<unsigned char>(c+32):c;}
bool equal(const char* a,const char* b) noexcept {
    for(unsigned i=0;i<128;++i){const auto x=fold(a[i]),y=fold(b[i]);if(x!=y)return false;if(!x)return true;}
    return false;
}
const ServerEventDictionaryEntry* named(const ServerEventDictionary& d,const char* name) noexcept {
    for(unsigned i=0;i<d.count;++i)if(equal(d.entries[i].name,name))return d.entries+i;
    return nullptr;
}
const ServerEventDictionaryEntry* by_id(const ServerEventDictionary& d,int id) noexcept {
    for(unsigned i=0;i<d.count;++i)if(d.entries[i].id==id)return d.entries+i;
    return nullptr;
}
bool append(ServerEventDictionary& d,const char* name,int id,unsigned flags,bool private_event) noexcept {
    if(d.count==d.capacity)return false;
    ServerEventDictionaryEntry entry{};entry.id=id;entry.flags=flags;entry.private_event=private_event;
    std::memcpy(entry.name,name,std::strlen(name)+1);
    d.entries[d.count++]=entry;
    if(id>d.highest)d.highest=id;
    return true;
}
int __cdecl lookup(const ServerEventDictionary* d,const char* name,int* id) noexcept {
    if(!id)return 0;*id=-1;
    if(!valid(d)||!name_ok(name))return 0;
    if(const auto* e=named(*d,name))*id=e->id;
    return 1;
}
int __cdecl flags(const ServerEventDictionary* d,int id,unsigned* out) noexcept {
    if(!out)return 0;*out=0xffffffffu;
    if(!valid(d))return 0;
    if(const auto* e=by_id(*d,id))*out=e->flags;
    return 1;
}
int __cdecl shared(ServerEventDictionary* d,const char* name,int id,unsigned flags) noexcept {
    if(!valid(d)||!name_ok(name)||id<0||named(*d,name)||by_id(*d,id))return 0;
    return append(*d,name,id,flags,false);
}
int __cdecl private_id(ServerEventDictionary* d,const char* name,int* out) noexcept {
    if(!out)return 0;*out=-1;
    if(!valid(d)||!name_ok(name))return 0;
    if(const auto* e=named(*d,name)){
        if(!e->private_event)return 0;
        *out=e->id;return 1;
    }
    if(d->highest==std::numeric_limits<int>::max())return 0;
    const int id=d->highest+1;
    if(by_id(*d,id)||!append(*d,name,id,1,true))return 0;
    *out=id;return 1;
}
int __cdecl clear(ServerEventDictionary* d) noexcept {
    if(!valid(d))return 0;
    d->count=0;++d->generation;return 1;
}
int __cdecl reset_highest(ServerEventDictionary* d) noexcept {
    // Original 221d78f0 is unconditional; restrict to an empty port registry
    // to prevent generating a duplicate private ID.
    if(!valid(d)||d->count)return 0;d->highest=0;return 1;
}
struct Seed {const char* name;unsigned flags;};
constexpr Seed seeds[]{
    {"AE_EMPTY",1},{"AE_NPC_LEFTFOOT",1},{"AE_NPC_RIGHTFOOT",1},
    {"AE_NPC_BODYDROP_LIGHT",1},{"AE_NPC_BODYDROP_HEAVY",1},{"AE_NPC_SWISHSOUND",1},
    {"AE_NPC_180TURN",1},{"AE_NPC_ITEM_PICKUP",1},{"AE_NPC_WEAPON_DROP",1},
    {"AE_NPC_WEAPON_SET_SEQUENCE_NAME",1},{"AE_NPC_WEAPON_SET_SEQUENCE_NUMBER",1},
    {"AE_NPC_WEAPON_SET_ACTIVITY",1},{"AE_NPC_HOLSTER",1},{"AE_NPC_DRAW",1},
    {"AE_CL_PLAYSOUND",16},{"AE_CL_STOPSOUND",16},{"AE_START_SCRIPTED_EFFECT",1},
    {"AE_STOP_SCRIPTED_EFFECT",1},{"AE_CLIENT_EFFECT_ATTACH",16},{"AE_MUZZLEFLASH",16},
    {"AE_NPC_MUZZLEFLASH",16},{"AE_THUMPER_THUMP",1},{"AE_AMMOCRATE_PICKUP_AMMO",1},
    {"AE_NPC_RAGDOLL",1}
};
int __cdecl seed(ServerEventDictionary* d) noexcept {
    if(!valid(d)||d->count||d->capacity<24)return 0;
    for(int i=0;i<24;++i)if(!append(*d,seeds[i].name,i,seeds[i].flags,false))return 0;
    return 1;
}
int __cdecl apply(ServerEventDictionary* d,void* data,unsigned bytes,unsigned desc,ServerStudioEventRegistrationResult* out) noexcept {
    if(!valid(d))return 0;
    const auto* registration=SiN_GetServerStudioEventRegistrationApi(1,sizeof(ServerStudioEventRegistrationApi));
    if(!registration)return 0;
    ServerStudioEventRegistrationBindings b{d,
        [](void* p,const char* n,int* id) noexcept{return lookup(static_cast<ServerEventDictionary*>(p),n,id);},
        [](void* p,const char* n,int* id) noexcept{return private_id(static_cast<ServerEventDictionary*>(p),n,id);},
        [](void* p,int id,unsigned* f) noexcept{return flags(static_cast<ServerEventDictionary*>(p),id,f);}
    };
    return registration->apply(data,bytes,desc,&b,out);
}
const ServerEventDictionaryApi api{1,sizeof(ServerEventDictionaryApi),&lookup,&flags,&shared,&private_id,&clear,&reset_highest,&seed,&apply};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerEventDictionaryApi=_SiN_GetServerEventDictionaryApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerEventDictionaryApi* __cdecl
SiN_GetServerEventDictionaryApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerEventDictionaryApi)?&api:nullptr;
}
