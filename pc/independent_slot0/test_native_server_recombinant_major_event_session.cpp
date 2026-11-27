#include "native_server_recombinant_major_event_session.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <string>
using namespace sin_port::native_weapon_tables;
struct F{
    alignas(4) unsigned char actor[0xc70]{},target[0xc0]{};
    sin_independent::EntityHandleEntry registry[2]{};
    std::string calls;bool player=false,hit=true;unsigned parents=0;
    template<class T>void put(unsigned off,T v){std::memcpy(actor+off,&v,sizeof(v));}
};
int main(int argc,char** argv){
    assert(argc==3);
    auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    const auto get=reinterpret_cast<GetServerRecombinantMajorEventSessionApi>(
        GetProcAddress(server,server_recombinant_major_event_session_export));
    const auto registry_get=reinterpret_cast<GetServerRecombinantMajorEventsApi>(
        GetProcAddress(server,server_recombinant_major_events_export));
    assert(get&&registry_get&&!GetProcAddress(client,server_recombinant_major_event_session_export));
    assert(!get(2,sizeof(ServerRecombinantMajorEventSessionApi)));
    const auto* api=get(1,sizeof(ServerRecombinantMajorEventSessionApi));assert(api);
    ServerEventDictionaryEntry names[16]{};
    ServerEventDictionary dictionary{names,16,0,1,23};
    ServerRecombinantMajorEventRegistration registration{};
    assert(registry_get(1,sizeof(ServerRecombinantMajorEventsApi))->register_major(&dictionary,&registration));
    F f;f.put(0,recombinant_major_vtable);f.put(0x610,1u);f.put(0xc44,0xffffffffu);
    f.put(0x36c,10.f);f.put(0x370,20.f);f.put(0x374,30.f);
    f.registry[1].entity=reinterpret_cast<CBaseEntity*>(f.target);f.registry[1].serial=7;
    ServerRecombinantPolicyBindings policy{};policy.context=&f;policy.entries=f.registry;policy.entry_count=2;
    policy.sound=[](void* ctx,void*,const char* name,int x,int y) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);assert(!x&&!y&&!std::strcmp(name,"NPC_Leperkin.AttackHit"));f.calls+='S';return 1;
    };
    ServerRecombinantMajorEventBindings e{};e.policy=&policy;e.registration=&registration;e.context=&f;
    e.normal_damage=12.75f;e.heavy_damage=27.5f;
    e.basis=[](void* ctx,const float angles[3],float forward[3],float right[3]) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);assert(angles[0]==10);f.calls+='B';forward[0]=1;right[1]=1;return 1;
    };
    e.forward=[](void* ctx,const float*,float value[3]) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);f.calls+='F';value[0]=1;return 1;
    };
    e.enemy=[](void* ctx,void*,void** out) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);f.calls+='E';*out=f.target;return 1;
    };
    e.is_player=[](void* ctx,void*,bool* out) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);f.calls+='P';*out=f.player;return 1;
    };
    e.melee=[](void* ctx,void*,const ServerRecombinantMajorMeleeRequest*,void** hit) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);f.calls+='L';*hit=f.target;return 1;
    };
    e.parent=[](void* ctx,void*,const void*,unsigned) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);++f.parents;f.calls+='Q';return 1;
    };
    ServerRecombinantMeleeBindings melee{};melee.event=&e;melee.context=&f;
    melee.throw_object=[](void* ctx,void*,void* target) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);assert(target==f.target);f.calls+='J';return 1;
    };
    melee.hull_mins=[](void* ctx,unsigned hull,float v[3]) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);assert(hull==1);f.calls+='M';v[0]=-1;v[1]=-2;v[2]=-3;return 1;
    };
    melee.hull_maxs=[](void* ctx,unsigned hull,float v[3]) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);assert(hull==1);f.calls+='X';v[0]=1;v[1]=2;v[2]=3;return 1;
    };
    melee.hit_query=[](void* ctx,void* a,const ServerRecombinantMeleeTrace* tr,void** hit,unsigned* extent) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);assert(a==f.actor&&tr->reach==65&&tr->damage==12&&tr->scale==1&&tr->type==4&&tr->mode==0);
        f.calls+='H';*hit=f.hit?f.target:nullptr;*extent=sizeof(f.target);return 1;
    };
    melee.effect_token=[](void* ctx,void* entity,unsigned* token) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);assert(entity==f.target);f.calls+='T';*token=77;return 1;
    };
    melee.effect_eligible=[](void* ctx,unsigned token,bool* eligible) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);assert(token==77);f.calls+='K';*eligible=false;return 1;
    };
    ServerRecombinantMajorEventSessionBindings b{&e,&melee};
    ServerRecombinantMajorEventSessionResult o{};
    auto call=[&](int index){
        f.calls.clear();int event=index<0?999:registration.ids[index];
        return api->execute(f.actor,sizeof(f.actor),&event,sizeof(event),&b,&o);
    };
    assert(call(4)&&o.melee_invocations==1&&o.event.hit&&o.melee.hit==f.target&&o.melee.hull_calls==2&&
        o.melee.hit_calls==1&&f.calls=="BEPEMXHSPTK");
    f.put(0xc44,0x7001u);
    assert(call(4)&&o.melee.goal_live&&o.melee.throw_calls==1&&o.event.hit&&f.calls=="BEPEJ");
    f.put(0xc44,0xffffffffu);
    auto fallback=b;fallback.melee=nullptr;f.calls.clear();
    int id=registration.ids[4];
    assert(api->execute(f.actor,sizeof(f.actor),&id,4,&fallback,&o)&&o.delegated_melee==1&&
        !o.melee_invocations&&f.calls=="BEPL");
    assert(call(-1)&&o.event.delegated&&f.parents==1&&f.calls=="Q");
    auto mismatch=melee;mismatch.event=nullptr;
    fallback.melee=&mismatch;
    assert(!api->execute(f.actor,sizeof(f.actor),&id,4,&fallback,&o)&&!o.completed);
    ++dictionary.generation;assert(!call(4)&&!o.completed);
    FreeLibrary(server);FreeLibrary(client);return 0;
}
