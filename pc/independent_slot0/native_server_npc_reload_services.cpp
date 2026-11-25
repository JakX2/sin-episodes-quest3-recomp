#include "native_server_npc_reload_services.h"
#include <cstring>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerWeaponScriptViewApi* __cdecl
SiN_GetServerWeaponScriptViewApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcConditionApi* __cdecl
SiN_GetServerNpcConditionApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerWeaponEventSoundApi* __cdecl
SiN_GetServerWeaponEventSoundApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
using S=ServerNpcReloadSession;
S& self(void* p) noexcept{return *static_cast<S*>(p);}
template<class T>T read(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T>void write(void* p,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
const ServerWeaponScriptViewApi* script_api() noexcept {return SiN_GetServerWeaponScriptViewApi(1,sizeof(ServerWeaponScriptViewApi));}
bool kind(S& s,void* w,unsigned* out) noexcept {
    const auto& b=s.host.original.weapon.weapons;
    return b.classify&&b.classify(b.context,w,out)&&*out<=WeaponActivitySmg;
}
int __cdecl clear(void* p,void* actor,int cond) noexcept {
    auto& s=self(p);if(actor!=s.actor)return 0;
    const auto* api=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));
    ServerNpcConditionResult result{};++s.condition_calls;
    return api&&api->store(actor,s.bytes,cond,NpcConditionCurrent,false,&s.host.conditions,&result);
}
int __cdecl prepare(void* actor,unsigned bytes,const ServerNpcReloadServicesBindings* b,S* out) noexcept {
    if(!actor||bytes<0x7ec||!b||!out)return 0;
    const auto& w=b->original.weapon.weapons;
    if(w.count>sin_independent::kEntityHandleCapacity||(w.count&&!w.entries))return 0;
    const auto source=*b;*out={};out->actor=actor;out->bytes=bytes;out->host=source;out->bound=source.original;
    out->bound.context=out;
    out->bound.script_info=[](void* p,std::uint16_t index,const void** data,unsigned* extent) noexcept -> int {
        auto& s=self(p);const auto* api=script_api();ServerWeaponScriptRecord rec{};++s.script_lookups;
        if(!api||!api->resolve(&s.host.scripts,index,&rec))return 0;
        *data=rec.data;*extent=rec.bytes;return 1;};
    out->bound.clear_condition=&clear;
    auto& sound=out->bound.weapon.sound;sound.context=out;
    sound.emit=[](void* p,void* weapon,int id,float time) noexcept -> int {
        const auto& b=self(p).host.original.weapon.sound;return b.emit&&b.emit(b.context,weapon,id,time);};
    sound.interval=[](void* p,void* weapon,double* result) noexcept -> int {
        auto& s=self(p);unsigned k=0;const auto* api=script_api();++s.sound_intervals;
        return api&&kind(s,weapon,&k)&&api->interval(k,result);};
    sound.max_clip=[](void* p,void* weapon,int* result) noexcept -> int {
        auto& s=self(p);unsigned k=0,n=0;const auto& w=s.host.original.weapon;const auto* api=script_api();++s.burst_queries;
        return api&&kind(s,weapon,&k)&&w.extent&&w.extent(w.context,weapon,&n)&&api->burst(weapon,n,k,result);};
    return 1;
}
bool store_clip(void* weapon,unsigned off,int value,const ServerWeaponEventRouteBindings& b,unsigned& changed) noexcept {
    if(read<int>(weapon,off)==value)return true;
    if(read<std::uint8_t>(weapon,0x1f0))write<std::uint8_t>(weapon,0x200,1);
    else{
        const auto token=read<std::uint32_t>(weapon,0x16c);
        if(token){void* data=nullptr;unsigned extent=0;
            if(!b.storage||!b.storage(b.context,token,&data,&extent)||!data||extent<4)return false;
            write(data,0,read<unsigned>(data,0)|1u);}
    }
    write(weapon,off,value);changed=1;return true;
}
int __cdecl fixed_reset(S* s,ServerNpcReloadResult* out) noexcept {
    if(!out)return 0;*out={};out->primary=out->secondary=-1;out->sound.last_sound=-1;
    if(!s||!s->actor||s->bytes<0x7ec||s->bound.context!=s)return 0;
    const auto& w=s->bound.weapon;const auto& registry=w.weapons;
    void* weapon=sin_independent::EntityHandleView(registry.entries,registry.count).Resolve(read<std::uint32_t>(s->actor,0x78c));
    if(weapon){
        unsigned k=0,n=0;const auto* sound=SiN_GetServerWeaponEventSoundApi(1,sizeof(ServerWeaponEventSoundApi));const auto* api=script_api();
        if(!sound||!api||!kind(*s,weapon,&k)||!w.extent||!w.extent(w.context,weapon,&n)||n<0x51e)return 0;
        out->live=1;
        if(!sound->play(weapon,n,k,9,0,&w.sound,&out->sound))return 0;
        ++s->script_lookups;
        if(!api->value(&s->host.scripts,weapon,n,0x404,&out->primary)||!store_clip(weapon,0x4ec,out->primary,w,out->primary_changed))return 0;
        ++s->script_lookups;
        if(!api->value(&s->host.scripts,weapon,n,0x408,&out->secondary)||!store_clip(weapon,0x4f0,out->secondary,w,out->secondary_changed))return 0;
    }
    for(int cond=3;cond<=5;++cond){if(!clear(s,s->actor,cond))return 0;++out->conditions_cleared;}
    return 1;
}
const ServerNpcReloadServicesApi api{1,sizeof(ServerNpcReloadServicesApi),&prepare,&fixed_reset};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerNpcReloadServicesApi=_SiN_GetServerNpcReloadServicesApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcReloadServicesApi* __cdecl
SiN_GetServerNpcReloadServicesApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerNpcReloadServicesApi)?&api:nullptr;
}
