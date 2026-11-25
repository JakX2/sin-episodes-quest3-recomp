#include "native_server_grunt_helper_task.h"
#include <cmath>
#include <cstring>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerGruntInjuryTaskApi* __cdecl SiN_GetServerGruntInjuryTaskApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcTaskLifecycleApi* __cdecl SiN_GetServerNpcTaskLifecycleApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcConditionApi* __cdecl SiN_GetServerNpcConditionApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* p,unsigned off) noexcept{T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T>void wr(void* p,unsigned off,T v) noexcept{std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
bool finite3(const float* v) noexcept{return std::isfinite(v[0])&&std::isfinite(v[1])&&std::isfinite(v[2]);}
double distance2(const float* a,const float* b) noexcept{
    const double x=static_cast<double>(a[0])-b[0],y=static_cast<double>(a[1])-b[1],z=static_cast<double>(a[2])-b[2];return z*z+y*y+x*x;
}
int __cdecl supports(int id,bool running) noexcept{return id==143||(!running&&id==151);}
int __cdecl execute(void* a,unsigned n,const ServerNpcScheduleTask* task,bool running,float now,const ServerGruntHelperTaskBindings* b,ServerGruntHelperTaskResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!a||n<0xfcc||!task||!b||!std::isfinite(now))return 0;
    const auto& g=b->injury.combat.grunt;const auto& reg=g.entities;
    if(reg.count>sin_independent::kEntityHandleCapacity||(reg.count&&!reg.entries))return 0;
    const auto* injury=SiN_GetServerGruntInjuryTaskApi(1,sizeof(ServerGruntInjuryTaskApi));
    if(!supports(task->id,running))return injury&&injury->execute(a,n,task,running,now,&b->injury,&out->injury);
    const auto* life=SiN_GetServerNpcTaskLifecycleApi(1,sizeof(ServerNpcTaskLifecycleApi));
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));if(!life||!cond)return 0;
    out->recovered=1;auto& o=out->injury.combat.base;o.route=GruntTaskNative;o.task=task->id;o.activity=-1;
    auto complete=[&](){return life->complete(a,n,now,false,&g.lifecycle,&o.lifecycle)!=0;};
    auto fail=[&](int code){out->failure=code;return life->fail(a,n,now,code,&g.lifecycle,&o.lifecycle)!=0;};
    const auto registry=sin_independent::EntityHandleView(reg.entries,reg.count);
    auto enemy=[&](){return registry.Resolve(rd<std::uint32_t>(a,0x8a0));};
    auto origin=[&](void* actor,float* v){return b->origin&&b->origin(b->context,actor,v)&&finite3(v);};
    auto save=[&](void* e){
        float v[3]{};if(!origin(e,v))return false;
        std::memcpy(static_cast<unsigned char*>(a)+0xa00,v,12);out->origin_saved=1;return true;
    };
    auto follow=[&](){
        bool yes=false;return b->path_follow&&b->path_follow(b->context,a,&yes)&&b->path_follow_update&&b->path_follow_update(b->context,a,yes);
    };
    if(!running)wr(a,0x9f0,.75f);
    if(task->id==143){
        if(!std::isfinite(task->parameter))return 0;
        if(running&&!follow())return 0;
        void* e=enemy();out->enemy_live=e!=nullptr;if(!e)return fail(6);
        float self[3]{},target[3]{};
        if(!origin(e,target)||!origin(a,self))return 0;
        const double limit=static_cast<double>(task->parameter)*task->parameter;
        if(distance2(self,target)<=limit){
            if(running){
                if(!b->path_stop||!b->path_stop(b->context,a,true))return 0;out->path_stopped=1;
            }
            return complete();
        }
        bool gated=false;
        if(!running){
            if(!b->injury.path_gate||!b->injury.path_gate(b->injury.context,a,e,&gated))return 0;
            if(gated)return fail(11);
            bool success=false;
            if(!b->path_start||!b->path_start(b->context,a,e,&success))return 0;
            if(!success){if(!b->path_fallback||!b->path_fallback(b->context,a,e))return 0;return fail(11);}
            out->path_started=1;
            int activity=-1;if(!b->movement_activity||!b->movement_activity(b->context,a,&activity)||!b->path_activity||!b->path_activity(b->context,a,activity==-1?6:10))return 0;
            wr(a,0x988,rd<unsigned>(a,0x988)&~2u);
            int status=0;if(!b->path_status||!b->path_status(b->context,a,&status))return 0;
            if(!status){
                if(!complete()||!b->path_stop||!b->path_stop(b->context,a,false))return 0;
                out->path_stopped=1;return 1;
            }
            wr(a,0x9f0,0.f);
            if(!follow()||!b->path_tolerance||!b->path_tolerance(b->context,a,task->parameter))return 0;
            e=enemy();return e?save(e):0;
        }
        float prior[3]{};std::memcpy(prior,static_cast<unsigned char*>(a)+0xa00,12);
        if(!finite3(prior))return 0;
        // Original rebuild comparison includes equality, unlike the close-enemy test.
        if(distance2(prior,target)<limit)return 1;
        if(!b->injury.path_gate||!b->injury.path_gate(b->injury.context,a,e,&gated))return 0;
        if(gated)return fail(11);
        bool success=false;if(!b->path_rebuild||!b->path_rebuild(b->context,a,&success))return 0;
        if(!success)return fail(11);
        out->path_rebuilt=1;return save(e);
    }
    const auto squad=rd<std::uint32_t>(a,0x9c4);if(!squad)return complete();
    const ServerGruntHelperCandidate* rows=nullptr;unsigned count=0;
    if(!b->squad_candidates||!b->squad_candidates(b->context,squad,&rows,&count)||count>4096||(count&&!rows))return 0;
    unsigned injured=0;
    for(unsigned i=0;i<count;++i){
        if(!rows[i].actor)continue; // filtered non-Grunt / null entries
        if(rows[i].bytes<0xcac||!rows[i].token)return 0;
        injured+=rd<std::uint8_t>(rows[i].actor,0xca0)!=0;
    }
    if(injured>=2)return complete();
    ServerGruntHelperCandidate first{};
    if(!b->nearest||!b->nearest(b->context,squad,a,&first))return 0;
    if(!first.actor)return complete();
    if(first.bytes<0xcac||!first.token)return 0;
    auto distance=[&](void* e,double& d){return b->distance&&b->distance(b->context,a,e,&d)&&std::isfinite(d)&&d>=0;};
    double d=0;if(!distance(first.actor,d))return 0;
    if(d>=1200.0)return complete();
    auto available=[&](const ServerGruntHelperCandidate& row){
        return row.actor&&row.actor!=a&&rd<int>(row.actor,0x8c)==0&&!rd<std::uint8_t>(row.actor,0xca0)&&!rd<std::uint32_t>(row.actor,0xca4);
    };
    auto assign=[&](const ServerGruntHelperCandidate& row){
        std::uint32_t token=0,handle=0xffffffffu;
        if(!b->self_identity||!b->self_identity(b->context,a,&token,&handle)||!token||registry.Resolve(handle)!=a)return false;
        wr(a,0xca8,row.token);wr<std::uint8_t>(a,0xca0,1);
        ServerNpcConditionResult cr{};
        if(!cond->store(row.actor,row.bytes,0x4a,NpcConditionCurrent,true,&g.lifecycle.conditions,&cr))return false;
        wr(row.actor,0xca4,token);wr(row.actor,0x8a8,handle);
        out->helper_assigned=1;return complete();
    };
    if(available(first))return assign(first);
    for(unsigned i=0;i<count;++i){
        const auto& row=rows[i];++out->candidates;
        if(!available(row))continue;
        if(!distance(row.actor,d))return 0;
        if(d<1200.0)return assign(row);
    }
    return complete();
}
const ServerGruntHelperTaskApi api{1,sizeof(ServerGruntHelperTaskApi),&supports,&execute};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerGruntHelperTaskApi=_SiN_GetServerGruntHelperTaskApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerGruntHelperTaskApi* __cdecl SiN_GetServerGruntHelperTaskApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerGruntHelperTaskApi)?&api:nullptr;
}
