#include "native_server_npc_weapon_runtime.h"
#include <cmath>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNativeActivityServicesApi* __cdecl
SiN_GetServerNativeActivityServicesApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcActivityAssignmentApi* __cdecl
SiN_GetServerNpcActivityAssignmentApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcTransitionApi* __cdecl
SiN_GetServerNpcTransitionApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcAnimationTickApi* __cdecl
SiN_GetServerNpcAnimationTickApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerWeaponAnimationApi* __cdecl
SiN_GetServerWeaponAnimationApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerWeaponPostFrameApi* __cdecl
SiN_GetServerWeaponPostFrameApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
struct Call {
    const ServerNpcWeaponRuntimeBindings* host;
    ServerNpcWeaponRuntimeResult* result;
    void* actor;
    unsigned bytes;
    float now;
    ServerNativeActivitySession services;
    ServerNpcAnimationTickBindings npc;
    ServerWeaponPostFrameBindings weapon;
};
Call& self(void* p) noexcept {return *static_cast<Call*>(p);}
bool prepare(Call& c) noexcept {
    const auto& host=*c.host;
    const auto& original=host.npc.transition.assignment;
    const auto* services=SiN_GetServerNativeActivityServicesApi(1,sizeof(ServerNativeActivityServicesApi));
    if(!services||!original.translation.activities||!original.translation.events||
       original.translation.activities!=host.npc.frame.activities||
       original.translation.events!=host.npc.frame.frame.dictionary.dictionary||
       original.translation.activities!=host.weapon.animation.activities||
       original.translation.events!=host.weapon.animation.events||
       original.translation.events!=host.weapon.animation.dictionary.dictionary||
       !services->prepare(c.actor,c.bytes,&original,&host.inherited,&c.services))return false;
    c.npc=host.npc;c.weapon=host.weapon;
    // All active weapon and owner resolution uses the same serial registry.
    c.weapon.weapons=host.inherited.weapons;
    auto& assignment=c.npc.transition.assignment;
    assignment=c.services.bound;assignment.context=&c;
    assignment.capabilities=[](void* p,void* actor,unsigned* caps) noexcept -> int {
        auto& c=self(p);const auto& b=c.services.bound;
        return actor==c.actor&&b.capabilities(b.context,actor,caps);};
    assignment.component_notify=[](void* p,void* actor,std::uint32_t component) noexcept -> int {
        auto& c=self(p);const auto& b=c.services.bound;
        return actor==c.actor&&b.component_notify(b.context,actor,component);};
    assignment.weapon_notify=[](void* p,void* actor,std::uint32_t,int activity,float duration) noexcept -> int {
        auto& c=self(p);if(actor!=c.actor)return 0;
        const auto* api=SiN_GetServerWeaponAnimationApi(1,sizeof(ServerWeaponAnimationApi));
        ServerWeaponAnimationDispatchBindings b{c.weapon.weapons,c.weapon.animation,c.weapon.context,c.weapon.extent};
        ++c.result->weapon_notifications;
        return api&&api->notify(actor,c.bytes,activity,duration,&b,&c.result->weapon_assignment);};
    c.npc.context=&c;
    c.npc.storage=[](void* p,std::uint32_t word,const void** out,unsigned* extent) noexcept -> int {
        auto& h=self(p).host->npc;return h.storage&&h.storage(h.context,word,out,extent);};
    c.npc.promote_path=[](void* p,void* actor,std::uint32_t component,bool* promoted) noexcept -> int {
        auto& h=self(p).host->npc;return h.promote_path&&h.promote_path(h.context,actor,component,promoted);};
    c.npc.weapon=[](void* p,std::uint32_t handle,void** out) noexcept -> int {
        auto& c=self(p);const auto& w=c.weapon.weapons;
        *out=sin_independent::EntityHandleView(w.entries,w.count).Resolve(handle);
        return *out?1:0;};
    c.npc.weapon_post=[](void* p,void* weapon,void* actor) noexcept -> int {
        auto& c=self(p);if(actor!=c.actor||!weapon)return 0;
        const auto* api=SiN_GetServerWeaponPostFrameApi(1,sizeof(ServerWeaponPostFrameApi));
        unsigned live=0;++c.result->weapon_frames;
        return api&&api->notify(actor,c.bytes,c.now,&c.weapon,&c.result->weapon_frame,&live)&&live;};
    return true;
}
int __cdecl request(void* actor,unsigned bytes,int activity,const ServerNpcWeaponRuntimeBindings* b,ServerNpcWeaponRuntimeResult* out) noexcept {
    if(!out)return 0;*out={};
    if(!b)return 0;
    Call c{};c.host=b;c.result=out;c.actor=actor;c.bytes=bytes;
    const auto* api=SiN_GetServerNpcActivityAssignmentApi(1,sizeof(ServerNpcActivityAssignmentApi));
    return api&&prepare(c)&&api->request(actor,bytes,activity,&c.npc.transition.assignment,&out->request);
}
int __cdecl update(void* actor,unsigned bytes,const ServerNpcWeaponRuntimeBindings* b,ServerNpcWeaponRuntimeResult* out) noexcept {
    if(!out)return 0;*out={};
    if(!b)return 0;
    Call c{};c.host=b;c.result=out;c.actor=actor;c.bytes=bytes;
    const auto* api=SiN_GetServerNpcTransitionApi(1,sizeof(ServerNpcTransitionApi));
    return api&&prepare(c)&&api->update(actor,bytes,&c.npc.transition,&out->transition);
}
int __cdecl run(void* actor,unsigned bytes,const ServerNpcRunAnimationInput* in,const ServerNpcWeaponRuntimeBindings* b,ServerNpcWeaponRuntimeResult* out) noexcept {
    if(!out)return 0;*out={};
    if(!b||!in||!std::isfinite(in->now))return 0;
    Call c{};c.host=b;c.result=out;c.actor=actor;c.bytes=bytes;c.now=in->now;
    const auto* api=SiN_GetServerNpcAnimationTickApi(1,sizeof(ServerNpcAnimationTickApi));
    return api&&prepare(c)&&api->run(actor,bytes,in,&c.npc,&out->tick);
}
const ServerNpcWeaponRuntimeApi api{1,sizeof(ServerNpcWeaponRuntimeApi),&request,&update,&run};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerNpcWeaponRuntimeApi=_SiN_GetServerNpcWeaponRuntimeApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcWeaponRuntimeApi* __cdecl
SiN_GetServerNpcWeaponRuntimeApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerNpcWeaponRuntimeApi)?&api:nullptr;
}
