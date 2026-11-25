#include "native_server_weapon_animation.h"
#include <cmath>
#include <cstring>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerActivityChoiceApi* __cdecl
SiN_GetServerActivityChoiceApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerSequenceDictionaryApi* __cdecl
SiN_GetServerSequenceDictionaryApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T> T read(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T> void write(void* p,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
bool dirty(void* weapon,const ServerSequenceResetBindings& b) noexcept {
    if(read<std::uint8_t>(weapon,0x1f0)){write<std::uint8_t>(weapon,0x200,1);return true;}
    const auto token=read<std::uint32_t>(weapon,0x16c);
    if(!token)return true;
    void* p=nullptr;unsigned extent=0;
    if(!b.storage||!b.storage(b.context,token,&p,&extent)||!p||extent<4)return false;
    write(p,0,read<unsigned>(p,0)|1u);return true;
}
template<class T> bool network(void* weapon,unsigned off,T v,const ServerSequenceResetBindings& b,bool* changed=nullptr) noexcept {
    const bool diff=read<T>(weapon,off)!=v;
    if(changed)*changed=diff;
    if(!diff)return true;
    if(!dirty(weapon,b))return false;write(weapon,off,v);return true;
}
bool choose(void* weapon,int activity,const ServerWeaponAnimationBindings& b,ServerWeaponAnimationResult& out) noexcept {
    const auto* api=SiN_GetServerActivityChoiceApi(1,sizeof(ServerActivityChoiceApi));
    if(!api)return false;
    const ServerActivityCompositeView* view=nullptr;
    const int status=b.metadata(b.context,weapon,&view);
    if(status<0||(status>0&&!view))return false;
    if(!status)view=nullptr;
    ServerActivityChoiceResult result{};
    ++out.selections;
    if(!api->choose(b.activities,b.events,view,activity,read<int>(weapon,0x3ec),true,&b.random,&result))return false;
    out.sequence=result.selected;return true;
}
int __cdecl assign(void* weapon,unsigned bytes,int requested,float npc_duration,const ServerWeaponAnimationBindings* b,ServerWeaponAnimationResult* out) noexcept {
    if(!out)return 0;*out={};out->sequence=-1;out->reset.registration.failed_index=-1;
    if(!weapon||bytes<0x4c0||!b||!b->metadata||!b->activities||!b->events||b->events!=b->dictionary.dictionary||
       !std::isfinite(npc_duration))return 0;
    if(!choose(weapon,requested,*b,*out))return 0;
    if(out->sequence==-1){
        out->fallback=1;
        if(!choose(weapon,157,*b,*out))return 0;
        if(out->sequence==-1){out->playback=read<float>(weapon,0x3c0);return 1;}
    }
    const auto* reset=SiN_GetServerSequenceDictionaryApi(1,sizeof(ServerSequenceDictionaryApi));
    if(!reset||!std::isfinite(read<float>(weapon,0x3e8))||!std::isfinite(read<float>(weapon,0x3c0)))return 0;
    bool changed=false;
    if(!network(weapon,0x3ec,out->sequence,b->reset,&changed))return 0;
    out->sequence_changed=changed;
    // 222ae7ce keeps the original requested activity even on idle fallback.
    write(weapon,0x4bc,requested);
    if(!network(weapon,0x3e8,0.f,b->reset,&changed))return 0;
    out->cycle_changed=changed;
    if(!reset->reset(weapon,bytes,&b->reset,&b->dictionary,&out->reset))return 0;
    float rate=1.f;
    if(npc_duration>0.f){
        double own=0;
        if(!b->reset.duration||!b->reset.duration(b->reset.context,weapon,out->sequence,&own)||!std::isfinite(own))return 0;
        const double ratio=own/static_cast<double>(npc_duration);
        rate=static_cast<float>(ratio);
        if(!std::isfinite(rate))return 0;
        // Original writes the rounded ratio, then clamps to 12.0. Preserve
        // the first dirty mark even when a later write restores the old value.
        if(!network(weapon,0x3c0,rate,b->reset))return 0;
        if(rate>=12.f)rate=12.f;
    }
    if(!network(weapon,0x3c0,rate,b->reset))return 0;
    out->playback=rate;out->assigned=1;return 1;
}
int __cdecl notify(void* owner,unsigned bytes,int activity,float duration,const ServerWeaponAnimationDispatchBindings* b,ServerWeaponAnimationDispatchResult* out) noexcept {
    if(!out)return 0;*out={};out->animation.sequence=-1;out->animation.reset.registration.failed_index=-1;
    if(!owner||bytes<0x790||!b||b->weapons.count>sin_independent::kEntityHandleCapacity||
       (b->weapons.count&&!b->weapons.entries))return 0;
    void* weapon=sin_independent::EntityHandleView(b->weapons.entries,b->weapons.count).Resolve(read<std::uint32_t>(owner,0x78c));
    if(!weapon)return 1;
    unsigned kind=0,extent=0;
    if(!b->weapons.classify||!b->weapons.classify(b->weapons.context,weapon,&kind)||kind>WeaponActivitySmg||
       !b->extent||!b->extent(b->context,weapon,&extent))return 0;
    out->live=1;
    return assign(weapon,extent,activity,duration,&b->animation,&out->animation);
}
const ServerWeaponAnimationApi api{1,sizeof(ServerWeaponAnimationApi),&assign,&notify};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerWeaponAnimationApi=_SiN_GetServerWeaponAnimationApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerWeaponAnimationApi* __cdecl
SiN_GetServerWeaponAnimationApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerWeaponAnimationApi)?&api:nullptr;
}
