#include "native_server_recombinant_task.h"
#include <cmath>
#include <cstring>
extern "C" const sin_port::native_weapon_tables::ServerNpcConditionApi* __cdecl SiN_GetServerNpcConditionApi(unsigned,unsigned) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerNpcTaskLifecycleApi* __cdecl SiN_GetServerNpcTaskLifecycleApi(unsigned,unsigned) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T>void wr(void* p,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
int __cdecl supports_major(int id,bool running) noexcept{
    return !running&&(id==54||id==55||id==69||id==121||id==139);
}
int __cdecl major(void* a,unsigned n,const ServerNpcScheduleTask* t,bool running,float now,const ServerRecombinantTaskBindings* b,ServerRecombinantTaskResult* o) noexcept {
    if(!o)return 0;*o={};o->activity=-1;
    if(!a||n<0xc54||rd<unsigned>(a,0)!=recombinant_major_vtable||!t||!b||!b->decision||!b->decision->policy||!std::isfinite(now))return 0;
    o->task=t->id;
    const auto& d=*b->decision;const auto& p=*d.policy;
    const auto* ca=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));
    const auto* life=SiN_GetServerNpcTaskLifecycleApi(1,sizeof(ServerNpcTaskLifecycleApi));
    if(!ca||!life)return 0;
    auto lifecycle=b->lifecycle;lifecycle.conditions=p.conditions;
    auto parent=[&](){++o->delegated;return b->parent&&b->parent(b->context,a,t,running);};
    auto done=[&](bool ok){o->completed=ok;return int(ok);};
    if(!supports_major(t->id,running))return done(parent());
    o->custom=1;
    auto random=[&](float lo,float hi){
        ++o->random_calls;return d.random_float&&d.random_float(d.context,lo,hi,&o->draw)&&std::isfinite(o->draw)&&o->draw>=lo&&o->draw<=hi;
    };
    switch(t->id){
    case 54:case 55:
        wr(a,0x988,rd<unsigned>(a,0x988)|0x40u);
        if(!d.flinch_activity||!d.flinch_activity(d.context,a,t->id==55,0,&o->activity))return 0;
        ++o->activity_calls;if(!b->activity||!b->activity(b->context,a,o->activity))return 0;
        if(!random(1.f,1.5f))return 0;
        o->deadline=static_cast<float>(double(now)+o->draw);if(!std::isfinite(o->deadline))return 0;
        wr(a,0x9e0,o->deadline);return done(true);
    case 69:{
        bool yes=false;++o->schedule_tests;
        if(!p.schedule_matches||!p.schedule_matches(p.context,a,39,true,&yes))return 0;
        if(yes){
            const float prior=rd<float>(a,0xc30);if(!std::isfinite(prior)||!random(0.f,1.f))return 0;
            if(o->draw<prior){
                ServerNpcConditionResult cr{};++o->condition_writes;
                if(!ca->store(a,n,69,NpcConditionCurrent,true,&p.conditions,&cr))return 0;
                o->probability=.2f;
            }else o->probability=static_cast<float>(double(prior)+.1f);
            if(!std::isfinite(o->probability))return 0;
            wr(a,0xc30,o->probability);
        }
        return done(parent());
    }
    case 121:
        wr<unsigned char>(a,0xc28,0);o->latch_cleared=1;return done(parent());
    case 139:{
        o->goal={4,{rd<float>(a,0xc48),rd<float>(a,0xc4c),rd<float>(a,0xc50)},-1,10,-1,-1,-1.f,-1.f,0,b->goal_owner};
        for(float x:o->goal.position)if(!std::isfinite(x))return 0;
        const unsigned nav=rd<unsigned>(a,0x8f8);
        bool yes=false;++o->goal_calls;
        if(!nav||!b->submit_goal||!b->submit_goal(b->context,nav,&o->goal,0,&yes))return 0;
        o->goal_accepted=yes;
        if(yes){
            // Original caches navigator before the enemy virtual call.
            const unsigned next=rd<unsigned>(a,0x8f8);void* enemy=nullptr;
            ++o->enemy_calls;if(!b->enemy||!b->enemy(b->context,a,&enemy))return 0;
            ++o->goal_enemy_calls;if(!next||!b->goal_enemy||!b->goal_enemy(b->context,next,enemy))return 0;
            return done(life->complete(a,n,now,false,&lifecycle,&o->lifecycle));
        }
        wr(a,0xc44,0xffffffffu);o->handle_cleared=1;
        if(!b->failure_token||!b->failure_token(b->context,"Unable to navigate to physics attack target!\n",&o->failure)||!o->failure)return 0;
        return done(life->fail(a,n,now,o->failure,&lifecycle,&o->lifecycle));
    }}
    return 0;
}
const ServerRecombinantTaskApi api{1,sizeof(ServerRecombinantTaskApi),&supports_major,&major};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantTaskApi=_SiN_GetServerRecombinantTaskApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantTaskApi* __cdecl SiN_GetServerRecombinantTaskApi(unsigned v,unsigned n) noexcept{
    using namespace sin_port::native_weapon_tables;return v==1&&n==sizeof(ServerRecombinantTaskApi)?&api:nullptr;
}
