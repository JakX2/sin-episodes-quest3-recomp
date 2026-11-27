#include "native_server_recombinant_policy.h"
#include "native_server_combat_owner.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <string>
using namespace sin_port::native_weapon_tables;
struct Fixture {
    alignas(4) unsigned char actor[0xc80]{},desc[0x48]{},weapon[4]{};
    sin_independent::EntityHandleEntry entries[2]{};
    int schedule=0,random=0,last_parent=0;
    unsigned matches=0,sounds=0,parents=0;
    const char* last_sound=nullptr;
    template<class T>void put(unsigned off,T v){std::memcpy(actor+off,&v,sizeof(v));}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    const auto get=reinterpret_cast<GetServerRecombinantPolicyApi>(GetProcAddress(server,server_recombinant_policy_export));
    assert(get&&!GetProcAddress(client,server_recombinant_policy_export)&&!get(2,sizeof(ServerRecombinantPolicyApi))&&!get(1,0));
    const auto* api=get(1,sizeof(ServerRecombinantPolicyApi));assert(api);
    const auto dc=reinterpret_cast<GetServerNpcConditionApi>(GetProcAddress(server,server_npc_condition_export));
    const auto da=reinterpret_cast<GetServerActivityDictionaryApi>(GetProcAddress(server,server_activity_dictionary_export));
    assert(dc&&da);const auto* cond=dc(1,sizeof(ServerNpcConditionApi));const auto* dict=da(1,sizeof(ServerActivityDictionaryApi));assert(cond&&dict);
    Fixture f;const int base=npc_global_condition_base;
    std::memcpy(f.desc+0x34,&base,4);const int high=127;std::memcpy(f.desc+0x3c,&high,4);
    ServerEventDictionaryEntry names[8]{};
    ServerActivityDictionary activities{{names,8,0,1,920},0};int idle=-1,walk=-1,run=-1,leap=-1;
    assert(dict->private_id(&activities,"ACT_IDLE_BLUNT_WEAPON",&idle)&&dict->private_id(&activities,"ACT_WALK_BLUNT_WEAPON",&walk)&&dict->private_id(&activities,"ACT_RUN_BLUNT_WEAPON",&run)&&dict->private_id(&activities,"ACT_LEAP_ATTACK",&leap));
    ServerRecombinantPolicyBindings b{};b.context=&f;b.activities=&activities;b.entries=f.entries;b.entry_count=2;
    b.conditions.context=&f;b.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept ->int{
        auto& f=*static_cast<Fixture*>(p);*out=f.desc;*n=sizeof(f.desc);return 1;
    };
    b.schedule_matches=[](void* p,void* a,int id,bool ideal,bool* out) noexcept ->int{
        auto& f=*static_cast<Fixture*>(p);assert(a==f.actor&&!ideal);++f.matches;*out=f.schedule==id;return 1;
    };
    b.parent_schedule=[](void* p,void* a,int id,int* out) noexcept ->int{
        auto& f=*static_cast<Fixture*>(p);assert(a==f.actor);++f.parents;f.last_parent=id;*out=id+100;return 1;
    };
    b.parent_activity=b.parent_schedule;
    b.random=[](void* p,int lo,int hi,int* out) noexcept ->int{
        auto& f=*static_cast<Fixture*>(p);assert(lo==0&&hi==10);*out=f.random;return 1;
    };
    b.sound=[](void* p,void* a,const char* name,int x,int y) noexcept ->int{
        auto& f=*static_cast<Fixture*>(p);assert(a==f.actor&&!x&&!y);++f.sounds;f.last_sound=name;return 1;
    };
    f.entries[1].entity=reinterpret_cast<CBaseEntity*>(f.weapon);f.entries[1].serial=7;
    ServerRecombinantPolicyResult o{};ServerNpcConditionResult cr{};
    f.put(0,recombinant_major_vtable);f.put(0x78c,0x7001u);
    assert(native_combat_owner_kind(recombinant_major_vtable)==NativeCombatOwnerKind::Npc);
    assert(native_combat_owner_kind(recombinant_minor_vtable)==NativeCombatOwnerKind::Npc);
    assert(api->schedule(f.actor,sizeof(f.actor),1,&b,&o)&&o.output==84&&!o.condition_tests);
    assert(api->schedule(f.actor,sizeof(f.actor),8,&b,&o)&&o.output==85);
    for(int id:{16,32,33,41}){
        assert(api->schedule(f.actor,sizeof(f.actor),id,&b,&o)&&o.output==80&&o.condition_tests==1&&!o.schedule_tests);
        assert(cond->store(f.actor,sizeof(f.actor),66,NpcConditionCurrent,true,&b.conditions,&cr));
        assert(api->schedule(f.actor,sizeof(f.actor),id,&b,&o)&&o.output==80&&o.condition_tests==2&&!o.schedule_tests);
        assert(cond->store(f.actor,sizeof(f.actor),33,NpcConditionCurrent,true,&b.conditions,&cr));
        f.schedule=79;assert(api->schedule(f.actor,sizeof(f.actor),id,&b,&o)&&o.output==80&&o.schedule_tests==1);
        f.schedule=80;assert(api->schedule(f.actor,sizeof(f.actor),id,&b,&o)&&o.output==73&&o.condition_tests==2&&o.schedule_tests==1);
        assert(cond->store(f.actor,sizeof(f.actor),66,NpcConditionCurrent,false,&b.conditions,&cr));
        assert(cond->store(f.actor,sizeof(f.actor),33,NpcConditionCurrent,false,&b.conditions,&cr));
    }
    assert(api->schedule(f.actor,sizeof(f.actor),2,&b,&o)&&o.delegated&&o.output==102);
    f.schedule=0;
    assert(api->activity(f.actor,sizeof(f.actor),1,&b,&o)&&o.output==idle&&o.custom_activity&&o.weapon_live);
    assert(api->activity(f.actor,sizeof(f.actor),6,&b,&o)&&o.output==walk&&o.schedule_tests==1);
    f.schedule=73;assert(api->activity(f.actor,sizeof(f.actor),6,&b,&o)&&o.output==run&&o.schedule_tests==1);
    assert(api->activity(f.actor,sizeof(f.actor),64,&b,&o)&&o.output==65&&!o.custom_activity);
    f.put(0xbf,static_cast<unsigned char>(8));f.schedule=0;
    assert(api->activity(f.actor,sizeof(f.actor),1,&b,&o)&&o.output==111&&!o.weapon_live);
    assert(api->activity(f.actor,sizeof(f.actor),6,&b,&o)&&o.output==112);
    assert(api->activity(f.actor,sizeof(f.actor),113,&b,&o)&&o.output==112);
    f.schedule=73;assert(api->activity(f.actor,sizeof(f.actor),6,&b,&o)&&o.output==run);
    f.put(0xbf,static_cast<unsigned char>(0));f.put(0x78c,0x8001u);
    assert(api->activity(f.actor,sizeof(f.actor),6,&b,&o)&&o.delegated&&o.output==110&&!o.weapon_live&&f.last_parent==10);
    for(int id:{1,6,10,42,43,99}){f.put(0x878,id);assert(api->yaw(f.actor,sizeof(f.actor),&b,&o)&&o.yaw==(id==1||id==6?25.f:id==10?35.f:20.f));}
    assert(api->pain(f.actor,sizeof(f.actor),&b,&o)&&o.sound_calls&&!std::strcmp(f.last_sound,"NPC_Leperkin.Pain"));
    f.put(0xbf,static_cast<unsigned char>(8));assert(api->pain(f.actor,sizeof(f.actor),&b,&o)&&o.suppressed&&!o.random_calls&&f.sounds==1);
    f.put(0,recombinant_minor_vtable);
    const int input[]{4,23,29,41,76},output[]{83,85,5,80,88};
    for(unsigned i=0;i<5;++i)assert(api->schedule(f.actor,sizeof(f.actor),input[i],&b,&o)&&o.output==output[i]&&!o.delegated);
    assert(api->schedule(f.actor,sizeof(f.actor),1,&b,&o)&&o.delegated&&o.output==101);
    assert(api->activity(f.actor,sizeof(f.actor),6,&b,&o)&&o.output==10&&!o.delegated);
    assert(api->activity(f.actor,sizeof(f.actor),10,&b,&o)&&o.output==110&&o.delegated);
    for(int id:{1,6,10,16,42,43,99,leap}){f.put(0x878,id);assert(api->yaw(f.actor,sizeof(f.actor),&b,&o)&&o.yaw==(id==leap?30.f:id==6||id==10?20.f:id==42||id==43?15.f:30.f));}
    f.random=10;assert(api->pain(f.actor,sizeof(f.actor),&b,&o)&&o.suppressed&&o.random_calls==1&&f.sounds==1);
    f.random=0;assert(api->pain(f.actor,sizeof(f.actor),&b,&o)&&o.sound_calls==1&&!std::strcmp(f.last_sound,"NPC_gib_hopper.Pain"));
    f.put(0xbf,static_cast<unsigned char>(0));assert(api->pain(f.actor,sizeof(f.actor),&b,&o)&&o.sound_calls==1&&!o.random_calls);
    auto missing=b;missing.activities=nullptr;assert(!api->yaw(f.actor,sizeof(f.actor),&missing,&o)&&!o.completed);
    assert(!api->yaw(f.actor,0x878,&b,&o)&&!o.completed);
    f.put(0,0x2249aaa4u);assert(!api->schedule(f.actor,sizeof(f.actor),1,&b,&o)&&!o.completed);

    // Authentic Major/Minor +0x438 failure-schedule policy. The inherited
    // scheduler and the Minor's collision probe remain host callbacks.
    const auto get_fail=reinterpret_cast<GetServerRecombinantFailScheduleApi>(
        GetProcAddress(server,server_recombinant_fail_schedule_export));
    assert(get_fail&&!GetProcAddress(client,server_recombinant_fail_schedule_export));
    assert(!get_fail(0,sizeof(ServerRecombinantFailScheduleApi))&&
        !get_fail(1,sizeof(ServerRecombinantFailScheduleApi)-1));
    const auto* fail_api=get_fail(1,sizeof(ServerRecombinantFailScheduleApi));
    assert(fail_api);
    struct FailHost {
        Fixture* fixture;
        std::string calls;
        unsigned probes=0,parents=0;
        bool clearance=false,fail_clearance=false,fail_parent=false;
        int last_failed=-1,last_reason=-1,last_arg3=-1;
    } h{&f};
    ServerRecombinantFailScheduleBindings fb{};
    fb.policy=&b;fb.context=&h;
    fb.ground_clear=[](void* context,void* actor,bool* out) noexcept ->int{
        auto& host=*static_cast<FailHost*>(context);
        assert(actor==host.fixture->actor&&out);
        ++host.probes;host.calls+='G';*out=host.clearance;
        return !host.fail_clearance;
    };
    fb.parent=[](void* context,void* actor,int failed,int reason,int arg3,int* out) noexcept ->int{
        auto& host=*static_cast<FailHost*>(context);
        assert(actor==host.fixture->actor&&out);
        ++host.parents;host.calls+='P';
        host.last_failed=failed;host.last_reason=reason;host.last_arg3=arg3;
        *out=failed+200;
        return !host.fail_parent;
    };
    ServerRecombinantFailScheduleResult fo{};
    f.put(0,recombinant_major_vtable);f.put(0xc68,0x1234u);
    assert(fail_api->evaluate(f.actor,sizeof(f.actor),7,8,9,&fb,&fo)&&
        fo.kind==RecombinantMajor&&fo.handle_cleared&&fo.delegated&&fo.completed&&
        fo.input==7&&fo.reason==8&&fo.arg3==9&&fo.output==207&&
        !fo.condition_tests&&!fo.ground_probes&&
        h.calls=="P"&&h.last_failed==7&&h.last_reason==8&&h.last_arg3==9);
    unsigned cleared=0;std::memcpy(&cleared,f.actor+0xc68,4);
    assert(cleared==0xffffffffu);
    f.put(0xc68,0x7777u);h.calls.clear();h.fail_parent=true;
    assert(!fail_api->evaluate(f.actor,sizeof(f.actor),5,6,7,&fb,&fo)&&
        fo.handle_cleared&&fo.delegated&&!fo.completed&&h.calls=="P");
    std::memcpy(&cleared,f.actor+0xc68,4);assert(cleared==0xffffffffu);
    h.fail_parent=false;h.calls.clear();f.put(0xc68,0x9999u);
    assert(!fail_api->evaluate(f.actor,0xc6b,7,8,9,&fb,&fo)&&
        !fo.handle_cleared&&!fo.delegated&&h.calls.empty());
    std::memcpy(&cleared,f.actor+0xc68,4);assert(cleared==0x9999u);

    f.put(0,recombinant_minor_vtable);
    // Condition 10 is the original local NPC condition, resolved through
    // the already reconstructed condition namespace, not a guessed bit.
    assert(cond->store(f.actor,sizeof(f.actor),10,NpcConditionCurrent,false,&b.conditions,&cr));
    h.calls.clear();h.probes=h.parents=0;h.clearance=false;
    assert(fail_api->evaluate(f.actor,sizeof(f.actor),85,0,9,&fb,&fo)&&
        fo.kind==RecombinantMinor&&fo.condition_tests==1&&!fo.condition_met&&
        fo.ground_probes==1&&!fo.ground_clear&&fo.output==86&&
        !fo.delegated&&h.calls=="G"&&h.probes==1&&!h.parents);
    assert(cond->store(f.actor,sizeof(f.actor),10,NpcConditionCurrent,true,&b.conditions,&cr));
    h.calls.clear();
    assert(fail_api->evaluate(f.actor,sizeof(f.actor),85,0,9,&fb,&fo)&&
        fo.condition_tests==1&&fo.condition_met&&fo.output==41&&
        !fo.ground_probes&&!fo.delegated&&h.calls.empty());
    h.calls.clear();
    assert(fail_api->evaluate(f.actor,sizeof(f.actor),23,100,11,&fb,&fo)&&
        fo.output==41&&fo.condition_tests==1&&!fo.ground_probes&&h.calls.empty());
    h.calls.clear();
    assert(fail_api->evaluate(f.actor,sizeof(f.actor),23,99,11,&fb,&fo)&&
        fo.output==86&&!fo.condition_tests&&fo.ground_probes==1&&h.calls=="G");
    for(int id:{72,73}){
        h.calls.clear();
        assert(fail_api->evaluate(f.actor,sizeof(f.actor),id,100,11,&fb,&fo)&&
            fo.output==86&&!fo.condition_tests&&fo.ground_probes==1&&h.calls=="G");
    }
    h.clearance=true;h.calls.clear();
    assert(fail_api->evaluate(f.actor,sizeof(f.actor),73,1,2,&fb,&fo)&&
        fo.output==273&&fo.ground_clear&&fo.ground_probes==1&&fo.delegated&&
        h.calls=="GP"&&h.last_failed==73&&h.last_reason==1&&h.last_arg3==2);
    h.calls.clear();
    assert(fail_api->evaluate(f.actor,sizeof(f.actor),77,11,12,&fb,&fo)&&
        fo.output==277&&!fo.condition_tests&&!fo.ground_probes&&
        fo.delegated&&h.calls=="P"&&h.last_arg3==12);
    assert(cond->store(f.actor,sizeof(f.actor),10,NpcConditionCurrent,false,&b.conditions,&cr));
    h.calls.clear();
    assert(fail_api->evaluate(f.actor,sizeof(f.actor),85,0,0,&fb,&fo)&&
        fo.output==285&&fo.condition_tests==1&&fo.ground_probes==1&&
        fo.delegated&&h.calls=="GP");
    h.clearance=false;h.fail_clearance=true;h.calls.clear();
    assert(!fail_api->evaluate(f.actor,sizeof(f.actor),23,99,1,&fb,&fo)&&
        !fo.completed&&fo.ground_probes==1&&!fo.delegated&&h.calls=="G");
    h.fail_clearance=false;h.calls.clear();
    auto invalid=fb;invalid.parent=nullptr;
    assert(!fail_api->evaluate(f.actor,sizeof(f.actor),77,0,0,&invalid,&fo)&&h.calls.empty());
    assert(!fail_api->evaluate(f.actor,0x7eb,85,0,0,&fb,&fo)&&h.calls.empty());
    f.put(0,0x2249aaa4u);
    assert(!fail_api->evaluate(f.actor,sizeof(f.actor),85,0,0,&fb,&fo)&&h.calls.empty());
    assert(!fail_api->evaluate(f.actor,sizeof(f.actor),85,0,0,&fb,nullptr));

    // Original Major virtual +0x5d8: the scalar is multiplied only for
    // mode 2, using the sign of (to.z - from.z). This is a class-local
    // policy, not a reconstruction of host movement/collision services.
    const auto get_scale=reinterpret_cast<GetServerRecombinantMajorScaleApi>(
        GetProcAddress(server,server_recombinant_major_scale_export));
    assert(get_scale&&!GetProcAddress(client,server_recombinant_major_scale_export)&&
        !get_scale(0,sizeof(ServerRecombinantMajorScaleApi))&&
        !get_scale(1,sizeof(ServerRecombinantMajorScaleApi)-1));
    const auto* scale=get_scale(1,sizeof(ServerRecombinantMajorScaleApi));
    assert(scale);
    f.put(0,recombinant_major_vtable);
    ServerRecombinantMajorScaleResult so{};
    const float from[]{0.f,0.f,4.f};
    float below[]{0.f,0.f,-2.f};
    float above[]{0.f,0.f,8.f};
    float equal[]{0.f,0.f,4.f};
    float v=2.f;
    assert(scale->scale(f.actor,sizeof(f.actor),2,from,below,&v,&so)&&so.completed&&
        so.modified&&so.mode==2&&so.input==2.f&&so.factor==.5f&&
        so.height_delta==-6.f&&v==1.f&&so.output==1.f);
    v=2.f;
    assert(scale->scale(f.actor,sizeof(f.actor),2,from,above,&v,&so)&&
        so.modified&&so.factor==1.5f&&so.height_delta==4.f&&v==3.f);
    v=2.f;
    assert(scale->scale(f.actor,sizeof(f.actor),2,from,equal,&v,&so)&&
        so.modified&&so.factor==1.5f&&!so.height_delta&&v==3.f);
    // Original return is true for factor != 1 even if the scalar is 0.
    v=0.f;
    assert(scale->scale(f.actor,sizeof(f.actor),2,from,above,&v,&so)&&
        so.modified&&so.factor==1.5f&&v==0.f);
    for(int id:{-1,0,1,3,200}){
        v=7.f;
        assert(scale->scale(f.actor,sizeof(f.actor),id,nullptr,nullptr,&v,&so)&&
            so.completed&&!so.modified&&so.factor==1.f&&so.input==7.f&&
            so.output==7.f&&v==7.f);
    }
    v=2.f;
    assert(!scale->scale(f.actor,sizeof(f.actor),2,nullptr,above,&v,&so)&&
        !so.completed&&!so.modified&&v==2.f);
    assert(!scale->scale(f.actor,sizeof(f.actor),2,from,nullptr,&v,&so)&&
        !so.completed&&!so.modified&&v==2.f);
    const float bad[]{0.f,0.f,std::numeric_limits<float>::quiet_NaN()};
    assert(!scale->scale(f.actor,sizeof(f.actor),2,from,bad,&v,&so)&&v==2.f);
    float huge=std::numeric_limits<float>::max();
    assert(!scale->scale(f.actor,sizeof(f.actor),2,from,above,&huge,&so)&&
        !so.completed&&huge==std::numeric_limits<float>::max());
    v=std::numeric_limits<float>::quiet_NaN();
    assert(!scale->scale(f.actor,sizeof(f.actor),2,from,below,&v,&so));
    v=2.f;
    assert(!scale->scale(f.actor,3,2,from,below,&v,&so)&&v==2.f);
    f.put(0,recombinant_minor_vtable);
    assert(!scale->scale(f.actor,sizeof(f.actor),2,from,below,&v,&so)&&
        !so.completed&&v==2.f);
    assert(!scale->scale(f.actor,sizeof(f.actor),2,from,below,&v,nullptr)&&v==2.f);
    FreeLibrary(server);FreeLibrary(client);return 0;
}
