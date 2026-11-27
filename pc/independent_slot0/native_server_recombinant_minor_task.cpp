#include "native_server_recombinant_minor_task.h"
#include <cmath>
#include <cstring>
extern "C" const sin_port::native_weapon_tables::ServerNpcTaskLifecycleApi* __cdecl SiN_GetServerNpcTaskLifecycleApi(unsigned,unsigned) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
unsigned __cdecl route(int id,bool running) noexcept{
    if(running)return id==67||id==95||id==121||id==140||id==142?MinorTaskNative:MinorTaskInherited;
    if(id==140)return MinorTaskUnrecovered;
    return id==13||id==67||id==131||id==141||id==142||id==143?MinorTaskNative:MinorTaskInherited;
}
struct Call{
    void* a;unsigned n;const ServerNpcScheduleTask& t;bool running;float now;
    const ServerRecombinantMinorTaskBindings& b;ServerRecombinantMinorTaskResult& o;
    const ServerRecombinantTaskBindings& g;const ServerRecombinantMinorBindings& m;
    const ServerRecombinantPolicyBindings& p;const ServerNpcTaskLifecycleApi* life;
    ServerNpcTaskLifecycleBindings lifecycle;
    bool parent(){++o.delegated;return g.parent&&g.parent(g.context,a,&t,running);}
    bool complete(){return life->complete(a,n,now,false,&lifecycle,&o.lifecycle);}
    bool fail(int id){o.failure=id;return life->fail(a,n,now,id,&lifecycle,&o.lifecycle);}
    bool enemy(void*& out){++o.enemy_calls;return g.enemy&&g.enemy(g.context,a,&out);}
    void* target(unsigned off){++o.target_calls;return sin_independent::EntityHandleView(p.entries,p.entry_count).Resolve(rd<unsigned>(a,off));}
    bool vector_valid(const float v[3]){return std::isfinite(v[0])&&std::isfinite(v[1])&&std::isfinite(v[2]);}
    bool face(unsigned motor,const float v[3],float delta,bool center){
        if(!motor||!vector_valid(v))return false;
        std::memcpy(o.vector,v,12);++o.face_calls;
        return b.face&&b.face(b.context,motor,v,delta,center);
    }
    bool leap(){
        ++o.leap_calls;return b.leap&&b.leap(b.context,a,5.f,0);
    }
    bool arrival(int activity){
        o.activity=activity;++o.arrival_calls;
        return b.set_arrival&&rd<unsigned>(a,0x8f8)&&b.set_arrival(b.context,rd<unsigned>(a,0x8f8),activity);
    }
    bool grounded(){o.grounded=(rd<unsigned char>(a,0xbc)&1)!=0;return o.grounded!=0;}
    bool ground(){o.ground_live=target(0x238)!=nullptr;return o.ground_live!=0;}
    bool start(){
        void* e=nullptr;
        switch(t.id){
        case 13:
            if(!enemy(e))return false;
            if(e){
                if(!enemy(e)||!e)return false;
                const unsigned motor=rd<unsigned>(a,0x908);float v[3]{};
                if(!b.world_center||!b.world_center(b.context,e,v)||!face(motor,v,0.f,true))return false;
            }
            return parent();
        case 67:return leap();
        case 131:
            ++o.movement_calls;
            return b.touch->set_movement&&b.touch->set_movement(b.touch->context,a,3,0)&&parent();
        case 141:{
            e=target(0x8a8);if(!e)return fail(11);
            e=target(0x8a8);bool blocked=false;
            if(!e||!b.target_gate||!b.target_gate(b.context,a,e,&blocked))return false;
            if(blocked)return fail(11);
            e=target(0x8a8);float v[3]{};
            if(!e||!b.origin||!b.origin(b.context,e,v)||!vector_valid(v))return false;
            o.goal={4,{v[0],v[1],v[2]},-1,-1,-1,-1,-1.f,-1.f,0,b.goal_owner};
            e=target(0x8a8);
            if(!e||!b.goal_adjust||!b.goal_adjust(b.context,a,e,&o.goal))return false;
            o.goal_adjusted=1;bool yes=false;++o.goal_calls;
            const unsigned nav=rd<unsigned>(a,0x8f8);
            if(!nav||!vector_valid(o.goal.position)||!g.submit_goal||!g.submit_goal(g.context,nav,&o.goal,2,&yes))return false;
            o.goal_accepted=yes;
            if(yes)return complete();
            if(!b.diagnostic||!b.diagnostic(b.context,2,"TASK_MINOR_GOTO_TARGET failed!!\n"))return false;
            ++o.diagnostic_calls;return fail(11);
        }
        case 142:
            o.activity=b.custom_activity;++o.activity_calls;
            return g.activity&&g.activity(g.context,a,o.activity);
        case 143:return arrival(b.arrival_activity)&&complete();
        default:return parent();
        }
    }
    bool run(){
        switch(t.id){
        case 95:
            ++o.reset_calls;return b.aim_reset&&b.aim_reset(b.context,a,0,0)&&parent();
        case 121:{
            bool yes=false;++o.schedule_tests;
            if(!p.schedule_matches||!p.schedule_matches(p.context,a,85,true,&yes))return false;
            if(yes){
                void* e=nullptr;if(!enemy(e))return false;
                if(e){
                    const unsigned motor=rd<unsigned>(a,0x908);float v[3]{};
                    if(!enemy(e)||!e||!b.origin||!b.origin(b.context,e,v)||!face(motor,v,-1.f,false))return false;
                }
            }
            return parent();
        }
        case 140:{
            if(grounded())return complete();
            const unsigned motor=rd<unsigned>(a,0x908);
            if(rd<unsigned>(a,0xb8)&0x1000u){
                if(!m.refresh_velocity||!m.refresh_velocity(m.context,a))return false;o.velocity_refresh=1;
            }
            float origin[3]{};
            if(!b.origin||!b.origin(b.context,a,origin))return false;
            for(unsigned i=0;i<3;++i)origin[i]=static_cast<float>(double(origin[i])+rd<float>(a,0x250+i*4));
            return face(motor,origin,-1.f,false);
        }
        case 142:{
            bool yes=false;++o.special_calls;
            if(!b.special_gate||!b.special_gate(b.context,a,&yes))return false;
            return !yes&&!grounded()&&!ground()?true:complete();
        }
        case 67:{
            if(grounded()||ground())return complete();
            const float deadline=rd<float>(a,0x9dc);
            if(!std::isfinite(deadline))return 0;
            if(now<deadline)return true;
            bool yes=false;++o.probe_calls;
            if(!b.support_probe||!b.support_probe(b.context,a,&yes))return false;
            return yes?complete():leap();
        }
        default:return parent();
        }
    }
};
int __cdecl execute(void* a,unsigned n,const ServerNpcScheduleTask* t,bool running,float now,const ServerRecombinantMinorTaskBindings* b,ServerRecombinantMinorTaskResult* o) noexcept {
    if(!o)return 0;*o={};o->activity=-1;
    if(!a||n<0xc48||rd<unsigned>(a,0)!=recombinant_minor_vtable||!t||!std::isfinite(now)||
       !b||!b->task||!b->task->decision||!b->task->decision->policy||!b->touch||!b->touch->minor)return 0;
    o->task=t->id;o->route=route(t->id,running);
    if(o->route==MinorTaskUnrecovered)return 0;
    const auto& p=*b->task->decision->policy;
    if(p.entry_count>sin_independent::kEntityHandleCapacity||(!p.entries&&p.entry_count))return 0;
    const auto* life=SiN_GetServerNpcTaskLifecycleApi(1,sizeof(ServerNpcTaskLifecycleApi));if(!life)return 0;
    auto lifecycle=b->task->lifecycle;lifecycle.conditions=p.conditions;
    Call c{a,n,*t,running,now,*b,*o,*b->task,*b->touch->minor,p,life,lifecycle};
    o->completed=running?c.run():c.start();return o->completed;
}
const ServerRecombinantMinorTaskApi api{1,sizeof(ServerRecombinantMinorTaskApi),&route,&execute};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantMinorTaskApi=_SiN_GetServerRecombinantMinorTaskApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantMinorTaskApi* __cdecl SiN_GetServerRecombinantMinorTaskApi(unsigned v,unsigned n) noexcept{
    using namespace sin_port::native_weapon_tables;return v==1&&n==sizeof(ServerRecombinantMinorTaskApi)?&api:nullptr;
}
