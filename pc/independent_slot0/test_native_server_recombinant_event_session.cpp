#include "native_server_recombinant_event_session.h"
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
struct F {
    alignas(4) unsigned char actor[0xc50]{},entity[4]{},desc[0x48]{};
    std::string calls;
    unsigned launches=0,projectiles=0,spits=0,parents=0,slash=0;
    bool live=true;
    const ServerNpcConditionApi* conditions=nullptr;
    ServerNpcConditionMapBindings map{};
    template<class T> void put(unsigned off,T v){std::memcpy(actor+off,&v,sizeof(v));}
    template<class T> T get(unsigned off){T v{};std::memcpy(&v,actor+off,sizeof(v));return v;}
    bool has(int id) {
        ServerNpcConditionResult r{};
        assert(conditions->test(actor,sizeof(actor),id,NpcConditionCurrent,&map,&r)&&r.mapped);
        return r.value!=0;
    }
};
int main(int argc,char** argv){
    assert(argc==3);
    auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    const auto get=reinterpret_cast<GetServerRecombinantEventSessionApi>(GetProcAddress(server,server_recombinant_event_session_export));
    const auto reg_get=reinterpret_cast<GetServerRecombinantEventsApi>(GetProcAddress(server,server_recombinant_events_export));
    const auto cond_get=reinterpret_cast<GetServerNpcConditionApi>(GetProcAddress(server,server_npc_condition_export));
    assert(get&&reg_get&&cond_get&&!GetProcAddress(client,server_recombinant_event_session_export));
    assert(!get(2,sizeof(ServerRecombinantEventSessionApi)));
    const auto* api=get(1,sizeof(ServerRecombinantEventSessionApi));assert(api);
    ServerEventDictionaryEntry entries[16]{};
    ServerEventDictionary dictionary{entries,16,0,1,23};
    ServerRecombinantEventRegistration reg{};
    assert(reg_get(1,sizeof(ServerRecombinantEventsApi))->register_minor(&dictionary,&reg));
    F f;f.conditions=cond_get(1,sizeof(ServerNpcConditionApi));
    f.put(0,recombinant_minor_vtable);f.put(0xc44,.7f);f.put(0x36c,2.f);
    f.put(0x360,4.f);f.put(0x364,5.f);f.put(0x368,6.f);
    f.put(0x2bc,7.f);f.put(0x2c0,8.f);f.put(0x2c4,9.f);
    f.put(0xc48,91u);f.put(0xc4c,92u);
    std::memcpy(f.desc+0x34,&npc_global_condition_base,4);
    int zero=0,lim=127;std::memcpy(f.desc+0x38,&zero,4);std::memcpy(f.desc+0x3c,&lim,4);
    f.map.context=&f;f.map.descriptor=[](void* ctx,void*,const void** out,unsigned* n) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);*out=f.desc;*n=sizeof(f.desc);return 1;
    };
    ServerRecombinantPolicyBindings policy{};policy.conditions=f.map;
    ServerRecombinantEventBindings event{};event.policy=&policy;event.registration=&reg;event.context=&f;
    event.enemy=[](void* ctx,void* a,void** out) noexcept ->int {
        auto& f=*static_cast<F*>(ctx);assert(a==f.actor);f.calls+='E';*out=f.live?f.entity:nullptr;return 1;
    };
    event.special=[](void* ctx,void* e,bool* out) noexcept ->int {auto& f=*static_cast<F*>(ctx);assert(e==f.entity);f.calls+='S';*out=true;return 1;};
    event.target_point=[](void* ctx,void* e,float v[3]) noexcept ->int {
        auto& f=*static_cast<F*>(ctx);assert(e==f.entity);f.calls+='P';v[0]=100;v[1]=0;v[2]=30;return 1;
    };
    event.center=[](void* ctx,void* e,float v[3]) noexcept ->int {auto& f=*static_cast<F*>(ctx);assert(e==f.entity);f.calls+='C';v[0]=100;v[1]=0;v[2]=20;return 1;};
    event.player_probe=[](void* ctx,void*) noexcept ->int {static_cast<F*>(ctx)->calls+='Y';return 1;};
    event.random=[](void* ctx,float lo,float hi,float* value) noexcept ->int {
        auto& f=*static_cast<F*>(ctx);f.calls+='R';
        if(lo==5.f&&hi==10.f){*value=6.f;return 1;}
        *value=lo==1.5f?1.75f:0.f;
        assert((lo==1.5f&&hi==2.f)||(lo==-.05f&&(hi==.05f||hi==0.f)));
        return 1;
    };
    event.pounce_slash=[](void* ctx,void*,void*) noexcept ->int {static_cast<F*>(ctx)->calls+='H';return 1;};
    event.angular_effect=[](void* ctx,void*,const float angles[3]) noexcept ->int {
        auto& f=*static_cast<F*>(ctx);assert(angles[0]==6.f&&angles[1]==6.f&&angles[2]==0.f);f.calls+='X';return 1;
    };
    event.pounce_off=[](void* ctx,void*) noexcept ->int {static_cast<F*>(ctx)->calls+='O';return 1;};
    event.parent=[](void* ctx,void*,const void*,unsigned) noexcept ->int {
        auto& f=*static_cast<F*>(ctx);++f.parents;f.calls+='Q';return 1;
    };
    ServerRecombinantDamageBindings damage{&policy,&f};
    ServerRecombinantMinorBindings minor{&damage,&f,1};
    minor.set_velocity=[](void* ctx,void* a,const float v[3]) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);assert(a==f.actor&&f.has(58));++f.launches;f.calls+='V';return 1;
    };
    minor.schedule_touch_think=[](void* ctx,void*,float,int mode) noexcept ->int {
        auto& f=*static_cast<F*>(ctx);assert(!mode);f.calls+='T';return 1;
    };
    ServerRecombinantMinorTouchBindings touch{&minor,&f};
    touch.set_movement=[](void* ctx,void*,int mode,int arg) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);assert(mode==3&&!arg);f.calls+='M';return 1;
    };
    touch.install_think=[](void* ctx,void*,unsigned marker,int,int) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);assert(marker==recombinant_airborne_think_marker);f.calls+='I';return 1;
    };
    ServerRecombinantAirborneBindings airborne{&touch,&f,600.f};
    airborne.ballistic=[](void* ctx,const float a[3],const float b[3],float gravity,float mode,float v[3]) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);assert(a[0]==7&&b[0]==100&&gravity==600.f&&mode==3);f.calls+='B';v[0]=40;v[1]=20;v[2]=50;return 1;
    };
    airborne.forward=[](void* ctx,const float*,float v[3]) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);f.calls+='F';v[0]=1;v[1]=0;v[2]=0;return 1;
    };
    airborne.clear_ground=[](void* ctx,void*) noexcept ->int {static_cast<F*>(ctx)->calls+='G';return 1;};
    airborne.relocate=[](void* ctx,void*,const float v[3],int mode) noexcept ->int {
        auto& f=*static_cast<F*>(ctx);assert(!mode&&v[2]==7);f.calls+='L';return 1;
    };
    ServerRecombinantSpitBindings spit{&event,&f,600.f,17.f,false};
    spit.attachment=[](void* ctx,void*,const char* name,float v[3]) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);assert(!std::strcmp(name,"Acid_spit"));f.calls+='A';v[0]=1;v[1]=2;v[2]=3;return 1;
    };
    spit.aim_point=[](void* ctx,void*,const float*,int mode,float v[3]) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);assert(mode==1);f.calls+='D';v[0]=5;v[1]=2;v[2]=3;return 1;
    };
    spit.ballistic=[](void* ctx,const float*,const float*,float gravity,float mode,float v[3]) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);assert(gravity==600.f&&mode==1);f.calls+='B';v[0]=1;v[1]=0;v[2]=1;return 1;
    };
    spit.normalize=[](void* ctx,float v[3]) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);f.calls+='N';v[0]=1;v[1]=0;v[2]=0;return 1;
    };
    spit.projectile=[](void* ctx,void*,const ServerRecombinantSpitRequest* req) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);assert(req->velocity[0]==900&&req->damage==17.f);++f.projectiles;f.calls+='J';return 1;
    };
    spit.effects=[](void* ctx,void*,const ServerRecombinantSpitEffect* effect) noexcept ->int {
        auto& f=*static_cast<F*>(ctx);assert(effect->first_token==91&&effect->second_token==92);f.calls+='X';return 1;
    };
    ServerRecombinantPounceBindings pounce{&airborne,&event,&f,9.f};
    pounce.damage_info=[](void* ctx,void* a,float damage,unsigned kind,void* info,unsigned bytes) noexcept ->int {
        auto& f=*static_cast<F*>(ctx);assert(a==f.actor&&damage==9.f&&kind==4&&bytes==0x48);f.calls+='D';std::memset(info,0,bytes);return 1;
    };
    pounce.force=[](void* ctx,void*,unsigned size,void* a,const float[3],const float[3],float scale) noexcept ->int {
        auto& f=*static_cast<F*>(ctx);assert(a==f.actor&&size==0x48&&scale==1.f);f.calls+='F';return 1;
    };
    pounce.apply_damage=[](void* ctx,void* target,const void*,unsigned size) noexcept ->int {
        auto& f=*static_cast<F*>(ctx);assert(target==f.entity&&size==0x48);f.calls+='K';return 1;
    };
    ServerRecombinantEventSessionBindings b{&event,&airborne,&spit,&pounce};
    ServerRecombinantEventSessionResult o{};
    auto call=[&](int id,float now) {
        f.calls.clear();int event_id=id<0?999:reg.ids[id];
        return api->execute(f.actor,sizeof(f.actor),&event_id,sizeof(event_id),now,&b,&o);
    };
    assert(call(1,10.f)&&o.event.cache_written&&f.calls=="EEESPY");
    assert(call(0,10.5f)&&o.leap_invocations==1&&o.event.cache_consumed&&o.airborne.ballistic_calls==1&&
        o.airborne.think_scheduled&&f.has(58)&&f.get<unsigned>(0x7c)==recombinant_minor_contact_marker&&
        f.calls=="EBGLVIT"&&f.launches==1);
    // Detached event without enemy must use original straight jump mode.
    f.put<unsigned char>(0xc41,0);f.live=false;
    assert(call(0,11.f)&&o.airborne.forward_calls==1&&o.leap_invocations==1&&f.calls=="EFGLVIT"&&f.launches==2);
    f.live=true;f.put<unsigned char>(0xc41,0);
    assert(call(2,20.f)&&o.spit_invocations==1&&o.spit.projectiles==1&&o.spit.deadline_written&&
        f.get<float>(0xc3c)==21.75f&&f.calls=="EAEDEPBNRRRJXR");
    f.live=false;f.put<unsigned char>(0xc41,0);
    assert(call(4,21.f)&&o.pounce_off_invocations==1&&o.pounce.fallback.forward_calls==1&&
        o.pounce.movement_set&&f.calls=="MEFGLVIT");
    f.live=true;
    sin_independent::EntityHandleEntry handles[2]{};
    handles[1].entity=reinterpret_cast<CBaseEntity*>(f.entity);handles[1].serial=7;
    policy.entries=handles;policy.entry_count=2;f.put(0xc30,0x7001u);
    assert(call(3,21.5f)&&o.pounce_slash_invocations==1&&o.pounce.damage_calls==1&&
        o.event.slash_calls==1&&o.event.angular_calls==1&&f.calls=="DFKRRX");
    auto owner_fallback=b;owner_fallback.pounce=nullptr;
    f.calls.clear();int off_id=reg.ids[4];
    assert(api->execute(f.actor,sizeof(f.actor),&off_id,4,22.f,&owner_fallback,&o)&&
        o.delegated_effects==1&&f.calls=="O");
    assert(call(-1,21.f)&&o.event.delegated&&f.parents==1&&f.calls=="Q");
    auto mismatched=spit;mismatched.event=nullptr;
    auto bad=b;bad.spit=&mismatched;
    assert(!api->execute(f.actor,sizeof(f.actor),&reg.ids[2],4,21.f,&bad,&o)&&!o.completed);
    bad=b;auto broken=pounce;broken.event=nullptr;bad.pounce=&broken;
    assert(!api->execute(f.actor,sizeof(f.actor),&reg.ids[4],4,22.f,&bad,&o));
    FreeLibrary(server);FreeLibrary(client);return 0;
}
