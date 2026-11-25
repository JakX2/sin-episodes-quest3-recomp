#include "native_server_native_activity_services.h"
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerWeaponActivityApi* __cdecl
SiN_GetServerWeaponActivityApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerInheritedActivityApi* __cdecl
SiN_GetServerInheritedActivityApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
using S=ServerNativeActivitySession;
using G=ServerNativeGruntForward;
S& session(void* p) noexcept {return *static_cast<S*>(p);}
G& forward(void* p) noexcept {return *static_cast<G*>(p);}
bool actor_ok(const S& s,const void* a) noexcept {return s.actor==a;}
ServerGruntActivityBindings bind_grunt(G& g) noexcept {
    auto b=g.original;b.context=&g;
    b.has_weapon=[](void* p,void* a,const char* name,int tag,bool* out) noexcept -> int {
        auto& g=forward(p);return actor_ok(*g.session,a)&&g.original.has_weapon&&g.original.has_weapon(g.original.context,a,name,tag,out);};
    b.component_identity=[](void* p,void* a,std::uint32_t c,bool* out) noexcept -> int {
        auto& g=forward(p);return actor_ok(*g.session,a)&&g.original.component_identity&&g.original.component_identity(g.original.context,a,c,out);};
    b.component_translate=[](void* p,std::uint32_t c,int act,int* out) noexcept -> int {
        auto& g=forward(p);return g.original.component_translate&&g.original.component_translate(g.original.context,c,act,out);};
    b.parent_translate=[](void* p,void* a,int act,int* out) noexcept -> int {
        auto& s=*forward(p).session;
        if(!actor_ok(s,a)||!out)return 0;
        const auto* api=SiN_GetServerInheritedActivityApi(1,sizeof(ServerInheritedActivityApi));
        ServerInheritedActivityResult r{};++s.parent_calls;
        if(!api||!api->translate(a,s.bytes,act,&s.inherited,&r))return 0;
        *out=r.activity;return 1;};
    return b;
}
int __cdecl prepare(void* a,unsigned bytes,const ServerNpcActivityAssignmentBindings* b,const ServerInheritedActivityBindings* inherited,S* out) noexcept {
    if(!a||!b||!inherited||!out||
       (b->translation.actor_class!=NpcActivityGrunt&&b->translation.actor_class!=NpcActivityJetpack)||
       bytes<(b->translation.actor_class==NpcActivityJetpack?0x1028u:0xfccu)||
       !b->translation.metadata||!b->translation.activities||!b->translation.events||
       b->translation.events!=b->dictionary.dictionary||
       inherited->weapons.count>sin_independent::kEntityHandleCapacity||
       (inherited->weapons.count&&!inherited->weapons.entries))return 0;
    // Copy source before clearing output, so re-prepare from original is safe.
    const auto original=*b;const auto services=*inherited;
    *out={};out->actor=a;out->bytes=bytes;out->original=original;out->bound=original;out->inherited=services;
    out->inherited.sequences=&out->original.translation;
    out->grunt={out,original.translation.grunt};out->jetpack={out,original.translation.jetpack.grunt};
    auto& t=out->bound.translation;t.context=out;
    t.grunt=bind_grunt(out->grunt);t.jetpack.grunt=bind_grunt(out->jetpack);
    t.weapon=[](void* p,void* a,int input,bool* flag,int* result) noexcept -> int {
        auto& s=session(p);if(!actor_ok(s,a))return 0;
        const auto* api=SiN_GetServerWeaponActivityApi(1,sizeof(ServerWeaponActivityApi));
        ++s.weapon_calls;return api&&api->actor_translate(a,s.bytes,input,&s.inherited.weapons,flag,result);};
    t.metadata=[](void* p,void* a,const ServerActivityCompositeView** result) noexcept -> int {
        auto& s=session(p);auto& t=s.original.translation;
        return actor_ok(s,a)?t.metadata(t.context,a,result):-1;};
    t.scripted_sequence=[](void* p,void* a,int* result) noexcept -> int {
        auto& s=session(p);auto& t=s.original.translation;
        return actor_ok(s,a)&&t.scripted_sequence&&t.scripted_sequence(t.context,a,result);};
    t.missing_required=[](void* p,void* a,int act) noexcept {
        auto& s=session(p);auto& t=s.original.translation;if(actor_ok(s,a)&&t.missing_required)t.missing_required(t.context,a,act);};
    t.missing_sequence=[](void* p,void* a,int act) noexcept {
        auto& s=session(p);auto& t=s.original.translation;if(actor_ok(s,a)&&t.missing_sequence)t.missing_sequence(t.context,a,act);};
    auto& ab=out->bound;ab.context=out;
    ab.capabilities=[](void* p,void* a,unsigned* caps) noexcept -> int {
        auto& s=session(p);if(!actor_ok(s,a))return 0;
        const auto* api=SiN_GetServerWeaponActivityApi(1,sizeof(ServerWeaponActivityApi));
        ++s.capability_calls;return api&&api->capabilities(a,s.bytes,&s.inherited.weapons,caps);};
    ab.weapon_notify=[](void* p,void* a,std::uint32_t h,int act,float duration) noexcept -> int {
        auto& s=session(p);auto& b=s.original;return actor_ok(s,a)&&b.weapon_notify&&b.weapon_notify(b.context,a,h,act,duration);};
    ab.component_notify=[](void* p,void* a,std::uint32_t c) noexcept -> int {
        auto& s=session(p);auto& b=s.original;return actor_ok(s,a)&&b.component_notify&&b.component_notify(b.context,a,c);};
    // Grunt and Jetpack use native movement/view/activity-change branches,
    // so the other-class callbacks are not reachable in this session.
    return 1;
}
const ServerNativeActivityServicesApi api{1,sizeof(ServerNativeActivityServicesApi),&prepare};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerNativeActivityServicesApi=_SiN_GetServerNativeActivityServicesApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNativeActivityServicesApi* __cdecl
SiN_GetServerNativeActivityServicesApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerNativeActivityServicesApi)?&api:nullptr;
}
