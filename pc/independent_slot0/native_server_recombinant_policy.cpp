#include "native_server_recombinant_policy.h"
#include <cstring>
extern "C" const sin_port::native_weapon_tables::ServerNpcConditionApi* __cdecl SiN_GetServerNpcConditionApi(unsigned,unsigned) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerActivityDictionaryApi* __cdecl SiN_GetServerActivityDictionaryApi(unsigned,unsigned) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
bool init(void* a,unsigned n,unsigned extent,const ServerRecombinantPolicyBindings* b,ServerRecombinantPolicyResult* o) noexcept {
    if(!o)return false;*o={};o->output=-1;o->random_value=-1;
    if(!a||n<extent||!b)return false;
    switch(rd<unsigned>(a,0)){
    case recombinant_major_vtable:o->kind=RecombinantMajor;break;
    case recombinant_minor_vtable:o->kind=RecombinantMinor;break;
    default:return false;
    }return true;
}
bool lookup(const ServerRecombinantPolicyBindings& b,const char* name,int& id) noexcept {
    const auto* api=SiN_GetServerActivityDictionaryApi(1,sizeof(ServerActivityDictionaryApi));
    return api&&b.activities&&api->lookup(b.activities,name,&id)&&id>=0;
}
int __cdecl schedule(void* a,unsigned n,int id,const ServerRecombinantPolicyBindings* b,ServerRecombinantPolicyResult* o) noexcept {
    if(!init(a,n,0x7ec,b,o))return 0;o->input=id;
    auto match=[&](int v,bool& yes){++o->schedule_tests;return b->schedule_matches&&b->schedule_matches(b->context,a,v,false,&yes);};
    auto has=[&](int v,bool& yes){
        ++o->condition_tests;ServerNpcConditionResult r{};
        const auto* api=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));
        if(!api||!api->test(a,n,v,NpcConditionCurrent,&b->conditions,&r))return false;
        yes=r.value!=0;return true;
    };
    auto done=[&](int v){o->output=v;o->completed=1;return 1;};
    if(o->kind==RecombinantMajor){
        switch(id){
        case 1:return done(84);
        case 8:return done(85);
        case 16:case 32:case 33:case 41:{
            bool yes=false;
            if(!has(66,yes))return 0;
            if(yes){if(!has(33,yes))return 0;if(yes){if(!match(80,yes))return 0;if(yes)return done(73);}}
            return done(80);
        }}
    }else{
        switch(id){case 4:return done(83);case 23:return done(85);case 29:return done(5);case 41:return done(80);case 76:return done(88);}
    }
    if(!b->parent_schedule||!b->parent_schedule(b->context,a,id,&o->output))return 0;
    o->delegated=o->completed=1;return 1;
}
int __cdecl activity(void* a,unsigned n,int id,const ServerRecombinantPolicyBindings* b,ServerRecombinantPolicyResult* o) noexcept {
    if(!init(a,n,0x790,b,o))return 0;o->input=id;
    auto done=[&](int v){o->output=v;o->completed=1;return 1;};
    if(o->kind==RecombinantMinor){if(id==6)return done(10);}
    else{
        if(id==6){
            bool yes=false;++o->schedule_tests;
            if(!b->schedule_matches||!b->schedule_matches(b->context,a,73,false,&yes))return 0;
            if(yes)id=10;
        }
        // Original injured/crouched state gate precedes weapon lookup.
        if(rd<unsigned char>(a,0xbf)&8u){
            if(id==1)return done(111);
            if(id==6||id==113)return done(112);
        }
        if(b->entry_count>sin_independent::kEntityHandleCapacity||(!b->entries&&b->entry_count))return 0;
        o->weapon_live=sin_independent::EntityHandleView(b->entries,b->entry_count).Resolve(rd<unsigned>(a,0x78c))!=nullptr;
        if(o->weapon_live){
            const char* name=id==1?"ACT_IDLE_BLUNT_WEAPON":id==6?"ACT_WALK_BLUNT_WEAPON":id==10?"ACT_RUN_BLUNT_WEAPON":nullptr;
            if(name){int v=-1;if(!lookup(*b,name,v))return 0;o->custom_activity=1;return done(v);}
            if(id==64)return done(65);
        }
    }
    if(!b->parent_activity||!b->parent_activity(b->context,a,id,&o->output))return 0;
    o->delegated=o->completed=1;return 1;
}
int __cdecl yaw(void* a,unsigned n,const ServerRecombinantPolicyBindings* b,ServerRecombinantPolicyResult* o) noexcept {
    if(!init(a,n,0x87c,b,o))return 0;
    const int id=rd<int>(a,0x878);o->input=id;
    if(o->kind==RecombinantMajor)o->yaw=id==1||id==6?25.f:id==10?35.f:20.f;
    else{
        int leap=-1;if(!lookup(*b,"ACT_LEAP_ATTACK",leap))return 0;
        o->yaw=id==leap?30.f:id==6||id==10?20.f:id==42||id==43?15.f:30.f;
    }
    o->completed=1;return 1;
}
int __cdecl pain(void* a,unsigned n,const ServerRecombinantPolicyBindings* b,ServerRecombinantPolicyResult* o) noexcept {
    if(!init(a,n,0xc0,b,o))return 0;
    if(rd<unsigned char>(a,0xbf)&8u){
        if(o->kind==RecombinantMajor){o->suppressed=o->completed=1;return 1;}
        ++o->random_calls;
        if(!b->random||!b->random(b->context,0,10,&o->random_value)||o->random_value<0||o->random_value>10)return 0;
        if(o->random_value>0){o->suppressed=o->completed=1;return 1;}
    }
    const char* sound=o->kind==RecombinantMajor?"NPC_Leperkin.Pain":"NPC_gib_hopper.Pain";
    if(!b->sound||!b->sound(b->context,a,sound,0,0))return 0;
    o->sound_calls=o->completed=1;return 1;
}
const ServerRecombinantPolicyApi api{1,sizeof(ServerRecombinantPolicyApi),&schedule,&activity,&yaw,&pain};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantPolicyApi=_SiN_GetServerRecombinantPolicyApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantPolicyApi* __cdecl SiN_GetServerRecombinantPolicyApi(unsigned v,unsigned n) noexcept {
    using namespace sin_port::native_weapon_tables;return v==1&&n==sizeof(ServerRecombinantPolicyApi)?&api:nullptr;
}
