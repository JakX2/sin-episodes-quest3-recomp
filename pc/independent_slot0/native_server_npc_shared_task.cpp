#include "native_server_npc_shared_task.h"
#include <cmath>
#include <cstring>
#include <limits>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcTaskLifecycleApi* __cdecl SiN_GetServerNpcTaskLifecycleApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcTransitionApi* __cdecl SiN_GetServerNpcTransitionApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleResolutionApi* __cdecl SiN_GetServerNpcScheduleResolutionApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleNamespaceApi* __cdecl SiN_GetServerNpcScheduleNamespaceApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleStateApi* __cdecl SiN_GetServerNpcScheduleStateApi(std::uint32_t,std::uint32_t) noexcept;
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
    if(running)return id==2||id==4||id==5||id==86||id==92||id==114;
    return id==2||id==4||id==5||id==86||id==87||id==88||id==92||id==95||id==114||id==119;
}
int __cdecl execute(void* a,unsigned n,const ServerNpcScheduleTask* t,bool running,float now,const ServerNpcSharedTaskBindings* b,ServerNpcSharedTaskResult* out) noexcept{
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
        if(t->id==92){
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
    if(t->id==2||t->id==4||t->id==5||t->id==114){
        const bool random=t->id==5||t->id==114;
        if(!wait(a,n,now,random?0.f:t->parameter,random?t->parameter:0.f,b,&out->ticks))return 0;
        out->timer_written=1;return 1;
    }
    int value=0;if(!integer(std::trunc(static_cast<double>(t->parameter)),value))return 0;
    switch(t->id){
    case 86:if(!value){wr(a,0x878,0);return 1;}return pending(value);
    case 92:case 95:return pending(value);
    case 88:wr(a,0x7bc,value);return complete();
    case 119:wr(a,0x988,rd<unsigned>(a,0x988)|static_cast<unsigned>(value));return complete();
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
const ServerNpcSharedTaskApi api{1,sizeof(ServerNpcSharedTaskApi),&wait,&cancel_wait,&wait_state,&supports,&execute};
 }
int bind_shared_parent_internal(void* actor,unsigned bytes,float now,const ServerGruntTaskBindings* original,const ServerNpcScheduleResolutionBindings* resolution,const ServerNpcSharedTaskBindings* shared,ServerNpcSharedTaskStats* stats,ServerNpcSharedParentSession* session,ServerGruntTaskBindings* bound) noexcept{
    if(!actor||!original||!resolution||!shared||!stats||!session||!bound)return 0;
    *stats={};*session={actor,bytes,now,*original,*shared,stats};*bound=*original;
    session->shared.lifecycle=original->lifecycle;
    session->shared.transition=original->transition;
    session->shared.resolution=resolution;
    bound->context=session;
    bound->parent=[](void* p,void* a,const ServerNpcScheduleTask* t,bool running) noexcept -> int{
        auto& q=*static_cast<ServerNpcSharedParentSession*>(p);
        if(a!=q.actor||!t)return 0;
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
