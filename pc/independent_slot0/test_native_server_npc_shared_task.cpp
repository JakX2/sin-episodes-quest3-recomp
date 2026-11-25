#include "native_server_npc_shared_task.h"
#include "server_fixed_difficulty_service.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <cstdio>
#include <vector>
using namespace sin_port::native_weapon_tables;
struct F {
    unsigned char a[0xc34]{},desc[0x48]{},component[9]{},row[52]{},task[8]{};
    unsigned char motor[0x1c]{},target[4]{},nav[0x50]{},path[0x48]{};
    float speed[3]{};bool can_stop=false;int pose_index=3;
    bool target_present=true;float target_pos[3]{};
    bool component_handles=false,aimed=false,step_align=false;int draw=2;double desired=0;std::vector<int> trace;
    float motor_yaw() const {float y=0;std::memcpy(&y,motor+0x14,4);return y;}
    template<class T>void put(void* p,unsigned o,T v){std::memcpy(static_cast<unsigned char*>(p)+o,&v,sizeof(v));}
    template<class T>T get(unsigned o){T v{};std::memcpy(&v,a+o,sizeof(v));return v;}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto c=LoadLibraryA(argv[1]),s=LoadLibraryA(argv[2]);assert(c&&s);
    auto g=reinterpret_cast<GetServerNpcSharedTaskApi>(GetProcAddress(s,server_npc_shared_task_export));
    assert(g&&!GetProcAddress(c,server_npc_shared_task_export)&&!g(0,sizeof(ServerNpcSharedTaskApi))&&!g(1,0));
    const auto* api=g(1,sizeof(ServerNpcSharedTaskApi));assert(api);
    F f;const int global=npc_global_condition_base;
    f.put(f.desc,4,global+100);f.put(f.desc,8,0);f.put(f.desc,12,127);f.put(f.desc,16,global+227);
    f.put(f.desc,0x34,global);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    f.put(f.row,0,global+125);f.put(f.row,4,2u);f.put(f.row,8,1);f.put(f.task,0,2);
    ServerNpcSharedTaskBindings b{};b.context=&f;b.lifecycle.conditions.context=&f;
    b.lifecycle.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    b.random=[](void* p,int lo,int hi,int* out) noexcept -> int{auto& f=F::self(p);assert(lo==1&&hi==3);*out=f.draw;f.trace.push_back(1);return 1;};
    b.component_storage=[](void* p,std::uint32_t tok,void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);assert(tok==77);*out=f.component;*n=sizeof(f.component);return 1;};
    b.component_task=[](void* p,std::uint32_t,const ServerNpcScheduleTask*,bool) noexcept -> int{auto& f=F::self(p);assert(f.component[8]==1);f.component[8]=f.component_handles;f.trace.push_back(2);return 1;};
    b.special67=[](void* p,void*,const ServerNpcScheduleTask*,bool) noexcept -> int{F::self(p).trace.push_back(3);return 1;};
    b.parent=[](void* p,void*,const ServerNpcScheduleTask*,bool) noexcept -> int{F::self(p).trace.push_back(4);return 1;};
    b.pending=[](void* p,void*,int id) noexcept -> int{F::self(p).trace.push_back(id);return 1;};
    b.aim_query=[](void* p,void*,bool* out) noexcept -> int{auto& f=F::self(p);*out=f.aimed;f.trace.push_back(5);return 1;};
    b.aim_update=[](void* p,void*) noexcept -> int{F::self(p).trace.push_back(6);return 1;};
    b.turn=[](void* p,void*) noexcept -> int{F::self(p).trace.push_back(7);return 1;};
    ServerNpcScheduleResolutionBindings re{};re.context=&f;re.head=1;re.grunt.schedule.context=&f;re.grunt.schedule.conditions=b.lifecycle.conditions;
    re.translate=[](void*,void*,int id,int* out) noexcept -> int{*out=id;return 1;};
    re.grunt.schedule.storage=[](void* p,std::uint32_t tok,const void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);if(tok==1){*out=f.row;*n=52;return 1;}if(tok==2){*out=f.task;*n=8;return 1;}return 0;};
    re.grunt.schedule.reset_movement=[](void*,void*,std::uint32_t) noexcept -> int{return 1;};b.resolution=&re;
    int tick=0;unsigned active=0,expired=0;
    assert(api->wait(f.a,sizeof(f.a),10,.13f,0,&b,&tick)&&tick==2&&f.get<float>(0x9dc)==10.2f);
    assert(api->wait_state(f.a,sizeof(f.a),10.19f,&active,&expired)&&active&&!expired);
    assert(api->wait_state(f.a,sizeof(f.a),10.2f,&active,&expired)&&expired);
    assert(api->cancel_wait(f.a,sizeof(f.a))&&api->wait_state(f.a,sizeof(f.a),10,&active,&expired)&&!active&&!expired);
    assert(api->wait(f.a,sizeof(f.a),10,-.11f,0,&b,&tick)&&tick==-1&&f.get<float>(0x9dc)==9.9f);
    assert(api->wait(f.a,sizeof(f.a),10,0,.3f,&b,&tick)&&tick==2);
    ServerNpcSharedTaskResult o{};
    auto run=[&](int id,bool running,float parameter,float now){
        f.put(f.a,0x7a4,1);f.trace.clear();ServerNpcScheduleTask task{id,parameter};
        const int ok=api->execute(f.a,sizeof(f.a),&task,running,now,&b,&o);
        if(!ok)std::fprintf(stderr,"Shared task %d (%s) failed at %.3f\n",id,running?"run":"start",now);
        assert(ok);};
    run(2,false,.13f,10);assert(o.native&&o.timer_written&&o.ticks==2);run(2,true,0,10.19f);assert(!o.lifecycle.completed);run(2,true,0,10.2f);assert(o.lifecycle.completed);
    run(5,false,.3f,10);assert(o.ticks==2&&f.trace==std::vector<int>{1});run(5,true,0,10.2f);assert(o.lifecycle.completed&&(f.trace==std::vector<int>{5,6}));
    f.aimed=true;run(4,true,0,11);assert(o.lifecycle.completed&&f.trace==std::vector<int>{5});
    run(88,false,49.9f,11);assert(f.get<int>(0x7bc)==49&&o.lifecycle.completed);
    f.put(f.a,0x988,4u);run(119,false,2,11);assert(f.get<unsigned>(0x988)==6&&o.lifecycle.completed);
    run(86,false,45,11);assert(o.activity==45&&f.trace==std::vector<int>{45});
    f.put(f.a,0x878,45);run(86,false,0,11);assert(!f.get<int>(0x878)&&f.trace.empty());
    f.put(f.a,0x3ec,3);f.put(f.a,0x884,3);run(86,true,0,11);assert(o.lifecycle.completed);
    run(92,false,45,11);assert(o.activity==45);run(92,true,0,11);assert(!o.lifecycle.completed&&f.trace==std::vector<int>{7});
    f.a[0x3e0]=1;run(92,true,0,11);assert(o.lifecycle.completed);
    run(95,false,54,11);assert(o.activity==54);
    // Retail immediate state handlers do not read the task parameter.
    f.put(f.a,0x878,45);f.put(f.a,0x7bc,49);f.put(f.a,0x988,0xf3u);
    run(1,false,123,11);assert(o.native&&o.lifecycle.completed&&!f.get<int>(0x878)&&f.get<int>(0x7bc)==49);
    run(3,false,2,11);assert(o.native&&o.lifecycle.completed&&f.get<int>(0x7bc)==49);
    run(91,false,999,11);assert(o.native&&o.lifecycle.completed&&!f.get<int>(0x7bc));
    run(120,false,3.9f,11);assert(o.native&&o.lifecycle.completed&&f.get<unsigned>(0x988)==0xf0u);
    run(120,false,0,11);assert(f.get<unsigned>(0x988)==0xf0u);
    run(120,false,-1,11);assert(!f.get<unsigned>(0x988));
    run(59,false,.13f,11);assert(o.native&&o.timer_written&&o.ticks==2&&f.get<float>(0x9dc)==11.2f);
    run(59,true,0,11);assert(o.parent_called&&!o.native&&f.trace==std::vector<int>{4});
    for(int id=93;id<=96;++id){run(id,false,54.9f,11);assert(o.native&&o.activity==54&&f.trace==std::vector<int>{54});}
    f.a[0x3e0]=0;run(93,true,0,11);assert(o.native&&o.turn_called&&!o.lifecycle.completed&&f.trace==std::vector<int>{7});
    f.a[0x3e0]=1;run(93,true,0,11);assert(o.lifecycle.completed&&f.trace==std::vector<int>{7});
    for(int id=94;id<=96;++id){run(id,true,0,11);assert(o.parent_called&&!o.native&&f.trace==std::vector<int>{4});}
    assert(api->supports(3,false)&&!api->supports(3,true)&&api->supports(93,true));
    ServerNpcScheduleTask small{88,50};assert(api->execute(f.a,0xc1c,&small,false,11,&b,&o)&&o.lifecycle.completed);
    ServerNpcScheduleTask replace{87,25};assert(!api->execute(f.a,0xc1c,&replace,false,11,&b,&o)&&!o.schedule_set);
    run(87,false,25,11);assert(o.schedule_set&&f.get<int>(0x79c)==global+125&&f.get<unsigned>(0x798)==1&&o.lifecycle.completed);
    f.put(f.a,0xc18,77u);f.component_handles=true;run(88,false,12,11);assert(o.component_handled&&!o.native&&f.trace==std::vector<int>{2});
    f.component_handles=false;run(88,false,12,11);assert(o.component_called&&!o.component_handled&&o.native&&f.get<int>(0x7bc)==12);
    run(67,true,0,11);assert(o.special_called&&!o.component_called&&f.trace==std::vector<int>{3});
    run(116,false,0,11);assert(o.parent_called&&(f.trace==std::vector<int>{2,4}));
    auto gf=reinterpret_cast<GetServerNpcSharedFacingApi>(GetProcAddress(s,server_npc_shared_facing_export));
    assert(gf&&!GetProcAddress(c,server_npc_shared_facing_export)&&!gf(0,sizeof(ServerNpcSharedFacingApi))&&!gf(1,0));
    const auto* face=gf(1,sizeof(ServerNpcSharedFacingApi));assert(face);
    float q=0;double err=0;
    assert(face->yaw_error(0,180,&q,&err)&&q==0&&err==-180);
    assert(face->yaw_error(180,0,&q,&err)&&q==180&&err==180);
    assert(face->yaw_error(-90,270,&q,&err)&&q==270&&err==0);
    assert(face->yaw_error(0,.0059f,&q,&err)&&err<.006);
    ServerNpcSharedFacingBindings fb{};fb.context=&f;
    fb.motor_storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{if(token!=88)return 0;auto& f=F::self(p);*out=f.motor;*n=sizeof(f.motor);return 1;};
    fb.aim_vector=[](void* p,void*,float* v) noexcept -> int{F::self(p).trace.push_back(10);v[0]=1;v[1]=2;v[2]=3;return 1;};
    fb.aligned=[](void* p,void*,const float* v,bool* out) noexcept -> int{auto& f=F::self(p);assert(v[0]==1&&v[2]==3);f.trace.push_back(11);*out=f.aimed;return 1;};
    fb.vector_yaw=[](void* p,void*,const float*,float* out) noexcept -> int{F::self(p).trace.push_back(12);*out=30;return 1;};
    fb.desired_yaw=[](void* p,void*,bool mode,double* out) noexcept -> int{auto& f=F::self(p);assert(mode);if(!f.aimed)assert(f.motor_yaw()==30);f.trace.push_back(13);*out=f.desired;return 1;};
    fb.begin_turn=[](void* p,void*) noexcept -> int{F::self(p).trace.push_back(14);return 1;};
    fb.step_turn=[](void* p,void* motor,int mode) noexcept -> int{auto& f=F::self(p);assert(motor==f.motor&&mode==-1);f.trace.push_back(15);if(f.step_align)f.put(f.a,0x370,f.motor_yaw());return 1;};
    f.put(f.a,0x908,88u);f.put(f.a,0xc18,0u);f.put(f.a,0x370,0.f);
    ServerNpcSharedFacingResult fr{};ServerNpcScheduleTask ft{60,0};
    auto facing=[&](bool running){f.put(f.a,0x7a4,1);f.trace.clear();assert(face->execute(f.a,sizeof(f.a),&ft,running,12,&b,&fb,&fr));};
    f.aimed=false;f.desired=45;facing(false);
    assert(fr.shared.native&&fr.vector_written&&fr.yaw_written&&fr.begun&&!fr.completed&&f.motor_yaw()==45&&(f.trace==std::vector<int>{10,11,12,13,14}));
    f.aimed=true;f.desired=45.5;facing(false);
    assert(fr.completed&&!fr.yaw_written&&!fr.begun&&f.motor_yaw()==45&&(f.trace==std::vector<int>{10,11,13}));
    f.desired=46;facing(false);assert(fr.yaw_written&&fr.begun&&!fr.completed&&f.motor_yaw()==46);
    f.desired=47;facing(true);assert(!fr.yaw_written&&fr.stepped&&!fr.completed&&f.motor_yaw()==46);
    f.desired=90;f.step_align=true;facing(true);assert(fr.yaw_written&&fr.stepped&&fr.completed&&fr.error==0&&f.motor_yaw()==90);
    f.step_align=false;f.put(f.a,0x370,0.f);f.put(f.motor,0x14,.0059f);f.desired=.0059;facing(true);assert(fr.completed);
    f.put(f.motor,0x14,.0061f);f.desired=.0061;facing(true);assert(!fr.completed);
    f.put(f.a,0xc18,77u);f.component_handles=true;facing(false);assert(fr.shared.component_handled&&!fr.shared.native&&f.trace==std::vector<int>{2});
    f.component_handles=false;facing(false);assert(fr.shared.component_called&&fr.shared.native&&fr.completed&&(f.trace==std::vector<int>{2,10,11,13}));
    f.put(f.a,0xc18,0u);f.put(f.a,0x908,0u);
    assert(!face->execute(f.a,sizeof(f.a),&ft,true,12,&b,&fb,&fr));
    run(60,false,0,12);assert(o.parent_called&&!o.native);
    f.put(f.a,0x908,88u);f.put(f.a,0x370,90.f);f.put(f.motor,0x14,0.f);
    f.aimed=true;f.desired=180;
    fb.desired_yaw=[](void* p,void*,bool mode,double* out) noexcept -> int{auto& f=F::self(p);f.trace.push_back(mode?13:16);*out=f.desired;return 1;};
    ft={56,0};facing(false);assert(fr.begun&&!fr.yaw_written&&f.motor_yaw()==0&&f.trace==std::vector<int>{14});
    ft={57,0};facing(false);assert(fr.begun&&fr.yaw_written&&f.motor_yaw()==180&&(f.trace==std::vector<int>{16,14}));
    f.put(f.a,0x9f4,1.f);f.put(f.a,0xa00,2.f);
    fb.vector_yaw=[](void* p,void*,const float* v,float* out) noexcept -> int{assert(v[0]==1.f||v[0]==2.f);F::self(p).trace.push_back(12);*out=30;return 1;};
    for(int id: {64,65}){ft={id,0};facing(false);assert(fr.begun&&fr.vector_written&&fr.yaw_written&&f.motor_yaw()==180&&(f.trace==std::vector<int>{12,13,14}));}
    ft={66,0};facing(false);assert(fr.completed&&fr.yaw_written&&!fr.begun&&f.motor_yaw()==90&&f.trace.empty());
    ft={117,45};facing(false);assert(fr.begun&&f.motor_yaw()==135);
    ft={118,45};facing(false);assert(fr.begun&&f.motor_yaw()==45);
    f.put(f.a,0x370,270.f);ft={117,90};facing(false);assert(f.motor_yaw()==0);
    for(int id: {56,57,58,61,63,64,65,112,117,118}){
        ft={id,0};f.put(f.a,0x370,0.f);f.put(f.motor,0x14,90.f);f.step_align=false;
        facing(true);assert(fr.stepped&&!fr.completed&&f.trace==std::vector<int>{15});
        f.step_align=true;facing(true);assert(fr.stepped&&fr.completed&&fr.error==0&&f.trace==std::vector<int>{15});
    }
    f.step_align=false;ft={58,0};assert(!face->execute(f.a,sizeof(f.a),&ft,false,12,&b,&fb,&fr));
    ft={66,0};assert(!face->execute(f.a,sizeof(f.a),&ft,true,12,&b,&fb,&fr));
    auto gt=reinterpret_cast<GetServerNpcSharedTargetAnimationApi>(GetProcAddress(s,server_npc_shared_target_animation_export));
    assert(gt&&!GetProcAddress(c,server_npc_shared_target_animation_export)&&!gt(0,sizeof(ServerNpcSharedTargetAnimationApi))&&!gt(1,0));
    const auto* target_api=gt(1,sizeof(ServerNpcSharedTargetAnimationApi));assert(target_api);
    sin_independent::EntityHandleEntry slots[2]{};slots[1].serial=7;slots[1].entity=reinterpret_cast<CBaseEntity*>(f.target);
    ServerNpcSharedTargetAnimationBindings tb{};tb.context=&f;tb.entries=slots;tb.count=2;tb.motor_storage=fb.motor_storage;tb.step_turn=fb.step_turn;
    tb.current_target=[](void* p,void*,void** out) noexcept -> int{auto& f=F::self(p);f.trace.push_back(20);*out=f.target_present?f.target:nullptr;return 1;};
    tb.absolute_position=[](void* p,void* target,float* out) noexcept -> int{auto& f=F::self(p);assert(target==f.target);f.trace.push_back(21);std::memcpy(out,f.target_pos,12);return 1;};
    f.put(f.a,0x908,88u);f.put(f.a,0x8a8,0x7001u);f.put(f.a,0x360,1.f);f.put(f.a,0x364,2.f);f.put(f.a,0x368,3.f);
    f.put(f.motor,0x18,123.f);f.target_pos[0]=1;f.target_pos[1]=-5;f.target_pos[2]=30;f.a[0x3e0]=0;
    ServerNpcSharedTargetAnimationResult tr{};
    auto anim=[&](int id){f.put(f.a,0x7a4,1);f.trace.clear();ServerNpcScheduleTask at{id,0};assert(target_api->execute(f.a,sizeof(f.a),&at,12,&b,&tb,&tr));};
    anim(94);assert(tr.shared.native&&tr.target_live&&tr.motor_written&&tr.stepped&&!tr.animation_finished&&tr.direction[0]==0&&tr.direction[1]==-7&&tr.direction[2]==27&&tr.yaw==270&&f.motor_yaw()==270&&(f.trace==std::vector<int>{20,21,15}));
    f.a[0x3e0]=1;f.target_pos[0]=2;f.target_pos[1]=2;anim(95);
    assert(tr.animation_finished&&tr.shared.lifecycle.completed&&tr.yaw==0&&(f.trace==std::vector<int>{20,21,15}));
    float unchanged=0;std::memcpy(&unchanged,f.motor+0x18,4);assert(unchanged==123.f);
    f.target_pos[0]=1;f.target_pos[1]=3;anim(96);assert(tr.target_handle==0x7001u&&tr.yaw==90&&tr.shared.lifecycle.completed&&(f.trace==std::vector<int>{21,15}));
    f.put(f.a,0x8a8,0x8001u);anim(96);assert(!tr.target_live&&!tr.motor_written&&!tr.stepped&&tr.shared.lifecycle.completed&&f.trace.empty());
    f.target_present=false;anim(95);assert(!tr.target_live&&tr.shared.lifecycle.completed&&f.trace==std::vector<int>{20});
    f.a[0x3e0]=0;anim(95);assert(!tr.shared.lifecycle.completed);
    f.target_present=true;f.put(f.a,0xc18,77u);f.component_handles=true;anim(95);
    assert(tr.shared.component_handled&&!tr.shared.native&&f.trace==std::vector<int>{2});
    f.component_handles=false;anim(95);assert(tr.shared.component_called&&tr.shared.native&&(f.trace==std::vector<int>{2,20,21,15}));
    f.put(f.a,0xc18,0u);f.put(f.a,0x908,99u);ServerNpcScheduleTask bad{95,0};
    assert(!target_api->execute(f.a,sizeof(f.a),&bad,12,&b,&tb,&tr));
    run(95,true,0,12);assert(o.parent_called&&!o.native);
    auto gm=reinterpret_cast<GetServerNpcSharedMovementApi>(GetProcAddress(s,server_npc_shared_movement_export));
    assert(gm&&!GetProcAddress(c,server_npc_shared_movement_export)&&!gm(0,sizeof(ServerNpcSharedMovementApi))&&!gm(1,0));
    const auto* mv=gm(1,sizeof(ServerNpcSharedMovementApi));assert(mv);
    ServerNpcSharedMovementBindings mb{};mb.context=&f;
    mb.storage=[](void* p,std::uint32_t tok,void** ptr,unsigned* n) noexcept -> int{
        auto& f=F::self(p);if(tok==100){*ptr=f.nav;*n=sizeof(f.nav);return 1;}if(tok==101){*ptr=f.path;*n=sizeof(f.path);return 1;}return 0;};
    mb.clear_path=[](void* p,void* nav) noexcept -> int{auto& f=F::self(p);assert(nav==f.nav);f.trace.push_back(30);f.put(f.path,0,0u);f.put(f.path,0x44,0u);return 1;};
    mb.stop_path=[](void* p,void* nav,bool finished) noexcept -> int{auto& f=F::self(p);assert(nav==f.nav);f.trace.push_back(finished?32:31);return 1;};
    mb.try_stopping_path=[](void* p,void* nav,bool* yes) noexcept -> int{auto& f=F::self(p);assert(nav==f.nav);f.trace.push_back(33);*yes=f.can_stop;return 1;};
    mb.pose_lookup=[](void* p,void*,const char* name,int* id) noexcept -> int{auto& f=F::self(p);assert(!std::strcmp(name,"move_yaw"));f.trace.push_back(34);*id=f.pose_index;return 1;};
    mb.pose_set=[](void* p,void*,const char* name,float value) noexcept -> int{assert(!std::strcmp(name,"move_yaw")&&value==0.f);F::self(p).trace.push_back(35);return 1;};
    mb.velocity=[](void* p,void*,float* out) noexcept -> int{auto& f=F::self(p);f.trace.push_back(36);std::memcpy(out,f.speed,12);return 1;};
    mb.movement_update=[](void* p,void*) noexcept -> int{F::self(p).trace.push_back(37);return 1;};
    mb.task_fail=[](void* p,void*,int id) noexcept -> int{assert(id==26);F::self(p).trace.push_back(38);return 1;};
    f.put(f.a,0x8f8,100u);f.put(f.nav,0x24,101u);f.put(f.path,0,1u);f.put(f.path,0x44,2u);f.put(f.path,0x24,45);
    ServerNpcSharedMovementResult mr{};
    auto moving=[&](int id,bool running,float param,float now=12.f){f.put(f.a,0x7a4,1);f.trace.clear();ServerNpcScheduleTask mt{id,param};assert(mv->execute(f.a,sizeof(f.a),&mt,running,now,&b,&mb,&mr));};
    moving(116,false,1);assert(mr.stop_calls==1&&mr.pose_calls==1&&!mr.shared.lifecycle.completed&&(f.trace==std::vector<int>{31,34,35}));
    moving(116,false,0);assert(mr.clear_calls==1&&!mr.shared.lifecycle.completed&&(f.trace==std::vector<int>{30,34,35}));
    moving(116,false,0);assert(mr.shared.lifecycle.completed&&mr.idle_activity==1&&(f.trace==std::vector<int>{30,1}));
    f.can_stop=true;moving(116,false,1);assert(mr.try_calls==1&&!mr.shared.lifecycle.completed&&f.trace==std::vector<int>{33});
    f.can_stop=false;moving(116,false,1);assert(mr.try_calls&&mr.shared.lifecycle.completed&&(f.trace==std::vector<int>{33,30,1}));
    f.put(f.nav,0xc,1);f.a[0xbc]=0;f.speed[0]=.02f;moving(116,true,0);assert(!mr.mode_cleared&&!mr.shared.lifecycle.completed&&f.trace==std::vector<int>{36});
    f.a[0xbc]=1;moving(116,true,0);assert(mr.landed&&mr.mode_cleared&&mr.shared.lifecycle.completed&&f.trace==std::vector<int>{1});
    f.put(f.nav,0xc,1);f.a[0xbc]=0;f.speed[0]=0;moving(116,true,0);assert(mr.stuck&&mr.mode_cleared&&mr.shared.failed_code==26&&(f.trace==std::vector<int>{36,38,1}));
    f.put(f.nav,0xc,3);moving(116,true,0);assert(!mr.shared.lifecycle.completed&&f.trace.empty());
    f.put(f.nav,0xc,0);f.put(f.path,0,1u);f.put(f.path,0x44,2u);f.put(f.path,0x24,45);
    moving(121,false,0);assert(mr.move_calls==1&&f.trace==std::vector<int>{37});
    f.put(f.path,0,0u);moving(121,false,0);assert(mr.idle_activity==1&&!mr.shared.lifecycle.completed&&f.trace==std::vector<int>{1});
    f.put(f.path,0,1u);f.put(f.a,0x7ac,10.f);
    moving(121,true,2,12);assert(!mr.timed_out&&mr.move_calls==1);
    moving(121,true,2,12.25f);assert(mr.timed_out&&mr.shared.lifecycle.completed&&mr.stop_calls==1&&f.trace==std::vector<int>{32});
    f.put(f.path,0x44,0u);moving(121,true,0);assert(mr.shared.lifecycle.completed&&mr.stop_calls==1);
    moving(116,true,1);assert(mr.nested_calls==1&&mr.nested.lifecycle.completed&&mr.stop_calls==1&&f.trace==std::vector<int>{32});
    moving(121,false,0);assert(mr.shared.lifecycle.completed&&mr.clear_calls==1&&f.trace==std::vector<int>{30});
    f.put(f.a,0xc18,77u);f.component_handles=true;moving(116,false,0);assert(mr.shared.component_handled&&!mr.shared.native&&f.trace==std::vector<int>{2});
    f.component_handles=false;moving(116,true,1);assert(mr.shared.component_called&&mr.nested.component_called&&(f.trace==std::vector<int>{2,2,32}));
    f.put(f.a,0xc18,0u);
    moving(89,false,48.75f);float raw=0;std::memcpy(&raw,f.path+4,4);assert(raw==48.f&&mr.shared.lifecycle.completed);
    moving(90,false,-5.75f);std::memcpy(&raw,f.nav+0x4c,4);assert(raw==-5.f&&mr.shared.lifecycle.completed);
    f.put(f.path,0,0u);moving(122,false,0);assert(mr.shared.lifecycle.completed&&!mr.clear_calls);
    f.put(f.path,0,1u);f.put(f.path,0x44,2u);f.a[0x3e0]=0;
    moving(122,false,0);assert(mr.move_calls==1&&!mr.shared.lifecycle.completed);
    f.a[0x3e0]=1;f.put(f.a,0x3ec,7);f.put(f.a,0x884,7);
    moving(122,false,0);assert(mr.shared.lifecycle.completed&&!mr.move_calls);
    f.put(f.path,0x44,0u);moving(122,true,0);assert(mr.shared.lifecycle.completed&&mr.stop_calls==1);
    ServerNpcTransitionBindings mt{};ServerActivityDictionary dict{};ServerEventDictionary events{};
    auto& at=mt.assignment.translation;at.actor_class=NpcActivityHosted;at.activities=&dict;at.events=&events;
    at.weapon=[](void*,void*,int id,bool* req,int* out) noexcept -> int{*req=false;*out=id;return 1;};
    at.other=[](void*,void*,int id,int* out) noexcept -> int{*out=id;return 1;};
    at.metadata=[](void*,void*,const ServerActivityCompositeView** out) noexcept -> int{*out=nullptr;return 0;};
    b.transition=&mt;
    f.put(f.a,0x988,0x13u);moving(43,false,0);unsigned chosen=0;std::memcpy(&chosen,f.path+8,4);
    assert(chosen==10&&mr.shared.activity==10&&f.get<unsigned>(0x988)==0x11u&&mr.shared.lifecycle.completed);
    f.put(f.a,0x204,0);moving(44,false,0);std::memcpy(&chosen,f.path+8,4);assert(chosen==6&&mr.shared.lifecycle.completed);
    for(int mode: {4,5}){f.put(f.a,0x204,mode);moving(44,false,0);std::memcpy(&chosen,f.path+8,4);assert(chosen==25&&mr.shared.lifecycle.completed);}
    b.transition=nullptr;ServerNpcScheduleTask no_translation{43,0};assert(!mv->execute(f.a,sizeof(f.a),&no_translation,false,12,&b,&mb,&mr));
    f.put(f.a,0x8f8,99u);ServerNpcScheduleTask bad_move{116,0};
    assert(!mv->execute(f.a,sizeof(f.a),&bad_move,false,12,&b,&mb,&mr));
    run(116,false,0,12);assert(o.parent_called&&!o.native);
    auto gdw=reinterpret_cast<GetServerNpcDifficultyWaitApi>(GetProcAddress(s,server_npc_difficulty_wait_export));
    using DifficultyApi=sin_port::fixed_service::Api;
    using GetDifficultyApi=const DifficultyApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
    auto gdiff=reinterpret_cast<GetDifficultyApi>(GetProcAddress(s,sin_port::fixed_service::export_name));
    assert(gdw&&gdiff&&!GetProcAddress(c,server_npc_difficulty_wait_export)&&!gdw(0,sizeof(ServerNpcDifficultyWaitApi))&&!gdw(1,0));
    const auto* dw=gdw(1,sizeof(ServerNpcDifficultyWaitApi));const auto* diff=gdiff(1,sizeof(DifficultyApi));assert(dw&&diff);
    f.put(f.a,0xc18,0u);
    assert(!api->supports(138,false)&&api->supports(138,true));
    run(138,false,.3f,20);assert(o.parent_called&&!o.native&&f.trace==std::vector<int>{4});
    b.random=[](void* p,int lo,int hi,int* out) noexcept -> int{auto& f=F::self(p);assert(lo==1&&hi>=lo);f.trace.push_back(hi);*out=hi;return 1;};
    ServerNpcScheduleTask dt{138,.3f};
    f.trace.clear();assert(dw->execute(f.a,sizeof(f.a),&dt,false,20,.1f,&b,&o)&&o.native&&o.timer_written&&o.ticks==4&&f.get<float>(0x9dc)==20.4f&&f.trace==std::vector<int>{4});
    f.trace.clear();assert(dw->execute(f.a,sizeof(f.a),&dt,false,20,-.1f,&b,&o)&&o.ticks==3&&f.get<float>(0x9dc)==20.3f&&f.trace==std::vector<int>{3});
    // Use the stored deadline as the witness: f32 addition may round upward at .3-.1.
    const float until=f.get<float>(0x9dc);
    f.put(f.a,0x7a4,1);assert(dw->execute(f.a,sizeof(f.a),&dt,true,until-.01f,0,&b,&o)&&!o.lifecycle.completed);
    assert(dw->execute(f.a,sizeof(f.a),&dt,true,until,0,&b,&o)&&o.lifecycle.completed);
    dt.parameter=.1f;f.trace.clear();assert(dw->execute(f.a,sizeof(f.a),&dt,false,20,-.1f,&b,&o)&&o.ticks==0&&f.trace.empty()&&f.get<float>(0x9dc)==20.f);
    assert(diff->begin_campaign(1)&&api->supports(138,false));
    for(int skill=1;skill<=3;++skill){
        assert(diff->set_skill(skill));float metric=0;assert(diff->get_metric(11,&metric));
        dt.parameter=.3f;f.trace.clear();assert(dw->execute(f.a,sizeof(f.a),&dt,false,21,metric,&b,&o));const int expected=o.ticks;const float deadline=f.get<float>(0x9dc);
        run(138,false,.3f,21);assert(o.native&&o.timer_written&&o.ticks==expected&&f.get<float>(0x9dc)==deadline);
    }
    assert(diff->end_campaign()&&!api->supports(138,false));
    run(138,false,.3f,22);assert(o.parent_called&&!o.native);
    // No-op start entries deliberately do NOT complete the task.
    for(int id:{6,13,97,113,115}){
        f.put(f.a,0x9dc,81.f);run(id,false,123,30);
        assert(o.native&&!o.lifecycle.completed&&f.get<int>(0x7a4)==1&&f.get<float>(0x9dc)==81.f&&f.trace.empty());
    }
    run(115,true,123,30);assert(o.native&&!o.lifecycle.completed&&f.trace.empty());
    run(7,false,3.9f,30);assert(o.native&&o.lifecycle.completed&&f.get<int>(0x858)==3);
    run(7,false,3,30);assert(o.lifecycle.completed&&f.get<int>(0x858)==3);
    run(7,false,-2.9f,30);assert(o.lifecycle.completed&&f.get<int>(0x858)==-2);
    run(128,false,999,30);assert(o.native&&o.activity==70&&!o.lifecycle.completed&&f.trace==std::vector<int>{70});
    f.a[0x3e0]=0;run(128,true,0,30);assert(!o.lifecycle.completed&&f.trace.empty());
    f.a[0x3e0]=1;run(128,true,0,30);assert(o.lifecycle.completed&&f.trace.empty());
    run(131,false,123,30);assert(o.native&&o.ticks==40&&o.timer_written&&f.get<float>(0x9dc)==34.f&&!o.lifecycle.completed);
    run(131,true,0,30);assert(o.parent_called&&!o.native);
    f.put(f.a,0xc18,77u);f.component_handles=true;run(6,false,0,30);assert(o.component_handled&&!o.native);
    f.component_handles=false;run(6,false,0,30);assert(o.component_called&&o.native&&!o.lifecycle.completed);
    FreeLibrary(s);FreeLibrary(c);std::puts("PASS original shared task timer, schedule and component routes");
}
