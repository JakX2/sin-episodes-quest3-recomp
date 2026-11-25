#include "native_server_npc_shared_attack.h"
#include <cmath>
#include <cstring>
#include <limits>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcTaskLifecycleApi* __cdecl SiN_GetServerNpcTaskLifecycleApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcTransitionApi* __cdecl SiN_GetServerNpcTransitionApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleResolutionApi* __cdecl SiN_GetServerNpcScheduleResolutionApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleNamespaceApi* __cdecl SiN_GetServerNpcScheduleNamespaceApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleStateApi* __cdecl SiN_GetServerNpcScheduleStateApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcSharedAttackApi* __cdecl SiN_GetServerNpcSharedAttackApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* p,unsigned off) noexcept{T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T>void wr(void* p,unsigned off,T v) noexcept{std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
bool integer(double v,int& out) noexcept{
    if(!std::isfinite(v)||v<std::numeric_limits<int>::min()||v>std::numeric_limits<int>::max())return false;
    out=static_cast<int>(v);return true;
}
bool ticks(float seconds,int& out) noexcept{
    const float scaled=seconds*10.f;
    return integer(std::ceil(static_cast<double>(scaled)),out);
}
int __cdecl wait(void* a,unsigned n,float now,float low,float high,const ServerNpcSharedTaskBindings* b,int* out) noexcept{
    if(!out)return 0;*out=0;
    if(!a||n<0x9e0||!std::isfinite(now)||!std::isfinite(low)||!std::isfinite(high)||!ticks(low,*out))return 0;
    if(high!=0.f){
        int hi=0;if(!ticks(high,hi))return 0;
        const int lo=*out?*out:1;
        if(lo>hi||!b||!b->random||!b->random(b->context,lo,hi,out)||*out<lo||*out>hi)return 0;
    }
    const float deadline=static_cast<float>(static_cast<double>(now)+static_cast<double>(*out)*.1);
    if(!std::isfinite(deadline))return 0;
    wr(a,0x9dc,deadline);return 1;
}
int __cdecl cancel_wait(void* a,unsigned n) noexcept{
    if(!a||n<0x9e0)return 0;wr(a,0x9dc,0x7f7fffffu);return 1;
}
int __cdecl wait_state(const void* a,unsigned n,float now,unsigned* active,unsigned* expired) noexcept{
    if(!active||!expired)return 0;*active=*expired=0;
    if(!a||n<0x9e0||!std::isfinite(now))return 0;
    const float deadline=rd<float>(a,0x9dc);if(!std::isfinite(deadline))return 0;
    *active=rd<unsigned>(a,0x9dc)!=0x7f7fffffu;*expired=now>=deadline;return 1;
}
int __cdecl supports(int id,bool running) noexcept{
    if(running)return id==2||id==4||id==5||id==86||id==92||id==93||id==114;
    return id==1||id==2||id==3||id==4||id==5||id==59||id==86||id==87||id==88||id==91||(id>=92&&id<=96)||id==114||id==119||id==120;
}
int __cdecl yaw_error(float actual,float desired,float* quantized,double* error) noexcept{
    if(!quantized||!error)return 0;*quantized=0;*error=0;
    if(!std::isfinite(actual)||!std::isfinite(desired))return 0;
    int fixed=0;
    // Original x87 FMUL with the exact float32 constants, FISTP toward zero.
    if(!integer(std::trunc(static_cast<double>(actual)*182.0444488525390625),fixed))return 0;
    *quantized=static_cast<float>(static_cast<unsigned>(fixed)&0xffffu)*.0054931640625f;
    const double delta=static_cast<double>(desired)-*quantized;
    double wrapped=std::fmod(delta,360.0);
    if(delta>0){if(wrapped>=180.0)wrapped-=360.0;}
    else if(wrapped<=-180.0)wrapped+=360.0;
    *error=wrapped;return 1;
}
int face60(void* a,unsigned n,bool running,float now,const ServerNpcSharedTaskBindings& s,const ServerNpcSharedFacingBindings& f,ServerNpcSharedFacingResult& out) noexcept{
    const auto motor_token=rd<std::uint32_t>(a,0x908);
    if(!motor_token||!f.motor_storage||!f.aim_vector||!f.aligned||!f.desired_yaw)return 0;
    void* motor=nullptr;unsigned size=0;
    if(!f.motor_storage(f.context,motor_token,&motor,&size)||!motor||size<0x18)return 0;
    float v[3]{};bool aligned=false;
    if(!f.aim_vector(f.context,a,v)||!std::isfinite(v[0])||!std::isfinite(v[1])||!std::isfinite(v[2])||
       !f.aligned(f.context,a,v,&aligned))return 0;
    out.queried=1;
    if(!aligned){
        float v_yaw=0;
        if(!f.vector_yaw||!f.vector_yaw(f.context,a,v,&v_yaw)||!std::isfinite(v_yaw))return 0;
        wr(motor,0x14,v_yaw);out.vector_written=1;
    }
    double target=0;
    if(!f.desired_yaw(f.context,a,true,&target)||!std::isfinite(target))return 0;
    const float stored=static_cast<float>(target);
    if(!std::isfinite(stored))return 0;
    const double difference=std::fabs(target-rd<float>(motor,0x14));
    if(aligned&&!std::isfinite(difference))return 0;
    auto complete=[&](){
        const auto* life=SiN_GetServerNpcTaskLifecycleApi(1,sizeof(ServerNpcTaskLifecycleApi));
        if(!life||!life->complete(a,n,now,false,&s.lifecycle,&out.shared.lifecycle))return 0;
        out.completed=out.shared.lifecycle.completed;return 1;
    };
    if(!running&&aligned&&difference<1.0)return complete();
    if(!aligned||(!running&&difference>=1.0)||(running&&difference>1.0)){
        wr(motor,0x14,stored);out.yaw_written=1;
    }
    if(!running){
        if(!f.begin_turn||!f.begin_turn(f.context,a))return 0;
        out.begun=1;return 1;
    }
    if(!f.step_turn||!f.step_turn(f.context,motor,-1))return 0;
    out.stepped=1;
    // The motor callback may update the actor's current yaw and motor target.
    if(!yaw_error(rd<float>(a,0x370),rd<float>(motor,0x14),&out.current_quantized,&out.error))return 0;
    return std::fabs(out.error)>.006||complete();
}
int target_animation(void* a,unsigned n,int task,float now,const ServerNpcSharedTaskBindings& s,const ServerNpcSharedTargetAnimationBindings& b,ServerNpcSharedTargetAnimationResult& out) noexcept{
    void* target=nullptr;
    if(task==96){
        if(b.count>sin_independent::kEntityHandleCapacity||(b.count&&!b.entries))return 0;
        out.target_handle=rd<unsigned>(a,0x8a8);
        target=sin_independent::EntityHandleView(b.entries,b.count).Resolve(out.target_handle);
    }else if(!b.current_target||!b.current_target(b.context,a,&target))return 0;
    out.target_live=target!=nullptr;
    if(target){
        const auto token=rd<unsigned>(a,0x908);
        void* motor=nullptr;unsigned size=0;
        if(!token||!b.motor_storage||!b.absolute_position||!b.step_turn||
           !b.motor_storage(b.context,token,&motor,&size)||!motor||size<0x18)return 0;
        float pos[3]{};
        if(!b.absolute_position(b.context,target,pos))return 0;
        for(unsigned i=0;i<3;++i){
            const float origin=rd<float>(a,0x360+4*i);
            if(!std::isfinite(pos[i])||!std::isfinite(origin))return 0;
            out.direction[i]=static_cast<float>(static_cast<double>(pos[i])-origin);
            if(!std::isfinite(out.direction[i]))return 0;
        }
        const double x=out.direction[0],y=out.direction[1];
        // 2228b220: zero XY returns zero; FPATAN * exact float32 degrees/radian.
        double yaw=(x==0.0&&y==0.0)?0.0:std::atan2(y,x)*57.295780181884765625;
        if(yaw<0.0)yaw+=360.0;
        out.yaw=static_cast<float>(yaw);
        // 2206bff0 with -2.0 sentinel writes +14 only, then +48(-1).
        wr(motor,0x14,out.yaw);out.motor_written=1;
        if(!b.step_turn(b.context,motor,-1))return 0;
        out.stepped=1;
    }
    const auto* life=SiN_GetServerNpcTaskLifecycleApi(1,sizeof(ServerNpcTaskLifecycleApi));
    if(!life||!life->animation_finished(a,n,&out.animation_finished))return 0;
    return !out.animation_finished||life->complete(a,n,now,false,&s.lifecycle,&out.shared.lifecycle);
}
int execute_impl(void* a,unsigned n,const ServerNpcScheduleTask* t,bool running,float now,const ServerNpcSharedTaskBindings* b,ServerNpcSharedTaskResult* out,const ServerNpcSharedFacingBindings* facing=nullptr,ServerNpcSharedFacingResult* facing_out=nullptr,const ServerNpcSharedTargetAnimationBindings* target=nullptr,ServerNpcSharedTargetAnimationResult* target_out=nullptr) noexcept{
    if(!out)return 0;*out={};out->activity=-1;
    if(!a||n<0xc1c||!t||!b||!std::isfinite(now))return 0;
    // The existing class schedule resolver has a larger actor extent.
    if(!running&&t->id==87&&n<0xc34)return 0;
    out->task=t->id;
    // 2202d930 / 2202daf0 route task67 directly to +780 / +784.
    if(t->id==67){out->special_called=1;return b->special67&&b->special67(b->context,a,t,running);}
    if(const auto token=rd<std::uint32_t>(a,0xc18)){
        void* component=nullptr;unsigned size=0;
        if(!b->component_storage||!b->component_storage(b->context,token,&component,&size)||!component||size<9||!b->component_task)return 0;
        wr<std::uint8_t>(component,8,1);out->component_called=1;
        if(!b->component_task(b->context,token,t,running))return 0;
        if(rd<std::uint8_t>(component,8)){out->component_handled=1;return 1;}
    }
    if(t->id==60&&facing&&facing_out){out->native=1;return face60(a,n,running,now,*b,*facing,*facing_out);}
    if(running&&t->id>=94&&t->id<=96&&target&&target_out){out->native=1;return target_animation(a,n,t->id,now,*b,*target,*target_out);}
    if(!supports(t->id,running)){out->parent_called=1;return b->parent&&b->parent(b->context,a,t,running);}
    out->native=1;
    const auto* life=SiN_GetServerNpcTaskLifecycleApi(1,sizeof(ServerNpcTaskLifecycleApi));if(!life)return 0;
    auto complete=[&](){return life->complete(a,n,now,false,&b->lifecycle,&out->lifecycle)!=0;};
    auto pending=[&](int id){
        out->activity=id;
        if(b->transition){
            const auto* api=SiN_GetServerNpcTransitionApi(1,sizeof(ServerNpcTransitionApi));
            return api&&api->set_pending(a,n,id,b->transition,&out->transition);
        }
        return b->pending&&b->pending(b->context,a,id);
    };
    if(running){
        if(t->id==86)return rd<int>(a,0x3ec)!=rd<int>(a,0x884)||complete();
        if(t->id==92||t->id==93){
            out->turn_called=1;if(!b->turn||!b->turn(b->context,a))return 0;
            unsigned yes=0;if(!life->animation_finished(a,n,&yes))return 0;return !yes||complete();
        }
        if(t->id==4||t->id==5){
            bool yes=false;out->aim_called=1;
            if(!b->aim_query||!b->aim_query(b->context,a,&yes))return 0;
            if(!yes&&(!b->aim_update||!b->aim_update(b->context,a)))return 0;
        }
        unsigned active=0,expired=0;if(!wait_state(a,n,now,&active,&expired))return 0;
        return !expired||complete();
    }
    if(t->id==1){wr(a,0x878,0);return complete();}
    if(t->id==3)return complete();
    if(t->id==91){wr(a,0x7bc,0);return complete();}
    if(t->id==2||t->id==4||t->id==5||t->id==59||t->id==114){
        const bool random=t->id==5||t->id==114;
        if(!wait(a,n,now,random?0.f:t->parameter,random?t->parameter:0.f,b,&out->ticks))return 0;
        out->timer_written=1;return 1;
    }
    int value=0;if(!integer(std::trunc(static_cast<double>(t->parameter)),value))return 0;
    switch(t->id){
    case 86:if(!value){wr(a,0x878,0);return 1;}return pending(value);
    case 92:case 93:case 94:case 95:case 96:return pending(value);
    case 88:wr(a,0x7bc,value);return complete();
    case 119:wr(a,0x988,rd<unsigned>(a,0x988)|static_cast<unsigned>(value));return complete();
    case 120:wr(a,0x988,rd<unsigned>(a,0x988)&~static_cast<unsigned>(value));return complete();
    case 87:{
        if(!b->resolution)return 0;
        const auto* api=SiN_GetServerNpcScheduleResolutionApi(1,sizeof(ServerNpcScheduleResolutionApi));
        const auto* ns=SiN_GetServerNpcScheduleNamespaceApi(1,sizeof(ServerNpcScheduleNamespaceApi));
        const auto* state=SiN_GetServerNpcScheduleStateApi(1,sizeof(ServerNpcScheduleStateApi));
        if(!api||!ns||!state||!api->find(a,n,value,b->resolution,&out->resolution))return 0;
        if(!out->resolution.token){out->failed_code=5;return life->fail(a,n,now,5,&b->lifecycle,&out->lifecycle);}
        const auto& sb=b->resolution->grunt.schedule;int ideal=-1;
        if(!ns->to_global(a,value,&sb.conditions,&ideal))return 0;
        wr(a,0x79c,ideal);
        if(!state->install(a,n,out->resolution.token,now,&sb,&out->schedule))return 0;
        out->schedule_set=1;return complete();
    }
    }
    return 0;
}
int __cdecl execute(void* a,unsigned n,const ServerNpcScheduleTask* t,bool running,float now,const ServerNpcSharedTaskBindings* b,ServerNpcSharedTaskResult* out) noexcept{
    return execute_impl(a,n,t,running,now,b,out);
}
int __cdecl execute_facing(void* a,unsigned n,const ServerNpcScheduleTask* t,bool running,float now,const ServerNpcSharedTaskBindings* b,const ServerNpcSharedFacingBindings* f,ServerNpcSharedFacingResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!t||t->id!=60||!f)return 0;
    return execute_impl(a,n,t,running,now,b,&out->shared,f,out);
}
int __cdecl execute_target_animation(void* a,unsigned n,const ServerNpcScheduleTask* t,float now,const ServerNpcSharedTaskBindings* b,const ServerNpcSharedTargetAnimationBindings* target,ServerNpcSharedTargetAnimationResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!t||t->id<94||t->id>96||!target)return 0;
    return execute_impl(a,n,t,true,now,b,&out->shared,nullptr,nullptr,target,out);
}
const ServerNpcSharedTargetAnimationApi target_animation_api{1,sizeof(ServerNpcSharedTargetAnimationApi),&execute_target_animation};
const ServerNpcSharedFacingApi facing_api{1,sizeof(ServerNpcSharedFacingApi),&yaw_error,&execute_facing};
const ServerNpcSharedTaskApi api{1,sizeof(ServerNpcSharedTaskApi),&wait,&cancel_wait,&wait_state,&supports,&execute};
 }
int bind_shared_parent_internal(void* actor,unsigned bytes,float now,const ServerGruntTaskBindings* original,const ServerNpcScheduleResolutionBindings* resolution,const ServerNpcSharedTaskBindings* shared,ServerNpcSharedTaskStats* stats,ServerNpcSharedParentSession* session,ServerGruntTaskBindings* bound,const ServerNpcSharedAttackBindings* attack,ServerNpcSharedAttackStats* attack_stats,const ServerNpcSharedFacingBindings* facing,ServerNpcSharedFacingStats* facing_stats) noexcept{
    if(!actor||!original||!resolution||!shared||!stats||!session||!bound)return 0;
    if((attack&&!attack_stats)||(facing&&!facing_stats))return 0;
    *stats={};if(attack_stats)*attack_stats={};if(facing_stats)*facing_stats={};
    *session={actor,bytes,now,*original,*shared,stats,attack,attack_stats,facing,facing_stats};*bound=*original;
    session->shared.lifecycle=original->lifecycle;
    session->shared.transition=original->transition;
    session->shared.resolution=resolution;
    bound->context=session;
    bound->parent=[](void* p,void* a,const ServerNpcScheduleTask* t,bool running) noexcept -> int{
        auto& q=*static_cast<ServerNpcSharedParentSession*>(p);
        if(a!=q.actor||!t)return 0;
        if(q.facing&&t->id==60){
            auto& st=*q.facing_stats;++st.calls;
            const int ok=facing_api.execute(a,q.bytes,t,running,q.now,&q.shared,q.facing,&st.last);
            st.native+=st.last.shared.native;st.components+=st.last.shared.component_handled;return ok;
        }
        if(q.attack){
            const auto* native=SiN_GetServerNpcSharedAttackApi(1,sizeof(ServerNpcSharedAttackApi));
            if(!native)return 0;
            if(native->supports(t->id)){
                auto bound=*q.attack;bound.shared=q.shared;bound.weapons=q.original.entities;
                auto& st=*q.attack_stats;++st.calls;
                const int ok=native->execute(a,q.bytes,t,running,q.now,&bound,&st.last);
                st.components+=st.last.component_handled;st.special+=st.last.special;return ok;
            }
        }
        auto& st=*q.stats;++st.calls;
        const int ok=api.execute(a,q.bytes,t,running,q.now,&q.shared,&st.last);
        st.native+=st.last.native;st.component_handled+=st.last.component_handled;
        st.special+=st.last.special_called;st.parent+=st.last.parent_called;
        return ok;
    };
    bound->special=original->special?+[](void* p,void* a,const ServerNpcScheduleTask* t,bool running) noexcept -> int{
        const auto& o=static_cast<ServerNpcSharedParentSession*>(p)->original;return o.special(o.context,a,t,running);
    }:nullptr;
    bound->storage=original->storage?+[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{
        const auto& o=static_cast<ServerNpcSharedParentSession*>(p)->original;return o.storage(o.context,token,out,n);
    }:nullptr;
    bound->peers=original->peers?+[](void* p,std::uint32_t token,const ServerGruntTaskPeer** out,unsigned* n) noexcept -> int{
        const auto& o=static_cast<ServerNpcSharedParentSession*>(p)->original;return o.peers(o.context,token,out,n);
    }:nullptr;
    bound->cue=original->cue?+[](void* p,void* a,unsigned slot,int x,int y) noexcept -> int{
        const auto& o=static_cast<ServerNpcSharedParentSession*>(p)->original;return o.cue(o.context,a,slot,x,y);
    }:nullptr;
    bound->set_enemy=original->set_enemy?+[](void* p,void* a,void* e,bool flag) noexcept -> int{
        const auto& o=static_cast<ServerNpcSharedParentSession*>(p)->original;return o.set_enemy(o.context,a,e,flag);
    }:nullptr;
    bound->finish_hook=original->finish_hook?+[](void* p,void* a) noexcept -> int{
        const auto& o=static_cast<ServerNpcSharedParentSession*>(p)->original;return o.finish_hook(o.context,a);
    }:nullptr;
    bound->turn_update=original->turn_update?+[](void* p,void* a) noexcept -> int{
        const auto& o=static_cast<ServerNpcSharedParentSession*>(p)->original;return o.turn_update(o.context,a);
    }:nullptr;
    bound->combat_target=original->combat_target?+[](void* p,void* e,void** out) noexcept -> int{
        const auto& o=static_cast<ServerNpcSharedParentSession*>(p)->original;return o.combat_target(o.context,e,out);
    }:nullptr;
    bound->target_gate=original->target_gate?+[](void* p,void* e,void* a,bool* out) noexcept -> int{
        const auto& o=static_cast<ServerNpcSharedParentSession*>(p)->original;return o.target_gate(o.context,e,a,out);
    }:nullptr;
    bound->path_refresh=original->path_refresh?+[](void* p,void* a,void* e) noexcept -> int{
        const auto& o=static_cast<ServerNpcSharedParentSession*>(p)->original;return o.path_refresh(o.context,a,e);
    }:nullptr;
    return 1;
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerNpcSharedTaskApi=_SiN_GetServerNpcSharedTaskApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcSharedTaskApi* __cdecl SiN_GetServerNpcSharedTaskApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcSharedTaskApi)?&api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcSharedFacingApi=_SiN_GetServerNpcSharedFacingApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcSharedFacingApi* __cdecl SiN_GetServerNpcSharedFacingApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcSharedFacingApi)?&facing_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcSharedTargetAnimationApi=_SiN_GetServerNpcSharedTargetAnimationApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcSharedTargetAnimationApi* __cdecl SiN_GetServerNpcSharedTargetAnimationApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcSharedTargetAnimationApi)?&target_animation_api:nullptr;
}
