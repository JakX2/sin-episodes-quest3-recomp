#include "native_server_recombinant_decision.h"
#include <cmath>
#include <cstring>
extern "C" const sin_port::native_weapon_tables::ServerNpcConditionApi* __cdecl SiN_GetServerNpcConditionApi(unsigned,unsigned) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerNpcScheduleStateApi* __cdecl SiN_GetServerNpcScheduleStateApi(unsigned,unsigned) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T>void wr(void* p,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
struct Call{
    void* a;unsigned n;
    const ServerRecombinantDecisionBindings& b;
    ServerRecombinantDecisionResult& o;
    const ServerNpcConditionApi* conditions;
    const ServerRecombinantPolicyBindings& p;
    bool test(int id,bool& yes){
        ServerNpcConditionResult r{};++o.condition_tests;
        if(!conditions->test(a,n,id,NpcConditionCurrent,&p.conditions,&r))return false;
        yes=r.value!=0;return true;
    }
    bool store(int id,unsigned bank,bool value){
        ServerNpcConditionResult r{};++o.condition_writes;
        if(!conditions->store(a,n,id,bank,value,&p.conditions,&r))return false;
        o.changed+=r.changed;return true;
    }
    bool match(int id,bool ideal,bool& yes){
        ++o.schedule_tests;return p.schedule_matches&&p.schedule_matches(p.context,a,id,ideal,&yes);
    }
    void* entity(unsigned off){
        return sin_independent::EntityHandleView(p.entries,p.entry_count).Resolve(rd<unsigned>(a,off));
    }
    bool parent(){
        ++o.delegated;return b.parent_select&&b.parent_select(b.context,a,&o.selected);
    }
    bool draw(float& x){
        if(o.random_calls>=2)return false;
        ++o.random_calls;
        if(!b.random_float||!b.random_float(b.context,0.f,1.f,&x)||!std::isfinite(x)||x<0.f||x>1.f)return false;
        o.draws[o.random_calls-1]=x;return true;
    }
    bool major_flinch(){
        if(rd<unsigned>(a,0x988)&0x40u)return true;
        bool yes=false;
        if(!test(17,yes))return false;
        if(yes){
            ++o.activity_calls;
            if(!b.flinch_activity||!b.flinch_activity(b.context,a,1,0,&o.activity))return false;
            ++o.model_calls;
            if(!b.model_available||!b.model_available(b.context,a,&yes))return false;
            if(yes){
                ++o.sequence_calls;
                if(!b.sequence_for_activity||!b.sequence_for_activity(b.context,a,o.activity,&o.sequence))return false;
                if(o.sequence!=-1)o.selected=22;
            }
            return true; // do not fall through to condition16
        }
        if(!test(16,yes))return false;
        if(yes){
            ++o.activity_calls;
            if(!b.flinch_activity||!b.flinch_activity(b.context,a,0,0,&o.activity))return false;
            ++o.flinch_calls;
            if(!b.small_flinch||!b.small_flinch(b.context,a,o.activity,&yes))return false;
            if(yes)o.selected=21;
        }
        return true;
    }
};
bool init(void* a,unsigned n,unsigned extent,const ServerRecombinantDecisionBindings* b,ServerRecombinantDecisionResult* o){
    if(!o)return false;*o={};o->activity=o->sequence=o->prior=-1;
    if(!a||n<extent||!b||!b->policy||b->policy->entry_count>sin_independent::kEntityHandleCapacity||
       (!b->policy->entries&&b->policy->entry_count))return false;
    switch(rd<unsigned>(a,0)){
    case recombinant_major_vtable:o->kind=RecombinantMajor;break;
    case recombinant_minor_vtable:o->kind=RecombinantMinor;break;
    default:return false;
    }
    return true;
}
int __cdecl select(void* a,unsigned n,const ServerRecombinantDecisionBindings* b,ServerRecombinantDecisionResult* o) noexcept {
    if(!init(a,n,0xc48,b,o))return 0;
    const auto* api=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));if(!api)return 0;
    Call c{a,n,*b,*o,api,*b->policy};
    auto done=[&](int v){o->selected=v;o->completed=1;return 1;};
    bool yes=false;
    if(o->kind==RecombinantMinor){
        if(!c.test(67,yes))return 0;
        if(yes)return c.store(67,NpcConditionCurrent,false)?done(81):0;
        if(!c.test(68,yes))return 0;
        if(!yes&&!c.test(66,yes))return 0;
        if(yes)return c.store(68,NpcConditionCurrent,false)?done(82):0;
        if(!c.parent())return 0;o->prior=o->selected;
        if(o->selected!=41&&o->selected!=42)return done(o->selected);
        float draw=0;if(!c.draw(draw))return 0;
        if(draw<0.6f)return done(91);
        const float threshold=rd<float>(a,0xc44);
        if(!std::isfinite(threshold)||!c.draw(draw))return 0;
        return done(draw<threshold?41:42);
    }
    if(!c.test(69,yes))return 0;if(yes)return done(83);
    if(!c.test(67,yes))return 0;if(yes)return done(86);
    if(!c.test(22,yes))return 0;
    if(yes){
        wr<unsigned char>(a,0xc28,0);o->latch_cleared=1;
        o->goal_live=c.entity(0xc44)!=nullptr;
        return done(o->goal_live?86:39);
    }
    if(!c.major_flinch())return 0;if(o->selected)return done(o->selected);
    if(rd<int>(a,0x850)==3){
        if(!c.test(25,yes))return 0;
        if(yes){
            if(void* enemy=c.entity(0x8a0)){
                o->enemy_live=1;unsigned size=0;
                if(!b->extent||!b->extent(b->context,enemy,&size)||size<0x36c||!std::isfinite(b->major_far_distance))return 0;
                double sum=0;
                for(unsigned i=0;i<3;++i){
                    const float d=static_cast<float>(double(rd<float>(a,0x360+4*i))-rd<float>(enemy,0x360+4*i));
                    if(!std::isfinite(d))return 0;sum+=double(d)*d;
                }
                const float squared=static_cast<float>(sum);if(!std::isfinite(squared))return 0;
                o->distance=std::sqrt(squared);
                // Original +770 on Major is unconditional true.
                if(o->distance>=b->major_far_distance)return done(81);
            }
        }
    }
    if(!c.parent())return 0;return done(o->selected);
}
int __cdecl gather_major(void* a,unsigned n,const ServerRecombinantDecisionBindings* b,ServerRecombinantDecisionResult* o) noexcept{
    if(!init(a,n,0xc48,b,o)||o->kind!=RecombinantMajor)return 0;
    const auto* api=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));if(!api)return 0;
    Call c{a,n,*b,*o,api,*b->policy};
    if(!c.store(66,NpcConditionCurrent,false)||!b->parent_gather||!b->parent_gather(b->context,a))return 0;
    ++o->delegated;o->goal_live=c.entity(0xc44)!=nullptr;
    if(!c.store(67,NpcConditionCurrent,o->goal_live!=0)||!c.store(20,NpcConditionCurrent,false))return 0;
    o->completed=1;return 1;
}
int __cdecl interrupts(void* a,unsigned n,const ServerRecombinantDecisionBindings* b,ServerRecombinantDecisionResult* o) noexcept{
    if(!init(a,n,0x80c,b,o))return 0;
    const auto* api=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));if(!api)return 0;
    Call c{a,n,*b,*o,api,*b->policy};bool match=false;
    if(!c.match(86,true,match))return 0;
    if(o->kind==RecombinantMinor){
        if(!c.store(66,NpcConditionInterrupt,!match)||!c.store(17,NpcConditionInterrupt,false))return 0;
        if(!b->parent_interrupts||!b->parent_interrupts(b->context,a))return 0;
        ++o->delegated;
    }else{
        if(!match&&!c.store(67,NpcConditionInterrupt,true))return 0;
        if(!b->parent_interrupts||!b->parent_interrupts(b->context,a))return 0;
        ++o->delegated;
        if(!c.match(39,true,match))return 0;
        if((match||(rd<unsigned char>(a,0xbf)&8u))&&
           (!c.store(16,NpcConditionInterrupt,false)||!c.store(17,NpcConditionInterrupt,false)))return 0;
    }
    o->completed=1;return 1;
}
int __cdecl invalidated_major(void* a,unsigned n,float now,const ServerRecombinantDecisionBindings* b,ServerRecombinantDecisionResult* o) noexcept{
    if(!init(a,n,0xc24,b,o)||o->kind!=RecombinantMajor||!std::isfinite(now))return 0;
    o->deadline=rd<float>(a,0xc20);if(!std::isfinite(o->deadline))return 0;
    if(double(now)+1.<o->deadline){o->deadline=static_cast<float>(double(now)+.5);if(!std::isfinite(o->deadline))return 0;wr(a,0xc20,o->deadline);o->deadline_changed=1;}
    if(!b->parent_invalidated||!b->parent_invalidated(b->context,a))return 0;
    o->delegated=o->completed=1;return 1;
}
int __cdecl flinch_maintenance_major(void* a,unsigned n,float now,const ServerRecombinantDecisionBindings* b,ServerRecombinantDecisionResult* o) noexcept{
    if(!init(a,n,0x9e4,b,o)||o->kind!=RecombinantMajor||!std::isfinite(now))return 0;
    const auto* api=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));
    const auto* schedule=SiN_GetServerNpcScheduleStateApi(1,sizeof(ServerNpcScheduleStateApi));
    if(!api||!schedule)return 0;
    Call c{a,n,*b,*o,api,*b->policy};
    auto play=[&](){++o->flinch_calls;return b->play_flinch&&b->play_flinch(b->context,a);};
    bool yes=false;
    if(!c.test(17,yes))return 0;
    if(yes){
        if(rd<unsigned>(a,0x988)&0x40u){
            if(!play()||!c.store(17,NpcConditionCurrent,false))return 0;
        }else{
            unsigned interrupt=0;
            if(!b->schedule||!schedule->interrupt(a,n,17,false,b->schedule,&interrupt))return 0;
            if(!interrupt&&!play())return 0;
        }
    }else{
        if(!c.test(16,yes))return 0;
        if(yes&&(rd<unsigned>(a,0x988)&0x40u)&&(!play()||!c.store(16,NpcConditionCurrent,false)))return 0;
    }
    unsigned flags=rd<unsigned>(a,0x988);
    if(flags&0x40u){
        o->deadline=rd<float>(a,0x9e0);if(!std::isfinite(o->deadline))return 0;
        if(now>o->deadline){wr(a,0x988,flags&~0x40u);o->latch_cleared=1;}
    }
    o->completed=1;return 1;
}
const ServerRecombinantDecisionApi api{1,sizeof(ServerRecombinantDecisionApi),&select,&gather_major,&interrupts,&invalidated_major,&flinch_maintenance_major};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantDecisionApi=_SiN_GetServerRecombinantDecisionApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantDecisionApi* __cdecl SiN_GetServerRecombinantDecisionApi(unsigned v,unsigned n) noexcept{
    using namespace sin_port::native_weapon_tables;return v==1&&n==sizeof(ServerRecombinantDecisionApi)?&api:nullptr;
}
