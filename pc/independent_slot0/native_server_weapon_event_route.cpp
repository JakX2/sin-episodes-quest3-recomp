#include "native_server_weapon_event_route.h"
#include <cmath>
#include <cstring>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerWeaponEventSoundApi* __cdecl
SiN_GetServerWeaponEventSoundApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T> T read(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T> void write(void* p,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
bool registry_ok(const ServerWeaponActivityBindings& b) noexcept {
    return b.count<=sin_independent::kEntityHandleCapacity&&(!b.count||b.entries);
}
void* resolve(const ServerWeaponEventRouteBindings& b,std::uint32_t handle) noexcept {
    return sin_independent::EntityHandleView(b.weapons.entries,b.weapons.count).Resolve(handle);
}
bool dirty(void* w,const ServerWeaponEventRouteBindings& b) noexcept {
    if(read<std::uint8_t>(w,0x1f0)){write<std::uint8_t>(w,0x200,1);return true;}
    const auto token=read<std::uint32_t>(w,0x16c);
    if(!token)return true;
    void* data=nullptr;unsigned extent=0;
    if(!b.storage||!b.storage(b.context,token,&data,&extent)||!data||extent<4)return false;
    write(data,0,read<unsigned>(data,0)|1u);return true;
}
bool decrement(void* w,const ServerWeaponEventRouteBindings& b,ServerWeaponEventRouteResult& out) noexcept {
    if(!dirty(w,b))return false;
    write(w,0x4ec,read<std::uint32_t>(w,0x4ec)-1u);out.clip_changed=1;return true;
}
bool owner(void* w,const ServerWeaponEventRouteBindings& b,ServerWeaponEventRouteResult& out,void** combat,void** player) noexcept {
    *combat=*player=nullptr;out.owner_live=out.player_owner=0;
    void* entity=resolve(b,read<std::uint32_t>(w,0x494));
    if(!entity)return true;
    if(!b.combat_owner)return false;
    const int status=b.combat_owner(b.context,entity,combat);
    if(status<0||(status>0&&!*combat))return false;
    if(!status){*combat=nullptr;return true;}
    bool is_player=false;
    if(!b.is_player||!b.is_player(b.context,*combat,&is_player))return false;
    out.owner_live=1;out.player_owner=is_player;
    if(is_player)*player=*combat;
    return true;
}
bool sound(void* w,unsigned bytes,unsigned kind,int mode,const ServerWeaponEventRouteBindings& b,ServerWeaponEventRouteResult& out) noexcept {
    const auto* api=SiN_GetServerWeaponEventSoundApi(1,sizeof(ServerWeaponEventSoundApi));
    return api&&api->play(w,bytes,kind,mode,0,&b.sound,&out.sound);
}
bool prepare(void* w,void* receiver,unsigned kind,const ServerAnimationEvent* e,const ServerWeaponEventRouteBindings& b,ServerWeaponEventRouteResult& out) noexcept {
    if(!receiver||!b.prepare||!b.prepare(b.context,w,receiver,kind,e))return false;
    out.prepared=1;return true;
}
bool shot(void* w,void* receiver,unsigned kind,const ServerAnimationEvent* e,const ServerWeaponEventRouteBindings& b,ServerWeaponEventRouteResult& out) noexcept {
    if(!b.primary_effect||!b.primary_effect(b.context,w,receiver,kind,e))return false;
    out.primary_called=1;return true;
}
bool flash(void* receiver,const ServerWeaponEventRouteBindings& b,ServerWeaponEventRouteResult& out) noexcept {
    if(!receiver||!b.flash||!b.flash(b.context,receiver))return false;
    out.flash_called=1;return true;
}
bool deadline(void* w,unsigned off,double next,const ServerWeaponEventRouteBindings& b,ServerWeaponEventRouteResult& out) noexcept {
    const float old=read<float>(w,off),rounded=static_cast<float>(next);
    if(!std::isfinite(old)||!std::isfinite(rounded))return false;
    if(static_cast<double>(old)==next)return true;
    if(!dirty(w,b))return false;write(w,off,rounded);++out.deadlines_written;return true;
}
int __cdecl handle(void* w,unsigned bytes,unsigned kind,void* receiver,const ServerAnimationEvent* e,float now,const ServerWeaponEventRouteBindings* b,ServerWeaponEventRouteResult* out) noexcept {
    if(!out)return 0;*out={};out->sound.last_sound=-1;
    if(!w||bytes<0x4f0||kind>WeaponActivitySmg||!e||!b||!registry_ok(b->weapons)||!std::isfinite(now))return 0;
    const int id=e->event;
    void* combat=nullptr;void* player=nullptr;
    if(kind==WeaponActivityGrenade){
        if(bytes<0x585||!owner(w,*b,*out,&combat,&player))return 0;
        if(id==3900){out->route=WeaponEventGrenadeReady;write<std::uint8_t>(w,0x584,1);return 1;}
        if(id==3005||id==3013||id==3016){
            out->route=WeaponEventGrenade;
            if(!player||!b->grenade||!b->consume_grenade||!b->grenade(b->context,w,player,id)||
               !b->consume_grenade(b->context,player,read<int>(w,0x4e4)))return 0;
            out->grenade_consumed=1;
            if(!deadline(w,0x49c,static_cast<double>(now)+.5,*b,*out)||
               !deadline(w,0x4a0,static_cast<double>(now)+.5,*b,*out))return 0;
            if(read<unsigned>(w,0x4a4)!=0x7f7fffffu){
                if(!dirty(w,*b))return 0;write(w,0x4a4,0x7f7fffffu);++out->deadlines_written;
            }
            // Original re-resolves combat owner after the grenade helpers.
            combat=resolve(*b,read<std::uint32_t>(w,0x494));
            if(combat){
                if(!b->combat_owner)return 0;
                void* actual=nullptr;const int status=b->combat_owner(b->context,combat,&actual);
                if(status<0||(status>0&&!actual))return 0;
                if(status>0&&(!b->grenade_danger||!b->grenade_danger(b->context,w,actual)))return 0;
            }
            return 1;
        }
    }
    if(id==3103&&(kind==WeaponActivityAssault||kind==WeaponActivityMagnum||kind==WeaponActivityNpcPistol||kind==WeaponActivityScatter)){
        out->route=WeaponEventSecondary;
        if(!b->secondary||!b->secondary(b->context,w,kind))return 0;
        out->secondary_called=1;return 1;
    }
    if(kind==WeaponActivityChaingun&&id==3002){
        if(bytes<0x598)return 0;
        const int state=read<int>(w,0x594);
        if(state==0){
            out->route=WeaponEventSpinStart;
            if(!sound(w,bytes,kind,13,*b,*out))return 0;
            write(w,0x594,1);write(w,0x590,now);write(w,0x58c,now);return 1;
        }
        if(state!=2){out->route=WeaponEventSpinIdle;return 1;}
        out->route=WeaponEventPrimary;
        if(!decrement(w,*b,*out)||!prepare(w,receiver,kind,e,*b,*out)||!shot(w,receiver,kind,e,*b,*out)||
           !flash(receiver,*b,*out)||!sound(w,bytes,kind,2,*b,*out))return 0;
        write(w,0x58c,now);return 1;
    }
    const bool primary=(kind==WeaponActivityAssault||kind==WeaponActivitySmg)?id==3002:
        (kind==WeaponActivityMagnum||kind==WeaponActivityNpcPistol)?id==3014:
        kind==WeaponActivityScatter&&id==3004;
    if(primary){
        out->route=WeaponEventPrimary;
        if(!prepare(w,receiver,kind,e,*b,*out))return 0;
        if(kind==WeaponActivityAssault){
            const auto* api=SiN_GetServerWeaponEventSoundApi(1,sizeof(ServerWeaponEventSoundApi));
            if(!api||!api->assault_tick(w,bytes,now,&b->sound,&out->sound))return 0;
        }else if(!sound(w,bytes,kind,2,*b,*out))return 0;
        // Scatter differs: muzzle callback and clip decrement BEFORE effect.
        if(kind==WeaponActivityScatter)return flash(receiver,*b,*out)&&decrement(w,*b,*out)&&shot(w,receiver,kind,e,*b,*out);
        return shot(w,receiver,kind,e,*b,*out)&&flash(receiver,*b,*out)&&decrement(w,*b,*out);
    }
    if(kind==WeaponActivityMelee&&id==3001){
        out->route=WeaponEventMelee;return receiver&&b->melee&&b->melee(b->context,w,receiver,e);
    }
    // 222cc450 shared parent.
    if(id==3001||id==3003){out->route=WeaponEventSound;return sound(w,bytes,kind,id==3001?11:10,*b,*out);}
    if(id==3200){
        out->route=WeaponEventBaseContact;
        if(!owner(w,*b,*out,&combat,&player))return 0;
        return !player||(b->base_contact&&b->base_contact(b->context,w,player,e));
    }
    out->route=WeaponEventDiagnostic;
    if(b->diagnostic)b->diagnostic(b->context,w,e);
    return 1;
}
int __cdecl active(void* a,unsigned bytes,const ServerAnimationEvent* e,float now,const ServerWeaponEventRouteBindings* b,ServerWeaponEventRouteResult* out) noexcept {
    if(!out)return 0;*out={};out->sound.last_sound=-1;
    if(!a||bytes<0x790||!b||!e||!registry_ok(b->weapons))return 0;
    void* w=resolve(*b,read<std::uint32_t>(a,0x78c));if(!w)return 0;
    unsigned kind=0,extent=0;
    if(!b->weapons.classify||!b->weapons.classify(b->weapons.context,w,&kind)||
       !b->extent||!b->extent(b->context,w,&extent))return 0;
    return handle(w,extent,kind,a,e,now,b,out);
}
int __cdecl parent_weapon(void* a,unsigned bytes,const ServerAnimationEvent* e,float now,const ServerNpcParentWeaponEventBindings* b,ServerNpcParentWeaponEventResult* out) noexcept {
    if(!out)return -1;*out={};out->clip=-1;
    if(!a||bytes<0x790||!e||!b||!std::isfinite(now)||!registry_ok(b->weapon.weapons))return -1;
    const int id=e->event;
    if(id<3000||id>3999)return 0;
    out->recognized=1;
    const auto& wbind=b->weapon;
    void* w=resolve(wbind,read<std::uint32_t>(a,0x78c));
    // Original 222a5720 and special reload routes are no-op for absent weapon.
    if(!w)return 1;
    out->live=1;
    unsigned kind=0,extent=0;
    if(!wbind.weapons.classify||!wbind.weapons.classify(wbind.weapons.context,w,&kind)||kind>WeaponActivitySmg||
       !wbind.extent||!wbind.extent(wbind.context,w,&extent)||extent<0x4f0)return -1;
    if(id!=3015&&id!=3017&&id!=3018){
        out->forwarded=1;return handle(w,extent,kind,a,e,now,&wbind,&out->weapon)?1:-1;
    }
    if(id!=3018){
        if(!sound(w,extent,kind,9,wbind,out->weapon))return -1;
        out->sounded=out->weapon.sound.emitted;
    }
    if(id==3017)return 1;
    // 222abf50 gets +160 from script info resolved by +51c.
    if(extent<0x51e||!b->script_info||!b->clear_condition)return -1;
    const void* info=nullptr;unsigned size=0;
    if(!b->script_info(b->context,read<std::uint16_t>(w,0x51c),&info,&size)||!info||size<0x164)return -1;
    out->clip=read<int>(info,0x160);
    if(read<int>(w,0x4ec)!=out->clip){
        if(!dirty(w,wbind))return -1;write(w,0x4ec,out->clip);out->clip_changed=1;
    }
    for(int cond=3;cond<=5;++cond){
        if(!b->clear_condition(b->context,a,cond))return -1;
        ++out->conditions_cleared;
    }
    return 1;
}
const ServerNpcParentWeaponEventApi parent_api{1,sizeof(ServerNpcParentWeaponEventApi),&parent_weapon};
const ServerWeaponEventRouteApi api{1,sizeof(ServerWeaponEventRouteApi),&handle,&active};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerWeaponEventRouteApi=_SiN_GetServerWeaponEventRouteApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerWeaponEventRouteApi* __cdecl
SiN_GetServerWeaponEventRouteApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerWeaponEventRouteApi)?&api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcParentWeaponEventApi=_SiN_GetServerNpcParentWeaponEventApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcParentWeaponEventApi* __cdecl
SiN_GetServerNpcParentWeaponEventApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerNpcParentWeaponEventApi)?&parent_api:nullptr;
}
