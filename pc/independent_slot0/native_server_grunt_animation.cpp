#include "native_server_grunt_animation.h"
#include <cmath>
#include <cstring>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerActivityDictionaryApi* __cdecl
SiN_GetServerActivityDictionaryApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerEventDictionaryApi* __cdecl
SiN_GetServerEventDictionaryApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerWeaponEventRouteApi* __cdecl
SiN_GetServerWeaponEventRouteApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcParentWeaponEventApi* __cdecl
SiN_GetServerNpcParentWeaponEventApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T> T read(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T> void write(void* p,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
bool extend(void* actor,double until) noexcept {
    if(until>read<float>(actor,0xc90)){write(actor,0xc90,static_cast<float>(until));return true;}
    return false;
}
int handle_impl(void* grunt,unsigned bytes,const ServerAnimationEvent* e,float now,const ServerGruntAnimationBindings* b,ServerGruntAnimationResult* out,const ServerNpcParentWeaponEventBindings* native=nullptr,ServerGruntNativeWeaponEventResult* nr=nullptr) noexcept {
    if(!out)return -1;
    *out={};
    if(!grunt||bytes<0xfcc||!e||!b||!std::isfinite(now))return -1;
    const int id=e->event;
    // Fixed event 0x14 precedes both dynamic IDs.
    if(id==0x14){
        out->route=GruntAnimBlocking;if(!b->debug)return -1;
        b->debug(b->context,"BLOCKING!\n");write<std::uint8_t>(grunt,0xc35,1);return 1;
    }
    if(id==b->begin_altfire){out->route=GruntAnimBeginAlt;return 1;}
    if(id==b->altfire){
        out->route=GruntAnimAlt;
        void* weapon=nullptr;
        if(native){
            const auto& w=native->weapon.weapons;
            if(w.count>sin_independent::kEntityHandleCapacity||(w.count&&!w.entries)||
               !sin_independent::EntityHandleView(w.entries,w.count).Resolve(read<std::uint32_t>(grunt,0x78c)))return -1;
        }else{
            if(!b->weapon||!b->weapon_event||
               !b->weapon(b->context,grunt,read<std::uint32_t>(grunt,0x78c),&weapon)||!weapon)return -1;
        }
        const double until=static_cast<double>(now)+10.;
        if(!std::isfinite(static_cast<float>(until))||!std::isfinite(read<float>(grunt,0xc90)))return -1;
        const auto squad=read<std::uint32_t>(grunt,0x9c4);
        void** peers=nullptr;unsigned count=0;
        if(squad&&(!b->peers||!b->peers(b->context,grunt,squad,&peers,&count)||count>4096||(count&&!peers)))return -1;
        for(unsigned i=0;i<count;++i)if(!peers[i]||!std::isfinite(read<float>(peers[i],0xc90)))return -1;
        // 2235e90a: weapon +4c0 first, then 22358200 deadline,
        // then raw +c38 decrement. Never updates +fc8 on this branch.
        if(native){
            const auto* api=SiN_GetServerWeaponEventRouteApi(1,sizeof(ServerWeaponEventRouteApi));
            // Retail only initializes event ID and source; port initializes
            // all other fields instead of forwarding indeterminate stack data.
            const ServerAnimationEvent generated{0xc1f,nullptr,0,now,0,grunt};
            if(!api||!nr)return -1;
            nr->native_altfire=1;
            if(!api->active(grunt,bytes,&generated,now,&native->weapon,&nr->altfire))return -1;
        }else b->weapon_event(b->context,weapon,0xc1f,grunt);
        extend(grunt,until);
        for(unsigned i=0;i<count;++i)if(extend(peers[i],until))++out->peers_updated;
        write(grunt,0xc38,read<std::uint32_t>(grunt,0xc38)-1u);
        return 1;
    }
    switch(id){
    case 2:{
        out->route=GruntAnimWeaponReset;
        if(!b->weapon||!b->clear_condition)return -1;
        void* weapon=nullptr;
        if(!b->weapon(b->context,grunt,read<std::uint32_t>(grunt,0x78c),&weapon))return -1;
        if(weapon){
            if(!b->weapon_mode||!b->weapon_value||!b->weapon_store)return -1;
            b->weapon_mode(b->context,weapon,9,0);
            const int a=b->weapon_value(b->context,weapon,0x404);
            b->weapon_store(b->context,weapon,0x4ec,a);
            const int c=b->weapon_value(b->context,weapon,0x408);
            b->weapon_store(b->context,weapon,0x4f0,c);
        }
        for(int cond=3;cond<=5;++cond)b->clear_condition(b->context,grunt,cond);
        break;
    }
    case 4:case 8:out->route=GruntAnimMark;break;
    case 10:
        out->route=GruntAnimActivity;
        if(!b->activity||!b->activity_request)return -1;
        {const int a=b->activity(b->context,grunt);b->activity_request(b->context,grunt,a,0,2);}
        break;
    // Dedicated object-interaction bodies, not the shared parent route.
    case 3:case 7:case 9:out->route=GruntAnimUnrecovered;return 0;
    default:
        out->route=GruntAnimParent;
        if(native){
            const auto* api=SiN_GetServerNpcParentWeaponEventApi(1,sizeof(ServerNpcParentWeaponEventApi));
            if(!api||!nr)return -1;
            const int status=api->handle(grunt,bytes,e,now,native,&nr->parent);
            if(status!=0){nr->native_parent=nr->parent.recognized;return status;}
        }
        if(!b->parent)return -1;
        b->parent(b->context,grunt,e);return 1;
    }
    write(grunt,0xfc8,id);return 1;
}
int __cdecl handle(void* grunt,unsigned bytes,const ServerAnimationEvent* e,float now,const ServerGruntAnimationBindings* b,ServerGruntAnimationResult* out) noexcept {
    return handle_impl(grunt,bytes,e,now,b,out);
}
constexpr const char* grunt_activities[]{
    "ACT_COMBINE_THROW_GRENADE","ACT_COMBINE_LAUNCH_GRENADE","ACT_COMBINE_BUGBAIT",
    "ACT_COMBINE_AR2_ALTFIRE","ACT_MELEE_KICK","ACT_GRUNT_SHOT_IN_HELMET"
};
constexpr const char* grunt_events[]{"COMBINE_AE_BEGIN_ALTFIRE","COMBINE_AE_ALTFIRE"};
int __cdecl register_jetpack(ServerEventDictionary* e,const ServerGruntRegistrationBindings* b,ServerGruntRegistration* out) noexcept {
    if(!out)return 0;
    out->events_ready=false;
    if(!e)return 0;
    const auto* api=SiN_GetServerEventDictionaryApi(1,sizeof(ServerEventDictionaryApi));
    if(!api)return 0;
    out->event_owner=e;out->event_generation=e->generation;
    for(unsigned i=0;i<2;++i){
        if(!api->private_id(e,grunt_events[i],out->events+i))return 0;
        if(b&&b->publish)b->publish(b->context,true,grunt_events[i],out->events[i]);
    }
    out->events_ready=true;return 1;
}
int __cdecl register_grunt(ServerActivityDictionary* a,ServerEventDictionary* e,const ServerGruntRegistrationBindings* b,ServerGruntRegistration* out) noexcept {
    if(!out)return 0;*out={};
    if(!a||!e||&a->names==e)return 0;
    const auto* api=SiN_GetServerActivityDictionaryApi(1,sizeof(ServerActivityDictionaryApi));
    if(!api)return 0;
    out->activity_owner=a;out->activity_generation=a->names.generation;
    for(unsigned i=0;i<6;++i){
        if(!api->private_id(a,grunt_activities[i],out->activities+i))return 0;
        if(b&&b->publish)b->publish(b->context,false,grunt_activities[i],out->activities[i]);
    }
    out->activities_ready=true;
    return register_jetpack(e,b,out);
}
bool activity_ids_valid(const ServerGruntRegistration* ids) noexcept {
    if(!ids||!ids->activities_ready||!ids->activity_owner||ids->activity_generation!=ids->activity_owner->names.generation)return false;
    const auto* api=SiN_GetServerActivityDictionaryApi(1,sizeof(ServerActivityDictionaryApi));
    if(!api)return false;
    for(unsigned i=0;i<6;++i){int id=-1;if(!api->lookup(ids->activity_owner,grunt_activities[i],&id)||id<0||id!=ids->activities[i])return false;}
    return true;
}
bool event_ids_valid(const ServerGruntRegistration* ids) noexcept {
    if(!ids||!ids->events_ready||!ids->event_owner||ids->event_generation!=ids->event_owner->generation)return false;
    const auto* api=SiN_GetServerEventDictionaryApi(1,sizeof(ServerEventDictionaryApi));
    if(!api)return false;
    for(unsigned i=0;i<2;++i){int id=-1;if(!api->lookup(ids->event_owner,grunt_events[i],&id)||id<0||id!=ids->events[i])return false;}
    return true;
}
int __cdecl translate(void* actor,unsigned bytes,int activity,const ServerGruntRegistration* ids,const ServerGruntActivityBindings* b,int* out) noexcept {
    if(!out)return 0;*out=activity;
    if(!actor||bytes<0xdc0||!b)return 0;
    if(activity==0x11){
        // Native direct return before either component service.
        bool launcher=false;
        if(!activity_ids_valid(ids)||!b->has_weapon||
           !b->has_weapon(b->context,actor,"weapon_grenadelauncher",0,&launcher))return 0;
        *out=ids->activities[launcher?1:0];return 1;
    }
    const bool knees=read<std::uint8_t>(actor,0xca0)!=0;
    const bool upright=read<std::uint8_t>(actor,0xc64)!=0;
    if(activity==0x10){if(knees)activity=0xc7;else if(!upright)activity=0x12;}
    else if(activity==0x42){if(knees)activity=0xc8;else if(!upright)activity=0x43;}
    else if(activity==0xa||activity==6)write<std::uint8_t>(actor,0xc64,1);
    else if(activity==1){
        if(knees)activity=0xc4;
        else if(!upright)activity=0x2d;
        else {const int state=read<int>(actor,0x850);if(state==2||state==3)activity=0x48;}
    }
    const auto component=read<std::uint32_t>(actor,0xdbc);
    bool identity=false;
    if(!b->component_identity||!b->component_identity(b->context,actor,component,&identity))return 0;
    if(identity){if(activity==1)activity=0x48;else if(activity==6)activity=7;else if(activity==0xa)activity=0xb;}
    const auto delegate=read<std::uint32_t>(actor,0xc18);
    if(delegate)return b->component_translate&&b->component_translate(b->context,delegate,activity,out);
    return b->parent_translate&&b->parent_translate(b->context,actor,activity,out);
}
int __cdecl registered_handle(void* actor,unsigned bytes,const ServerAnimationEvent* e,float now,const ServerGruntRegistration* ids,const ServerGruntAnimationBindings* b,ServerGruntAnimationResult* out) noexcept {
    if(!out)return -1;*out={};
    if(!b||!event_ids_valid(ids))return -1;
    auto bindings=*b;bindings.begin_altfire=ids->events[0];bindings.altfire=ids->events[1];
    return handle(actor,bytes,e,now,&bindings,out);
}
int __cdecl handle_native(void* actor,unsigned bytes,const ServerAnimationEvent* e,float now,const ServerGruntRegistration* ids,const ServerGruntAnimationBindings* b,const ServerNpcParentWeaponEventBindings* native,ServerGruntNativeWeaponEventResult* out) noexcept {
    if(!out)return -1;*out={};
    if(!b||!native||!event_ids_valid(ids))return -1;
    auto binding=*b;binding.begin_altfire=ids->events[0];binding.altfire=ids->events[1];
    return handle_impl(actor,bytes,e,now,&binding,&out->grunt,native,out);
}
const ServerGruntNativeWeaponEventApi native_event_api{1,sizeof(ServerGruntNativeWeaponEventApi),&handle_native};
int __cdecl jetpack_translate(void* actor,unsigned bytes,int incoming,const ServerGruntRegistration* ids,const ServerJetpackActivityBindings* b,ServerJetpackActivityResult* out) noexcept {
    if(!out)return 0;*out={};out->activity=incoming;
    if(!actor||bytes<0x1028||!b||!b->attachment)return 0;
    const auto handle=read<std::uint32_t>(actor,0x1024);
    void* entity=nullptr;unsigned extent=0;
    if(handle==0xffffffffu||!b->attachment(b->context,handle,&entity,&extent)||!entity||extent<0xc24)return 0;
    const double x=read<float>(entity,0xc18),y=read<float>(entity,0xc1c),z=read<float>(entity,0xc20);
    if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z))return 0;
    // Original 22364755 FSTP f32, 222680e0 FSQRT,
    // 22364768 FSTP f32. Do not compare an unrounded double magnitude.
    const float squared=static_cast<float>(x*x+y*y+z*z);
    if(!std::isfinite(squared))return 0;
    out->magnitude=static_cast<float>(std::sqrt(static_cast<double>(squared)));
    if(incoming==1||incoming==7||incoming==16){
        out->activity=out->magnitude>50.f?213:208;return 1;
    }
    if(incoming==66){out->activity=218;return 1;}
    out->delegated=1;
    return translate(actor,bytes,incoming,ids,&b->grunt,&out->activity);
}
const ServerJetpackActivityApi jetpack_api{1,sizeof(ServerJetpackActivityApi),&jetpack_translate};
const ServerGruntRegistrationApi registration_api{1,sizeof(ServerGruntRegistrationApi),&register_grunt,&register_jetpack,&translate,&registered_handle};
const ServerGruntAnimationApi api{1,sizeof(ServerGruntAnimationApi),&handle};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerGruntAnimationApi=_SiN_GetServerGruntAnimationApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerGruntAnimationApi* __cdecl
SiN_GetServerGruntAnimationApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerGruntAnimationApi)?&api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerGruntRegistrationApi=_SiN_GetServerGruntRegistrationApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerGruntRegistrationApi* __cdecl
SiN_GetServerGruntRegistrationApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerGruntRegistrationApi)?&registration_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerJetpackActivityApi=_SiN_GetServerJetpackActivityApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerJetpackActivityApi* __cdecl
SiN_GetServerJetpackActivityApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerJetpackActivityApi)?&jetpack_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerGruntNativeWeaponEventApi=_SiN_GetServerGruntNativeWeaponEventApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerGruntNativeWeaponEventApi* __cdecl
SiN_GetServerGruntNativeWeaponEventApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerGruntNativeWeaponEventApi)?&native_event_api:nullptr;
}
