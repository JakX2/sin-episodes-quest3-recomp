#include "native_server_grunt_combat_task.h"
#include "native_server_weapon_script_view.h"
#include <cmath>
#include <cstring>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcTaskLifecycleApi* __cdecl SiN_GetServerNpcTaskLifecycleApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcConditionApi* __cdecl SiN_GetServerNpcConditionApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerGruntTaskDispatchApi* __cdecl SiN_GetServerGruntTaskDispatchApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcInheritedScheduleApi* __cdecl SiN_GetServerNpcInheritedScheduleApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerWeaponScriptViewApi* __cdecl SiN_GetServerWeaponScriptViewApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcTransitionApi* __cdecl SiN_GetServerNpcTransitionApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* p,unsigned off) noexcept{T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T>void wr(void* p,unsigned off,T v) noexcept{std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
int __cdecl supports(int id,bool running) noexcept{return running?(id==1||id==67||id==146):(id==3||id==67||id==140||id==146);}
struct Call{
    void* a;unsigned n;float now;const ServerGruntCombatTaskBindings& b;ServerGruntCombatTaskResult& o;
    const ServerNpcTaskLifecycleApi* life;
    const ServerNpcConditionApi* cond;
    bool ok=true;
    void* entity(unsigned off) noexcept{return sin_independent::EntityHandleView(b.grunt.entities.entries,b.grunt.entities.count).Resolve(rd<std::uint32_t>(a,off));}
    bool complete() noexcept{return life->complete(a,n,now,false,&b.grunt.lifecycle,&o.base.lifecycle)!=0;}
    bool finished(unsigned& yes) noexcept{return life->animation_finished(a,n,&yes)!=0;}
    bool has(int id) noexcept{ServerNpcConditionResult r{};if(!ok||!cond->test(a,n,id,NpcConditionCurrent,&b.grunt.lifecycle.conditions,&r)){ok=false;return false;}return r.value!=0;}
    bool activity(int id,bool restart) noexcept{
        if(restart&&rd<int>(a,0x878)==id)wr(a,0x878,0);
        o.activity=o.base.activity=id;o.base.activity_requested=1;
        if(b.grunt.transition){
            const auto* api=SiN_GetServerNpcTransitionApi(1,sizeof(ServerNpcTransitionApi));
            return api&&api->set_pending(a,n,id,b.grunt.transition,&o.base.transition);
        }
        return b.activity&&b.activity(b.context,a,id,restart);
    }
    bool deadline(double value) noexcept{
        const float f=static_cast<float>(value);if(!std::isfinite(f))return false;
        if(rd<float>(a,0x60c)==f)return true;
        if(rd<std::uint8_t>(a,0x1f0))wr<std::uint8_t>(a,0x200,1);
        else if(auto token=rd<std::uint32_t>(a,0x16c)){if(!b.dirty||!b.dirty(b.context,a,token))return false;}
        wr(a,0x60c,f);++o.deadline_changed;return true;
    }
    bool next_burst() noexcept{
        const float interval=rd<float>(a,0xc9c);
        if(!activity(16,true))return false;
        wr(a,0x99c,now);return deadline(static_cast<double>(now)+interval-.1);
    }
    bool start_burst(int id) noexcept{
        if(void* w=entity(0x78c)){
            o.weapon_live=1;
            const auto* svc=SiN_GetServerNpcInheritedScheduleApi(1,sizeof(ServerNpcInheritedScheduleApi));
            const auto* script=SiN_GetServerWeaponScriptViewApi(1,sizeof(ServerWeaponScriptViewApi));
            auto wb=b.weapon;wb.entities=b.grunt.entities;
            int burst=0;double interval=0;unsigned kind=0;
            if(!svc||!script||!svc->burst(w,&wb,&burst))return false;
            wr(a,0xc98,burst);o.burst=burst;
            if(!wb.entities.classify||!wb.entities.classify(wb.entities.context,w,&kind)||!script->interval(kind,&interval)||!std::isfinite(interval))return false;
            wr(a,0xc9c,static_cast<float>(interval));
        }
        if(!deadline(static_cast<double>(now)+rd<float>(a,0xc9c)-.1)||!activity(id==146?196:16,true))return false;
        wr(a,0x99c,now);return true;
    }
    bool cue(unsigned slot,int x,int y) noexcept{return b.grunt.cue&&b.grunt.cue(b.grunt.context,a,slot,x,y);}
    bool wait2() noexcept{float v=static_cast<float>(static_cast<double>(now)+2.0);if(!std::isfinite(v))return false;wr(a,0x9dc,v);return true;}
    bool aim(bool& yes) noexcept{++o.aim_queries;return b.aim_query&&b.aim_query(b.context,a,&yes);}
    bool aim_update(bool already) noexcept{++o.aim_updates;return b.aim_update&&b.aim_update(b.context,a,already);}
    bool run_burst(bool stun) noexcept{
        if(!stun){
            if(!b.grunt.turn_update||!b.grunt.turn_update(b.grunt.context,a))return false;
            bool yes=false;if(!aim(yes)||!aim_update(yes))return false;
        }
        const float when=rd<float>(a,0x60c);if(!std::isfinite(when))return false;
        unsigned yes=0;if(now<when)return true;if(!finished(yes))return false;if(!yes)return true;
        if(!stun){
            const auto old=rd<std::uint32_t>(a,0xc98);wr(a,0xc98,old-1u);
            return rd<int>(a,0xc98)>0?next_burst():complete();
        }
        if(!entity(0x8a0))return activity(196,true)&&complete();
        bool aimed=false;if(!aim(aimed))return false;
        if(aimed){
            const int old=rd<int>(a,0xc98);wr(a,0xc98,rd<std::uint32_t>(a,0xc98)-1u);
            if(old>0)return next_burst();
        }
        // Original second +304 query is independent of first.
        if(!aim(aimed))return false;
        const bool reload=has(4)||has(3);if(!ok)return false;
        if(!aimed)return reload?activity(196,true):aim_update(false);
        if(reload)return activity(66,true);
        return aim_update(true)&&complete();
    }
};
int __cdecl execute(void* a,unsigned n,const ServerNpcScheduleTask* t,bool running,float now,const ServerGruntCombatTaskBindings* b,ServerGruntCombatTaskResult* out) noexcept{
    if(!out)return 0;*out={};out->activity=-1;out->base.activity=-1;
    if(!a||n<0xfcc||!t||!b||!std::isfinite(now)||b->grunt.entities.count>sin_independent::kEntityHandleCapacity||(b->grunt.entities.count&&!b->grunt.entities.entries))return 0;
    const auto* base=SiN_GetServerGruntTaskDispatchApi(1,sizeof(ServerGruntTaskDispatchApi));
    if(!supports(t->id,running))return base&&base->execute(a,n,t,running,now,&b->grunt,&out->base);
    const auto* life=SiN_GetServerNpcTaskLifecycleApi(1,sizeof(ServerNpcTaskLifecycleApi));
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));if(!life||!cond)return 0;
    out->recovered=1;out->base.route=GruntTaskNative;out->base.task=t->id;
    Call q{a,n,now,*b,*out,life,cond};
    if(!running){
        wr(a,0x9f0,.75f);
        if(t->id==67||t->id==146)return q.start_burst(t->id);
        if(t->id==140){
            const auto squad=rd<std::uint32_t>(a,0x9c4);
            if(squad){
                int count=0;if(!b->squad_count||!b->squad_count(b->context,squad,&count))return 0;
                if(count>2)if(void* e=q.entity(0x8a0)){
                    double d=0;if(!b->distance||!b->distance(b->context,a,e,&d)||!std::isfinite(d))return 0;
                    if(d>512.0&&!q.deadline(static_cast<double>(now)+t->parameter))return 0;
                }
            }
            return q.complete();
        }
        if(!std::isfinite(t->parameter))return 0;
        if(std::trunc(t->parameter)!=1.f)return q.cue(0x7e8,1,2)&&q.activity(1,false)&&q.wait2();
        void* target=nullptr;if(void* e=q.entity(0x8a0)){if(!b->grunt.combat_target||!b->grunt.combat_target(b->grunt.context,e,&target))return 0;}
        if(!target)return q.complete();
        bool player=false,gate=false;
        if(!b->is_player||!b->is_player(b->context,target,&player))return 0;if(!player)return q.complete();
        if(!b->grunt.target_gate||!b->grunt.target_gate(b->grunt.context,target,a,&gate))return 0;if(gate)return q.complete();
        const float prior=rd<float>(a,0x99c);if(!std::isfinite(prior))return 0;
        if(static_cast<double>(now)-prior<=3.0)return q.complete();
        wr(a,0x99c,now);return q.cue(0x7cc,2,2)&&q.wait2()&&q.activity(rd<std::uint8_t>(a,0xc64)?1:61,false);
    }
    if(t->id==1||t->id==146)return q.run_burst(t->id==146);
    // RunTask67 selects inventory slot0 (+6cc), NOT active weapon.
    void* w=q.entity(0x6cc);unsigned kind=0;
    if(w&&(!b->grunt.entities.classify||!b->grunt.entities.classify(b->grunt.entities.context,w,&kind)))return 0;
    if(!w||kind!=WeaponActivitySmg){out->base.parent_called=1;return b->grunt.parent&&b->grunt.parent(b->grunt.context,a,t,true);}
    unsigned yes=0;if(!q.finished(yes))return 0;if(!yes)return 1;
    if(!b->shared_smg_stamp||!std::isfinite(*b->shared_smg_stamp))return 0;
    if(*b->shared_smg_stamp==0.f){*b->shared_smg_stamp=now;out->shared_changed=1;return 1;}
    if(static_cast<double>(now)-*b->shared_smg_stamp<=static_cast<double>(.9f))return 1;
    *b->shared_smg_stamp=0;out->shared_changed=1;return q.complete();
}
const ServerGruntCombatTaskApi api{1,sizeof(ServerGruntCombatTaskApi),&supports,&execute};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerGruntCombatTaskApi=_SiN_GetServerGruntCombatTaskApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerGruntCombatTaskApi* __cdecl SiN_GetServerGruntCombatTaskApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerGruntCombatTaskApi)?&api:nullptr;
}
