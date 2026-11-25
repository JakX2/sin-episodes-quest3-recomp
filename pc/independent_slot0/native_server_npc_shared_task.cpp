#include "native_server_npc_shared_attack.h"
#include "server_fixed_difficulty_service.h"
#include <cmath>
#include <cstring>
#include <limits>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcConditionApi* __cdecl SiN_GetServerNpcConditionApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcActivityTranslateApi* __cdecl SiN_GetServerNpcActivityTranslateApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcTaskLifecycleApi* __cdecl SiN_GetServerNpcTaskLifecycleApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcTransitionApi* __cdecl SiN_GetServerNpcTransitionApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleResolutionApi* __cdecl SiN_GetServerNpcScheduleResolutionApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleNamespaceApi* __cdecl SiN_GetServerNpcScheduleNamespaceApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleStateApi* __cdecl SiN_GetServerNpcScheduleStateApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcSharedAttackApi* __cdecl SiN_GetServerNpcSharedAttackApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::fixed_service::Api* __cdecl SiN_GetFixedDifficultyApi(std::uint32_t,std::uint32_t) noexcept;
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
bool fixed_shoot_metric(float& metric) noexcept{
    using namespace sin_port::fixed_service;
    const auto* api=SiN_GetFixedDifficultyApi(abi_version,sizeof(Api));
    return api&&api->get_metric(11,&metric)&&std::isfinite(metric);
}
int __cdecl supports(int id,bool running) noexcept{
    if(id==138){
        float value=0;
        return running||fixed_shoot_metric(value);
    }
    if(running)return id==2||id==4||id==5||id==86||id==92||id==93||id==114||id==115||id==123||id==128;
    return (id>=1&&id<=7)||id==13||id==53||id==59||id==86||id==87||id==88||id==91||(id>=92&&id<=97)||id==113||id==114||id==115||id==119||id==120||id==123||id==128||id==131;
}
bool quantize_yaw(double value,float& out) noexcept{
    int fixed=0;
    if(!integer(std::trunc(value*182.0444488525390625),fixed))return false;
    out=static_cast<float>(static_cast<unsigned>(fixed)&0xffffu)*.0054931640625f;
    return true;
}
int __cdecl yaw_error(float actual,float desired,float* quantized,double* error) noexcept{
    if(!quantized||!error)return 0;*quantized=0;*error=0;
    if(!std::isfinite(actual)||!std::isfinite(desired))return 0;
    // Original x87 FMUL with the exact float32 constants, FISTP toward zero.
    if(!quantize_yaw(actual,*quantized))return 0;
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
bool facing_supports(int id,bool running) noexcept{
    if(id==60)return true;
    if(running)return id==56||id==57||id==58||id==61||id==63||id==64||id==65||id==112||id==117||id==118;
    return id==56||id==57||id==64||id==65||id==66||id==117||id==118;
}
int facing_alias(void* a,unsigned n,const ServerNpcScheduleTask& t,bool running,float now,const ServerNpcSharedTaskBindings& s,const ServerNpcSharedFacingBindings& f,ServerNpcSharedFacingResult& out) noexcept{
    auto complete=[&](){
        const auto* life=SiN_GetServerNpcTaskLifecycleApi(1,sizeof(ServerNpcTaskLifecycleApi));
        if(!life||!life->complete(a,n,now,false,&s.lifecycle,&out.shared.lifecycle))return 0;
        out.completed=out.shared.lifecycle.completed;return 1;
    };
    auto begin=[&](){if(!f.begin_turn||!f.begin_turn(f.context,a))return 0;out.begun=1;return 1;};
    auto resolve=[&](void*& motor){
        const auto token=rd<unsigned>(a,0x908);unsigned size=0;
        motor=nullptr;
        return token&&f.motor_storage&&f.motor_storage(f.context,token,&motor,&size)&&motor&&size>=0x18;
    };
    if(!running&&t.id==56)return begin(); // entry aliases the last half of Start57.
    void* motor=nullptr;if(!resolve(motor))return 0;
    if(running){
        if(!f.step_turn||!f.step_turn(f.context,motor,-1))return 0;
        out.stepped=1;
        if(!resolve(motor)||!yaw_error(rd<float>(a,0x370),rd<float>(motor,0x14),&out.current_quantized,&out.error))return 0;
        return std::fabs(out.error)>.006||complete();
    }
    float target=0;
    if(t.id==66){
        if(!quantize_yaw(rd<float>(a,0x370),target))return 0;
        wr(motor,0x14,target);out.yaw_written=1;return complete();
    }
    if(t.id==117||t.id==118){
        float current=0;
        if(!quantize_yaw(rd<float>(a,0x370),current)||!std::isfinite(t.parameter))return 0;
        const double offset=t.id==117?static_cast<double>(t.parameter):-static_cast<double>(t.parameter);
        if(!quantize_yaw(static_cast<double>(current)+offset,target))return 0;
    }else{
        if(t.id==64||t.id==65){
            const unsigned off=t.id==64?0x9f4:0xa00;
            float vector[3]{rd<float>(a,off),rd<float>(a,off+4),rd<float>(a,off+8)};
            if(!std::isfinite(vector[0])||!std::isfinite(vector[1])||!std::isfinite(vector[2])||
               !f.vector_yaw||!f.vector_yaw(f.context,a,vector,&target)||!std::isfinite(target))return 0;
            wr(motor,0x14,target);out.vector_written=1;
            if(!resolve(motor))return 0;
        }
        double desired=0;
        if(!f.desired_yaw||!f.desired_yaw(f.context,a,t.id!=57,&desired)||!std::isfinite(desired))return 0;
        target=static_cast<float>(desired);if(!std::isfinite(target))return 0;
    }
    wr(motor,0x14,target);out.yaw_written=1;return begin();
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
int execute_impl(void*,unsigned,const ServerNpcScheduleTask*,bool,float,const ServerNpcSharedTaskBindings*,ServerNpcSharedTaskResult*,const ServerNpcSharedFacingBindings* = nullptr,ServerNpcSharedFacingResult* = nullptr,const ServerNpcSharedTargetAnimationBindings* = nullptr,ServerNpcSharedTargetAnimationResult* = nullptr,const ServerNpcSharedMovementBindings* = nullptr,ServerNpcSharedMovementResult* = nullptr,const float* = nullptr,const ServerNpcSharedCueBindings* = nullptr,ServerNpcSharedCueResult* = nullptr,const ServerNpcSharedObservationBindings* = nullptr,ServerNpcSharedObservationResult* = nullptr,const ServerNpcSharedSoundCoverBindings* = nullptr,ServerNpcSharedSoundCoverResult* = nullptr,const ServerNpcSharedTacticalCoverBindings* = nullptr,ServerNpcSharedTacticalCoverResult* = nullptr) noexcept;
bool movement_supports(int id,bool running) noexcept{
    if(running)return id==116||id==121||id==122;
    return id==43||id==44||id==89||id==90||id==116||id==121||id==122;
}
int movement(void* a,unsigned n,const ServerNpcScheduleTask& t,bool running,float now,const ServerNpcSharedTaskBindings& s,const ServerNpcSharedMovementBindings& b,ServerNpcSharedMovementResult& out,ServerNpcSharedTaskResult& result) noexcept{
    if(!std::isfinite(t.parameter))return 0;
    const auto* life=SiN_GetServerNpcTaskLifecycleApi(1,sizeof(ServerNpcTaskLifecycleApi));
    if(!life)return 0;
    auto complete=[&](){return life->complete(a,n,now,false,&s.lifecycle,&result.lifecycle)!=0;};
    auto pending=[&](int id){
        result.activity=id;out.idle_activity=id;
        if(s.transition){
            const auto* api=SiN_GetServerNpcTransitionApi(1,sizeof(ServerNpcTransitionApi));
            return api&&api->set_pending(a,n,id,s.transition,&result.transition);
        }
        return s.pending&&s.pending(s.context,a,id);
    };
    auto storage=[&](unsigned token,unsigned minimum,void*& ptr){
        unsigned size=0;ptr=nullptr;return token&&b.storage&&b.storage(b.context,token,&ptr,&size)&&ptr&&size>=minimum;
    };
    auto navigator=[&](void*& nav){return storage(rd<unsigned>(a,0x8f8),0x28,nav);};
    auto path=[&](void*& nav,void*& p){return navigator(nav)&&storage(rd<unsigned>(nav,0x24),0x48,p);};
    auto clear=[&](){void* nav=nullptr;if(!navigator(nav)||!b.clear_path)return false;++out.clear_calls;return b.clear_path(b.context,nav)!=0;};
    auto stop=[&](bool finished){void* nav=nullptr;if(!navigator(nav)||!b.stop_path)return false;++out.stop_calls;return b.stop_path(b.context,nav,finished)!=0;};
    auto idle=[&](){
        void* nav=nullptr;void* p=nullptr;
        if(!path(nav,p))return false;
        const int id=rd<unsigned>(p,0)?rd<int>(p,0x24):1;
        return pending(id>0?id:1);
    };
    auto update=[&](){++out.move_calls;return b.movement_update&&b.movement_update(b.context,a);};
    if(!running&&(t.id==89||t.id==90)){
        int value=0;if(!integer(std::trunc(static_cast<double>(t.parameter)),value))return 0;
        void* nav=nullptr;void* p=nullptr;
        if(t.id==89){
            if(!path(nav,p))return 0;
            wr(p,4,static_cast<float>(value));
        }else{
            if(!storage(rd<unsigned>(a,0x8f8),0x50,nav))return 0;
            wr(nav,0x4c,static_cast<float>(value));
        }
        return complete();
    }
    if(!running&&(t.id==43||t.id==44)){
        if(!s.transition)return 0;
        const auto* api=SiN_GetServerNpcActivityTranslateApi(1,sizeof(ServerNpcActivityTranslateApi));
        if(!api)return 0;
        auto translated=[&](int id,int& selected){
            ServerNpcActivityTranslateResult tr{};
            if(!api->translate(a,n,id,&s.transition->assignment.translation,&tr))return false;
            selected=tr.activity;return true;
        };
        int selected=-1,activity=0;
        if(t.id==43){
            if(!translated(10,selected))return 0;
            activity=selected!=-1?10:6;
        }else{
            const int mode=rd<int>(a,0x204);
            if(mode==4||mode==5){if(!translated(25,selected))return 0;if(selected!=-1)activity=25;}
            if(!activity){if(!translated(6,selected))return 0;activity=selected!=-1?6:10;}
        }
        void* nav=nullptr;void* p=nullptr;if(!path(nav,p))return 0;
        wr(p,8,activity);result.activity=activity;
        wr(a,0x988,rd<unsigned>(a,0x988)&~2u);
        return complete();
    }
    if(!running&&t.id==122){
        void* nav=nullptr;void* p=nullptr;if(!path(nav,p))return 0;
        if(!rd<unsigned>(p,0))return complete();
        unsigned finished=0;if(!life->animation_finished(a,n,&finished))return 0;
        return finished?complete():update();
    }
    if(t.id==121||t.id==122){
        // Timeout is a strict > test; parameter zero bypasses elapsed-time query.
        if(running&&t.parameter!=0.f){
            const float started=rd<float>(a,0x7ac);
            if(!std::isfinite(started))return 0;
            if(static_cast<double>(now)-started>t.parameter){out.timed_out=1;return complete()&&stop(true);}
        }
        void* nav=nullptr;void* p=nullptr;if(!path(nav,p))return 0;
        if(!rd<unsigned>(p,0x44))return complete()&&(running?stop(true):clear());
        return rd<unsigned>(p,0)?update():idle();
    }
    if(!running){
        void* nav=nullptr;void* p=nullptr;if(!path(nav,p))return 0;
        out.navigator_mode=rd<int>(nav,0xc);
        if((rd<unsigned>(p,0x44)&&rd<unsigned>(p,0))||out.navigator_mode==1){
            if(t.parameter==1.f){if(!stop(false))return 0;}else if(!clear())return 0;
            int pose=-1;
            if(!b.pose_lookup||!b.pose_lookup(b.context,a,"move_yaw",&pose))return 0;
            if(pose>=0){++out.pose_calls;if(!b.pose_set||!b.pose_set(b.context,a,"move_yaw",0.f))return 0;}
            return 1;
        }
        if(t.parameter==1.f){
            bool yes=false;++out.try_calls;
            if(!b.try_stopping_path||!b.try_stopping_path(b.context,nav,&yes))return 0;
            if(yes)return 1;
        }
        return clear()&&idle()&&complete();
    }
    if(t.parameter==1.f){
        ServerNpcScheduleTask nested{121,0.f};ServerNpcSharedMovementResult nr{};++out.nested_calls;
        const int ok=execute_impl(a,n,&nested,true,now,&s,&nr.shared,nullptr,nullptr,nullptr,nullptr,&b,&nr);
        out.nested=nr.shared;out.stop_calls+=nr.stop_calls;out.move_calls+=nr.move_calls;
        return ok;
    }
    void* nav=nullptr;if(!navigator(nav))return 0;
    out.navigator_mode=rd<int>(nav,0xc);
    if(out.navigator_mode==1){
        if(rd<unsigned char>(a,0xbc)&1u)out.landed=1;
        else{
            float v[3]{};
            if(!b.velocity||!b.velocity(b.context,a,v))return 0;
            double squared=0;
            for(float x:v){if(!std::isfinite(x))return 0;squared+=static_cast<double>(x)*x;}
            const float rounded=static_cast<float>(squared);
            if(!std::isfinite(rounded))return 0;
            out.speed=std::sqrt(static_cast<double>(rounded));
            if(out.speed>.01)return 1;
            out.stuck=1;
        }
        // 22032220 -> 22074110 is a plain navigator mode write.
        wr(nav,0xc,0);out.mode_cleared=1;
        if(out.stuck){
            result.failed_code=26;
            if(!b.task_fail||!b.task_fail(b.context,a,26))return 0;
        }
    }
    // The original re-reads navigator mode after the virtual failure call.
    if(!navigator(nav))return 0;
    out.navigator_mode=rd<int>(nav,0xc);
    if(out.navigator_mode==3)return 1;
    return idle()&&complete();
}
bool grunt_cue_class(std::uint32_t v) noexcept{
    switch(v){case 0x2248df04:case 0x224954a4:case 0x2249aaa4:case 0x224a416c:
    case 0x2248c514:case 0x2248e83c:case 0x2249964c:case 0x224a4a4c:return true;default:return false;}
}
int shared_cue(void* a,unsigned n,const ServerNpcScheduleTask& t,float now,const ServerNpcSharedTaskBindings& s,const ServerNpcSharedCueBindings& b,ServerNpcSharedCueResult& out) noexcept{
    if(n<0xfcc||!grunt_cue_class(b.original_vtable)||!integer(std::trunc(static_cast<double>(t.parameter)),out.mode))return 0;
    const auto* life=SiN_GetServerNpcTaskLifecycleApi(1,sizeof(ServerNpcTaskLifecycleApi));if(!life)return 0;
    auto complete=[&](){return life->complete(a,n,now,false,&s.lifecycle,&out.shared.lifecycle)!=0;};
    auto emit=[&](const char* name){if(!b.cue||!b.cue(b.context,a,name,0,2))return false;out.emitted=1;return true;};
    if(out.mode==1){
        const auto nav=rd<unsigned>(a,0x8f8);
        if(!nav||!b.path_distance||!b.path_distance(b.context,nav,&out.path_distance)||!std::isfinite(out.path_distance))return 0;
        if(out.path_distance>240.f&&!emit("GRUNT_FLANK"))return 0;
        return complete();
    }
    if(out.mode!=0)return complete();
    if(!b.random||!b.random(b.context,0,5,&out.draw)||out.draw<0||out.draw>5)return 0;
    if(out.draw>1)return complete();
    void* target=nullptr;
    if(!b.target||!b.target(b.context,a,&target))return 0;
    if(!target)return complete();
    // 22031a80 re-queries +154 before target +fc.
    if(!b.target(b.context,a,&target)||!target||!b.as_npc)return 0;
    void* npc=nullptr;if(!b.as_npc(b.context,target,&npc))return 0;
    if(!npc)return complete();out.live=1;
    bool gate=false;
    const auto squad=rd<unsigned>(a,0x9c4);
    if(squad){if(!b.squad_gate||!b.squad_gate(b.context,squad,0,&gate))return 0;}
    else{
        const float until=rd<float>(a,0xc04);if(!std::isfinite(until))return 0;
        gate=now>until||0>rd<int>(a,0xc08);
    }
    if(gate&&(rd<unsigned>(a,0xb4)&2u)&&rd<int>(a,0x850)!=3)gate=false;
    out.gate=gate;if(!gate)return complete();
    float p[3]{},q[3]{};
    if(!b.center||!b.center(b.context,npc,p)||!b.center(b.context,a,q))return 0;
    double d[3]{};for(unsigned i=0;i<3;++i){if(!std::isfinite(p[i])||!std::isfinite(q[i]))return 0;d[i]=static_cast<double>(q[i])-p[i];}
    out.distance_squared=d[2]*d[2]+d[1]*d[1]+d[0]*d[0];
    out.within_range=out.distance_squared<=4000000.0;
    if(!out.within_range)return complete();
    if(!b.target_gate||!b.target_gate(b.context,npc,a,&gate))return 0;
    out.target_allowed=gate;if(!gate)return complete();
    if(!b.visible||!b.visible(b.context,a,npc,0x4081,&gate))return 0;
    out.visible=gate;if(gate&&!emit("GRUNT_ASSAULT"))return 0;
    return complete();
}
int observation(void* a,unsigned n,float now,const ServerNpcSharedTaskBindings& s,const ServerNpcSharedObservationBindings& b,ServerNpcSharedObservationResult& out) noexcept{
    const auto* life=SiN_GetServerNpcTaskLifecycleApi(1,sizeof(ServerNpcTaskLifecycleApi));if(!life)return 0;
    auto complete=[&](){return life->complete(a,n,now,false,&s.lifecycle,&out.shared.lifecycle)!=0;};
    bool gate=false,handled=false;
    // Original +45c = 2216e2d0 for all eight known Grunt classes.
    if(const auto token=rd<unsigned>(a,0xc18)){
        void* component=nullptr;unsigned size=0;
        if(!s.component_storage||!s.component_storage(s.context,token,&component,&size)||!component||size<9||!b.component_gate)return 0;
        const auto old=rd<std::uint8_t>(component,8);wr<std::uint8_t>(component,8,1);
        out.gate_component=1;
        const int ok=b.component_gate(b.context,token,&gate);
        handled=rd<std::uint8_t>(component,8)!=0;wr(component,8,old);
        out.gate_handled=handled;if(!ok)return 0;
    }
    if(!handled)gate=(rd<unsigned>(a,0xb4)&0x400u)!=0;
    out.flag_gate=gate;if(gate)return complete();
    if(!b.pvs_query)return 0;
    bool yes=false;out.pvs_called=1;
    if(!b.pvs_query(b.context,rd<unsigned>(a,0x16c),&yes))return 0;
    out.pvs=yes;if(yes)return complete();
    if(rd<int>(a,0x850)!=3)return 1;
    void* target=nullptr;if(!b.target)return 0;
    ++out.target_calls;if(!b.target(b.context,a,&target))return 0;
    if(!target)return 1;
    ++out.target_calls;if(!b.target(b.context,a,&target))return 0;
    // 2206b450 returns 0 for null, or -FLT_MAX on an absent record.
    if(target){
        const auto token=rd<unsigned>(a,0x984);
        if(!token||!b.memory_record)return 0;
        bool found=false;float stamp=0;out.memory_called=1;
        if(!b.memory_record(b.context,token,target,&found,&stamp))return 0;
        out.memory_found=found;
        out.last_seen=found?stamp:-std::numeric_limits<float>::max();
        if(!std::isfinite(out.last_seen))return 0;
    }
    out.elapsed=static_cast<double>(now)-out.last_seen;
    return out.elapsed>=15.0||complete();
}
int sound_cover97(void* a,unsigned n,const ServerNpcScheduleTask& task,float now,const ServerNpcSharedTaskBindings& s,const ServerNpcSharedSoundCoverBindings& b,ServerNpcSharedSoundCoverResult& out) noexcept{
    out.stage=rd<int>(a,0x7b4);
    if(out.stage!=0&&out.stage!=1)return 1;
    const auto nav=rd<unsigned>(a,0x8f8);
    auto best_sound=[&](const void*& sound,unsigned& size){
        sound=nullptr;size=0;++out.sound_queries;
        return b.best_sound&&b.best_sound(b.context,a,0xfff1f,&sound,&size)&&(!sound||size>=0x2c);
    };
    if(out.stage==0){
        const void* sound=nullptr;unsigned size=0;
        if(!best_sound(sound,size))return 0;
        bool found=false;
        if(sound){
            float position[3]{};std::memcpy(position,static_cast<unsigned char*>(a)+0xa0c,12);
            if(!b.cover_position||!b.cover_position(b.context,a,sound,position,&found))return 0;
            if(found){for(float v:position)if(!std::isfinite(v))return 0;}
            std::memcpy(static_cast<unsigned char*>(a)+0xa0c,position,12);
        }
        out.position_found=found;
        if(!found){
            const auto* life=SiN_GetServerNpcTaskLifecycleApi(1,sizeof(ServerNpcTaskLifecycleApi));
            out.shared.failed_code=8;return life&&life->fail(a,n,now,8,&s.lifecycle,&out.shared.lifecycle);
        }
        if(!nav||!b.prepare_navigation||!b.prepare_navigation(b.context,nav))return 0;
        out.prepared=1;
        // 22034b50: clear old cached sound and notify only if active, then re-query.
        const auto cache_token=rd<unsigned>(a,0x89c);void* cache=nullptr;unsigned extent=0;
        if(!cache_token||!b.storage||!b.storage(b.context,cache_token,&cache,&extent)||!cache||extent<0x2c)return 0;
        if(rd<unsigned>(cache,0xc)){
            wr(cache,0xc,0u);out.cache_cleared=1;
            if(!b.sound_cleared||!b.sound_cleared(b.context,a))return 0;
        }
        if(!best_sound(sound,size))return 0;
        if(sound){
            unsigned char snapshot[0x2c]{};std::memcpy(snapshot,sound,sizeof(snapshot));
            // Retail 2202f950 copies fields, leaving padding byte +1b untouched.
            std::memcpy(cache,snapshot,0x1b);
            std::memcpy(static_cast<unsigned char*>(cache)+0x1c,snapshot+0x1c,0x10);
            out.cache_copied=1;
        }
        wr(a,0x7b4,1);out.advanced=1;return 1;
    }
    if(!nav||!b.goal_context||!b.submit_goal)return 0;
    auto& goal=out.goal;goal.type=4;std::memcpy(goal.position,static_cast<unsigned char*>(a)+0xa0c,12);
    for(float v:goal.position)if(!std::isfinite(v))return 0;
    goal.target=goal.aux1=goal.aux2=-1;goal.activity=10;goal.tolerance=-2.f;goal.range=-1.f;
    if(!b.goal_context(b.context,&goal.context_token))return 0;
    const void* sound=nullptr;unsigned size=0;if(!best_sound(sound,size))return 0;
    if(sound)goal.range=static_cast<float>(static_cast<double>(rd<int>(sound,4))*.5);
    bool accepted=false;out.submitted=1;
    if(!b.submit_goal(b.context,nav,&goal,&accepted))return 0;
    out.accepted=accepted;if(!accepted)return 1;
    const float deadline=static_cast<float>(static_cast<double>(now)+task.parameter);
    if(!std::isfinite(deadline))return 0;
    wr(a,0x8f0,deadline);out.deadline_written=1;return 1;
}
bool tactical_cover_supports(int id) noexcept{return id==98||id==101||id==102||id==103;}
int tactical_cover(void* a,unsigned n,const ServerNpcScheduleTask& t,float now,const ServerNpcSharedTaskBindings& s,const ServerNpcSharedTacticalCoverBindings& b,ServerNpcSharedTacticalCoverResult& out) noexcept{
    if(b.count>sin_independent::kEntityHandleCapacity||(b.count&&!b.entries))return 0;
    const auto registry=sin_independent::EntityHandleView(b.entries,b.count);
    auto resolve=[&](unsigned off){return static_cast<void*>(registry.Resolve(rd<unsigned>(a,off)));};
    const auto* life=SiN_GetServerNpcTaskLifecycleApi(1,sizeof(ServerNpcTaskLifecycleApi));if(!life)return 0;
    auto fail=[&](){out.shared.failed_code=8;return life->fail(a,n,now,8,&s.lifecycle,&out.shared.lifecycle);};
    auto extent=[&](void* p,unsigned minimum){unsigned size=0;return p&&b.extent&&b.extent(b.context,p,&size)&&size>=minimum;};
    auto storage=[&](unsigned token,unsigned minimum,void*& p){unsigned size=0;p=nullptr;return token&&b.storage&&b.storage(b.context,token,&p,&size)&&p&&size>=minimum;};
    void* target=resolve(0x8a0);if(!target)target=a;
    // 2204f5d0(0): release owned hint, clear the actor's handle only when live.
    if(void* hint=resolve(0xa18)){
        if(!extent(hint,0x3d8))return 0;
        if(registry.Resolve(rd<unsigned>(hint,0x3d0))==a){
            wr(hint,0x3d0,0xffffffffu);wr(hint,0x3d4,now);out.hint_released=1;
        }
        wr(a,0xa18,0xffffffffu);out.hint_cleared=1;
    }
    out.minimum=t.id==103?t.parameter:0.f;
    out.maximum=t.id==102?t.parameter:std::numeric_limits<float>::max();
    bool found=false;float position[3]{std::numeric_limits<float>::max(),std::numeric_limits<float>::max(),std::numeric_limits<float>::max()};
    if(t.id==98){
        out.special_called=1;
        if(!b.special_cover||!b.special_cover(b.context,a,target,position,&found))return 0;
    }else{
        if(out.maximum==std::numeric_limits<float>::max()){
            if(!b.default_range||!b.default_range(b.context,a,&out.maximum))return 0;
        }
        if(!std::isfinite(out.minimum)||!std::isfinite(out.maximum)||!b.absolute_origin||!b.viewpoint||!b.search)return 0;
        float origin[3]{},eye[3]{};
        // Original ensures dirty absolute transform before the viewpoint query.
        // The search sees the post-viewpoint origin cache.
        if(!b.absolute_origin(b.context,target,origin)||!b.viewpoint(b.context,target,eye))return 0;
        for(unsigned i=0;i<3;++i)if(!std::isfinite(origin[i])||!std::isfinite(eye[i]))return 0;
        const auto tactical_token=rd<unsigned>(a,0x9d8);if(!tactical_token)return 0;
        out.search_called=1;
        if(!b.search(b.context,tactical_token,origin,eye,out.minimum,out.maximum,position,&found))return 0;
    }
    out.position_found=found;if(!found)return fail();
    for(float v:position)if(!std::isfinite(v))return 0;
    const auto nav=rd<unsigned>(a,0x8f8);if(!nav||!b.goal_context||!b.submit_goal)return 0;
    auto& g=out.goal;g.type=7;std::memcpy(g.position,position,12);g.target=g.aux1=g.aux2=-1;g.activity=10;g.tolerance=-2.f;g.range=-1.f;
    if(!b.goal_context(b.context,&g.context_token))return 0;
    bool accepted=false;out.submitted=1;
    if(!b.submit_goal(b.context,nav,&g,&accepted))return 0;
    out.accepted=accepted;if(!accepted)return fail();
    // A new live hint selected by the cover service applies activity and direction.
    if(void* hint=resolve(0xa18)){
        void* navigator=nullptr;void* path=nullptr;int activity=0;float direction[3]{};
        if(!storage(nav,0x28,navigator)||!storage(rd<unsigned>(navigator,0x24),0x60,path)||
           !b.hint_activity||!b.hint_activity(b.context,a,hint,&activity))return 0;
        wr(path,0x24,activity);wr(path,0x28,0xffffffffu);
        if(!b.hint_direction||!b.hint_direction(b.context,hint,direction))return 0;
        for(float v:direction)if(!std::isfinite(v))return 0;
        wr(path,0x5c,0xffffffffu);std::memcpy(static_cast<unsigned char*>(path)+0x50,direction,12);out.hint_applied=1;
    }
    if(t.id==98){
        const float stamp=static_cast<float>(static_cast<double>(now)+t.parameter);if(!std::isfinite(stamp))return 0;
        wr(a,0x8f0,stamp);out.deadline_written=1;
    }
    return life->complete(a,n,now,false,&s.lifecycle,&out.shared.lifecycle);
}
int execute_impl(void* a,unsigned n,const ServerNpcScheduleTask* t,bool running,float now,const ServerNpcSharedTaskBindings* b,ServerNpcSharedTaskResult* out,const ServerNpcSharedFacingBindings* facing,ServerNpcSharedFacingResult* facing_out,const ServerNpcSharedTargetAnimationBindings* target,ServerNpcSharedTargetAnimationResult* target_out,const ServerNpcSharedMovementBindings* move,ServerNpcSharedMovementResult* move_out,const float* shoot_metric,const ServerNpcSharedCueBindings* cue,ServerNpcSharedCueResult* cue_out,const ServerNpcSharedObservationBindings* observe,ServerNpcSharedObservationResult* observe_out,const ServerNpcSharedSoundCoverBindings* sound_cover,ServerNpcSharedSoundCoverResult* sound_cover_out,const ServerNpcSharedTacticalCoverBindings* tactical,ServerNpcSharedTacticalCoverResult* tactical_out) noexcept{
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
    if(!running&&tactical_cover_supports(t->id)&&tactical&&tactical_out){out->native=1;return tactical_cover(a,n,*t,now,*b,*tactical,*tactical_out);}
    if(running&&t->id==97&&sound_cover&&sound_cover_out){out->native=1;return sound_cover97(a,n,*t,now,*b,*sound_cover,*sound_cover_out);}
    if(running&&t->id==6&&observe&&observe_out){out->native=1;return observation(a,n,now,*b,*observe,*observe_out);}
    if(!running&&t->id==84&&cue&&cue_out){out->native=1;return shared_cue(a,n,*t,now,*b,*cue,*cue_out);}
    if(movement_supports(t->id,running)&&move&&move_out){out->native=1;return movement(a,n,*t,running,now,*b,*move,*move_out,*out);}
    if(facing_supports(t->id,running)&&facing&&facing_out){out->native=1;return t->id==60?face60(a,n,running,now,*b,*facing,*facing_out):facing_alias(a,n,*t,running,now,*b,*facing,*facing_out);}
    if(running&&t->id>=94&&t->id<=96&&target&&target_out){out->native=1;return target_animation(a,n,t->id,now,*b,*target,*target_out);}
    if(t->id==138&&!running){
        float metric=0;
        if(shoot_metric||fixed_shoot_metric(metric)){
            if(shoot_metric)metric=*shoot_metric;
            if(!std::isfinite(metric)||!std::isfinite(t->parameter))return 0;
            // Original +1ac returns x87 metric11+argument, caller rounds f32.
            const float adjusted=static_cast<float>(static_cast<double>(metric)+t->parameter);
            if(!wait(a,n,now,0.f,adjusted,b,&out->ticks))return 0;
            out->native=out->timer_written=1;return 1;
        }
    }
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
    if(t->id==123){
        const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));ServerNpcConditionResult result{};
        if(!cond||!cond->test(a,n,0x30,NpcConditionCurrent,&b->lifecycle.conditions,&result))return 0;
        return result.value||complete();
    }
    if(running){
        if(t->id==115)return 1;
        if(t->id==128){unsigned yes=0;if(!life->animation_finished(a,n,&yes))return 0;return !yes||complete();}
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
    if(t->id==53){wr(a,0x8f0,now);return complete();}
    if(t->id==6||t->id==13||t->id==97||t->id==113||t->id==115)return 1;
    if(t->id==128)return pending(70);
    if(t->id==131){if(!wait(a,n,now,4.f,0.f,b,&out->ticks))return 0;out->timer_written=1;return 1;}
    if(t->id==91){wr(a,0x7bc,0);return complete();}
    if(t->id==2||t->id==4||t->id==5||t->id==59||t->id==114){
        const bool random=t->id==5||t->id==114;
        if(!wait(a,n,now,random?0.f:t->parameter,random?t->parameter:0.f,b,&out->ticks))return 0;
        out->timer_written=1;return 1;
    }
    int value=0;if(!integer(std::trunc(static_cast<double>(t->parameter)),value))return 0;
    switch(t->id){
    case 7:if(rd<int>(a,0x858)!=value)wr(a,0x858,value);return complete();
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
    if(!t||!facing_supports(t->id,running)||!f)return 0;
    return execute_impl(a,n,t,running,now,b,&out->shared,f,out);
}
int __cdecl execute_target_animation(void* a,unsigned n,const ServerNpcScheduleTask* t,float now,const ServerNpcSharedTaskBindings* b,const ServerNpcSharedTargetAnimationBindings* target,ServerNpcSharedTargetAnimationResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!t||t->id<94||t->id>96||!target)return 0;
    return execute_impl(a,n,t,true,now,b,&out->shared,nullptr,nullptr,target,out);
}
int __cdecl execute_movement(void* a,unsigned n,const ServerNpcScheduleTask* t,bool running,float now,const ServerNpcSharedTaskBindings* s,const ServerNpcSharedMovementBindings* b,ServerNpcSharedMovementResult* out) noexcept{
    if(!out)return 0;*out={};out->idle_activity=-1;
    if(!t||!movement_supports(t->id,running)||!b)return 0;
    return execute_impl(a,n,t,running,now,s,&out->shared,nullptr,nullptr,nullptr,nullptr,b,out);
}
int __cdecl execute_difficulty_wait(void* a,unsigned n,const ServerNpcScheduleTask* t,bool running,float now,float metric,const ServerNpcSharedTaskBindings* b,ServerNpcSharedTaskResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!t||t->id!=138)return 0;
    return execute_impl(a,n,t,running,now,b,out,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,&metric);
}
int __cdecl execute_cue(void* a,unsigned n,const ServerNpcScheduleTask* t,float now,const ServerNpcSharedTaskBindings* b,const ServerNpcSharedCueBindings* cue,ServerNpcSharedCueResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!t||t->id!=84||!cue||!grunt_cue_class(cue->original_vtable)||n<0xfcc)return 0;
    return execute_impl(a,n,t,false,now,b,&out->shared,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,cue,out);
}
int __cdecl execute_observation(void* a,unsigned n,const ServerNpcScheduleTask* t,float now,const ServerNpcSharedTaskBindings* b,const ServerNpcSharedObservationBindings* observe,ServerNpcSharedObservationResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!t||t->id!=6||!observe||!grunt_cue_class(observe->original_vtable))return 0;
    return execute_impl(a,n,t,true,now,b,&out->shared,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,observe,out);
}
int __cdecl execute_sound_cover(void* a,unsigned n,const ServerNpcScheduleTask* t,float now,const ServerNpcSharedTaskBindings* b,const ServerNpcSharedSoundCoverBindings* sound,ServerNpcSharedSoundCoverResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!t||t->id!=97||!sound||!grunt_cue_class(sound->original_vtable))return 0;
    return execute_impl(a,n,t,true,now,b,&out->shared,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,sound,out);
}
int __cdecl execute_tactical_cover(void* a,unsigned n,const ServerNpcScheduleTask* t,float now,const ServerNpcSharedTaskBindings* b,const ServerNpcSharedTacticalCoverBindings* tactical,ServerNpcSharedTacticalCoverResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!t||!tactical_cover_supports(t->id)||!tactical||!grunt_cue_class(tactical->original_vtable))return 0;
    return execute_impl(a,n,t,false,now,b,&out->shared,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,tactical,out);
}
const ServerNpcSharedTacticalCoverApi tactical_cover_api{1,sizeof(ServerNpcSharedTacticalCoverApi),&execute_tactical_cover};
const ServerNpcSharedSoundCoverApi sound_cover_api{1,sizeof(ServerNpcSharedSoundCoverApi),&execute_sound_cover};
const ServerNpcSharedObservationApi observation_api{1,sizeof(ServerNpcSharedObservationApi),&execute_observation};
const ServerNpcSharedCueApi cue_api{1,sizeof(ServerNpcSharedCueApi),&execute_cue};
const ServerNpcDifficultyWaitApi difficulty_wait_api{1,sizeof(ServerNpcDifficultyWaitApi),&execute_difficulty_wait};
const ServerNpcSharedMovementApi movement_api{1,sizeof(ServerNpcSharedMovementApi),&execute_movement};
const ServerNpcSharedTargetAnimationApi target_animation_api{1,sizeof(ServerNpcSharedTargetAnimationApi),&execute_target_animation};
const ServerNpcSharedFacingApi facing_api{1,sizeof(ServerNpcSharedFacingApi),&yaw_error,&execute_facing};
const ServerNpcSharedTaskApi api{1,sizeof(ServerNpcSharedTaskApi),&wait,&cancel_wait,&wait_state,&supports,&execute};
 }
int bind_shared_parent_internal(void* actor,unsigned bytes,float now,const ServerGruntTaskBindings* original,const ServerNpcScheduleResolutionBindings* resolution,const ServerNpcSharedTaskBindings* shared,ServerNpcSharedTaskStats* stats,ServerNpcSharedParentSession* session,ServerGruntTaskBindings* bound,const ServerNpcSharedAttackBindings* attack,ServerNpcSharedAttackStats* attack_stats,const ServerNpcSharedFacingBindings* facing,ServerNpcSharedFacingStats* facing_stats,const ServerNpcSharedTargetAnimationBindings* target_animation,ServerNpcSharedTargetAnimationStats* target_animation_stats,const ServerNpcSharedMovementBindings* movement,ServerNpcSharedMovementStats* movement_stats,const ServerNpcSharedCueBindings* cue,ServerNpcSharedCueStats* cue_stats,const ServerNpcSharedObservationBindings* observe,ServerNpcSharedObservationStats* observe_stats,const ServerNpcSharedSoundCoverBindings* sound_cover,ServerNpcSharedSoundCoverStats* sound_cover_stats,const ServerNpcSharedTacticalCoverBindings* tactical,ServerNpcSharedTacticalCoverStats* tactical_stats) noexcept{
    if(!actor||!original||!resolution||!shared||!stats||!session||!bound)return 0;
    if((attack&&!attack_stats)||(facing&&!facing_stats)||(target_animation&&!target_animation_stats)||(movement&&!movement_stats)||(cue&&!cue_stats)||(observe&&!observe_stats)||(sound_cover&&!sound_cover_stats)||(tactical&&!tactical_stats))return 0;
    *stats={};if(attack_stats)*attack_stats={};if(facing_stats)*facing_stats={};if(target_animation_stats)*target_animation_stats={};if(movement_stats)*movement_stats={};if(cue_stats)*cue_stats={};if(observe_stats)*observe_stats={};if(sound_cover_stats)*sound_cover_stats={};if(tactical_stats)*tactical_stats={};
    *session={actor,bytes,now,*original,*shared,stats,attack,attack_stats,facing,facing_stats,target_animation,target_animation_stats,movement,movement_stats,cue,cue_stats,observe,observe_stats,sound_cover,sound_cover_stats,tactical,tactical_stats};*bound=*original;
    session->shared.lifecycle=original->lifecycle;
    session->shared.transition=original->transition;
    session->shared.resolution=resolution;
    bound->context=session;
    bound->parent=[](void* p,void* a,const ServerNpcScheduleTask* t,bool running) noexcept -> int{
        auto& q=*static_cast<ServerNpcSharedParentSession*>(p);
        if(a!=q.actor||!t)return 0;
        if(q.tactical&&!running&&tactical_cover_supports(t->id)){
            auto binding=*q.tactical;binding.entries=q.original.entities.entries;binding.count=q.original.entities.count;
            auto& st=*q.tactical_stats;++st.calls;
            const int ok=tactical_cover_api.execute(a,q.bytes,t,q.now,&q.shared,&binding,&st.last);
            st.native+=st.last.shared.native;st.components+=st.last.shared.component_handled;return ok;
        }
        if(q.sound_cover&&running&&t->id==97){
            auto& st=*q.sound_cover_stats;++st.calls;
            const int ok=sound_cover_api.execute(a,q.bytes,t,q.now,&q.shared,q.sound_cover,&st.last);
            st.native+=st.last.shared.native;st.components+=st.last.shared.component_handled;return ok;
        }
        if(q.observe&&running&&t->id==6){
            auto& st=*q.observe_stats;++st.calls;
            const int ok=observation_api.execute(a,q.bytes,t,q.now,&q.shared,q.observe,&st.last);
            st.native+=st.last.shared.native;st.components+=st.last.shared.component_handled;return ok;
        }
        if(q.cue&&!running&&t->id==84){
            auto& st=*q.cue_stats;++st.calls;
            const int ok=cue_api.execute(a,q.bytes,t,q.now,&q.shared,q.cue,&st.last);
            st.native+=st.last.shared.native;st.components+=st.last.shared.component_handled;return ok;
        }
        if(q.movement&&movement_supports(t->id,running)&&((t->id!=43&&t->id!=44)||q.shared.transition)){
            auto& st=*q.movement_stats;++st.calls;
            const int ok=movement_api.execute(a,q.bytes,t,running,q.now,&q.shared,q.movement,&st.last);
            st.native+=st.last.shared.native;st.components+=st.last.shared.component_handled;return ok;
        }
        if(q.target_animation&&running&&t->id>=94&&t->id<=96){
            auto binding=*q.target_animation;binding.entries=q.original.entities.entries;binding.count=q.original.entities.count;
            auto& st=*q.target_animation_stats;++st.calls;
            const int ok=target_animation_api.execute(a,q.bytes,t,q.now,&q.shared,&binding,&st.last);
            st.native+=st.last.shared.native;st.components+=st.last.shared.component_handled;return ok;
        }
        if(q.facing&&facing_supports(t->id,running)){
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

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcSharedMovementApi=_SiN_GetServerNpcSharedMovementApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcSharedMovementApi* __cdecl SiN_GetServerNpcSharedMovementApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcSharedMovementApi)?&movement_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcDifficultyWaitApi=_SiN_GetServerNpcDifficultyWaitApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcDifficultyWaitApi* __cdecl SiN_GetServerNpcDifficultyWaitApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcDifficultyWaitApi)?&difficulty_wait_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcSharedCueApi=_SiN_GetServerNpcSharedCueApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcSharedCueApi* __cdecl SiN_GetServerNpcSharedCueApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcSharedCueApi)?&cue_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcSharedObservationApi=_SiN_GetServerNpcSharedObservationApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcSharedObservationApi* __cdecl SiN_GetServerNpcSharedObservationApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcSharedObservationApi)?&observation_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcSharedSoundCoverApi=_SiN_GetServerNpcSharedSoundCoverApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcSharedSoundCoverApi* __cdecl SiN_GetServerNpcSharedSoundCoverApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcSharedSoundCoverApi)?&sound_cover_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcSharedTacticalCoverApi=_SiN_GetServerNpcSharedTacticalCoverApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcSharedTacticalCoverApi* __cdecl SiN_GetServerNpcSharedTacticalCoverApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcSharedTacticalCoverApi)?&tactical_cover_api:nullptr;
}
