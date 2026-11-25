#include "native_server_grunt_task_dispatch.h"
#include <cstring>
#include <cmath>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcTaskLifecycleApi* __cdecl
SiN_GetServerNpcTaskLifecycleApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcConditionApi* __cdecl
SiN_GetServerNpcConditionApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcTransitionApi* __cdecl
SiN_GetServerNpcTransitionApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerActivityDictionaryApi* __cdecl
SiN_GetServerActivityDictionaryApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T read(const void* p,unsigned off) noexcept{T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T>void write(void* p,unsigned off,T v) noexcept{std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
int __cdecl route(int id,bool running,unsigned* out) noexcept {
    if(!out)return 0;*out=GruntTaskParent;
    if(running){
        switch(id){
        case 3:case 141:case 147:case 148:case 149:case 154:*out=GruntTaskNative;break;
        case 1:case 67:case 139:case 143:case 146:*out=GruntTaskHostedSpecial;break;
        }
    }else{
        switch(id){
        case 43:case 44:case 56:case 60:case 139:case 142:case 147:case 148:case 153:case 154:case 155:case 156:case 157:case 158:*out=GruntTaskNative;break;
        case 3:case 67:case 98:case 140:case 141:case 143:case 144:case 146:case 149:case 150:case 151:case 152:*out=GruntTaskHostedSpecial;break;
        }
    }
    return 1;
}
struct Call{
    void* a;unsigned bytes;float now;
    const ServerGruntTaskBindings& b;
    ServerGruntTaskResult& out;
    const ServerNpcTaskLifecycleApi* life;
    const ServerNpcConditionApi* cond;
    bool parent(const ServerNpcScheduleTask* task,bool running) noexcept{
        out.parent_called=1;return b.parent&&b.parent(b.context,a,task,running);
    }
    bool complete() noexcept{return life->complete(a,bytes,now,false,&b.lifecycle,&out.lifecycle)!=0;}
    bool fail(int id) noexcept{return life->fail(a,bytes,now,id,&b.lifecycle,&out.lifecycle)!=0;}
    bool condition(void* actor,unsigned n,int id,bool value) noexcept{
        ServerNpcConditionResult cr{};if(!cond->store(actor,n,id,NpcConditionCurrent,value,&b.lifecycle.conditions,&cr))return false;
        ++out.condition_writes;return true;
    }
    bool clear_link(void* actor,unsigned n,unsigned off,bool get_up) noexcept{
        if(n<0xcac)return false;
        const auto token=read<std::uint32_t>(actor,off);if(!token)return true;
        void* peer=nullptr;unsigned extent=0;
        if(!b.storage||!b.storage(b.context,token,&peer,&extent)||!peer||extent<0xcac)return false;
        if(get_up){
            // 2235a0f0: only live (+8c==0) linked Grunt.
            if(read<int>(peer,0x8c)==0){
                if(!condition(peer,extent,0x49,true))return false;
                write<std::uint8_t>(peer,0xca0,0);
                const auto helper=read<std::uint32_t>(peer,0xca8);
                if(helper){
                    void* target=nullptr;unsigned size=0;
                    if(!b.storage(b.context,helper,&target,&size)||!target||size<0xca8)return false;
                    write(target,0xca4,0u);write(target,0x8a8,0xffffffffu);write(peer,0xca8,0u);++out.helper_writes;
                }
            }
        }else{write(peer,0xca4,0u);write(peer,0x8a8,0xffffffffu);}
        write(actor,off,0u);++out.helper_writes;return true;
    }
    bool pending(int activity,bool restart) noexcept{
        if(restart&&read<int>(a,0x878)==activity)write(a,0x878,0);
        const auto* api=SiN_GetServerNpcTransitionApi(1,sizeof(ServerNpcTransitionApi));
        out.activity=activity;out.activity_requested=1;
        return api&&b.transition&&api->set_pending(a,bytes,activity,b.transition,&out.transition);
    }
    bool cue(unsigned slot,int x,int y) noexcept{return b.cue&&b.cue(b.context,a,slot,x,y);}
    bool peers(bool clear) noexcept{
        const auto squad=read<std::uint32_t>(a,0x9c4);if(!squad)return true;
        const ServerGruntTaskPeer* rows=nullptr;unsigned count=0;
        if(!b.peers||!b.peers(b.context,squad,&rows,&count)||count>4096||(count&&!rows))return false;
        for(unsigned i=0;i<count;++i)if(!rows[i].actor||rows[i].bytes<0xcbc)return false;
        const float deadline=static_cast<float>(static_cast<double>(now)+(read<std::uint8_t>(a,0xc6c)?2.5:6.0));
        if(!std::isfinite(deadline))return false;
        for(unsigned i=0;i<count;++i){
            if(clear)write(rows[i].actor,0xcb8,0xffffffffu);else write(rows[i].actor,0xc80,deadline);
            ++out.peer_writes;
        }
        return true;
    }
    void* entity(unsigned off) noexcept{return sin_independent::EntityHandleView(b.entities.entries,b.entities.count).Resolve(read<std::uint32_t>(a,off));}
};
int __cdecl execute(void* a,unsigned bytes,const ServerNpcScheduleTask* task,bool running,float now,const ServerGruntTaskBindings* b,ServerGruntTaskResult* out) noexcept{
    if(!out)return 0;*out={};out->activity=-1;
    if(!a||bytes<0xfcc||!task||!b||!std::isfinite(now)||b->entities.count>sin_independent::kEntityHandleCapacity||(b->entities.count&&!b->entities.entries))return 0;
    out->task=task->id;route(task->id,running,&out->route);
    const auto* life=SiN_GetServerNpcTaskLifecycleApi(1,sizeof(ServerNpcTaskLifecycleApi));
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));
    if(!life||!cond)return 0;
    Call q{a,bytes,now,*b,*out,life,cond};
    // Every base StartTask, including parent routes, sets this raw float.
    if(!running)write(a,0x9f0,.75f);
    if(out->route==GruntTaskParent)return q.parent(task,running);
    if(out->route==GruntTaskHostedSpecial){
        out->special_called=1;return b->special&&b->special(b->context,a,task,running);
    }
    if(running){
        switch(task->id){
        case 3:{
            void* e=q.entity(0x8a0);void* target=nullptr;
            if(e&&(!b->combat_target||!b->combat_target(b->context,e,&target)))return 0;
            bool gate=true;if(target&&(!b->target_gate||!b->target_gate(b->context,target,a,&gate)))return 0;
            if((!target||gate)&&!q.complete())return 0;
            const float deadline=read<float>(a,0x9dc);if(!std::isfinite(deadline))return 0;
            return now>=deadline?q.complete():1;
        }
        case 141:case 154:{
            unsigned finished=0;
            if(!b->turn_update||!b->turn_update(b->context,a)||!life->animation_finished(a,bytes,&finished))return 0;
            return finished?q.complete():1;
        }
        case 147:return q.complete();
        case 148:return q.clear_link(a,bytes,0xca4,true)&&q.complete();
        case 149:{
            void* target=q.entity(0x8a8);if(!target)return q.fail(6);
            const int sub=read<int>(a,0x7b4);
            if(sub<=0){write(a,0x7b4,static_cast<std::uint32_t>(sub)+1u);return 1;}
            write(a,0x7b4,0u);return b->path_refresh&&b->path_refresh(b->context,a,target);
        }
        }
    }else{
        switch(task->id){
        case 43:case 44:write(a,0x988,read<unsigned>(a,0x988)&~2u);return q.parent(task,false);
        case 56:case 60:{
            ServerNpcConditionResult cr{};
            if(task->id==60){
                if(!cond->test(a,bytes,0x14,NpcConditionCurrent,&b->lifecycle.conditions,&cr))return 0;
                if(cr.value)return q.complete();
            }
            if(!q.parent(task,false))return 0;
            const int mode=read<int>(a,0x204);return mode==4||mode==5?q.pending(0x1b,false):1;
        }
        case 139:return 1;
        case 142:return q.peers(false)&&q.complete();
        case 147:
            write<std::uint8_t>(a,0xca0,0);
            return q.condition(a,bytes,0x49,false)&&q.condition(a,bytes,0x47,false)&&q.clear_link(a,bytes,0xca8,false);
        case 148:
            if(!read<std::uint32_t>(a,0xca4))return 1;
            return q.clear_link(a,bytes,0xca4,true)&&q.complete();
        case 153:return q.cue(0x7e0,2,0)&&q.complete();
        case 154:{
            write(a,0x99c,now);
            const auto* ids=b->registration;const auto* api=SiN_GetServerActivityDictionaryApi(1,sizeof(ServerActivityDictionaryApi));
            int id=-1;
            if(!ids||!ids->activities_ready||!ids->activity_owner||ids->activity_generation!=ids->activity_owner->names.generation||
               !api||!api->lookup(ids->activity_owner,"ACT_MELEE_KICK",&id)||id<0||id!=ids->activities[4])return 0;
            return q.pending(id,true);
        }
        case 155:return q.peers(true)&&q.complete();
        case 156:return b->set_enemy&&b->set_enemy(b->context,a,q.entity(0xcbc),true)&&q.complete();
        case 157:return q.cue(0x7dc,0,2)&&q.complete();
        case 158:return b->finish_hook&&b->finish_hook(b->context,a)&&q.complete();
        }
    }
    return 0;
}
const ServerGruntTaskDispatchApi api{1,sizeof(ServerGruntTaskDispatchApi),&route,&execute};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerGruntTaskDispatchApi=_SiN_GetServerGruntTaskDispatchApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerGruntTaskDispatchApi* __cdecl
SiN_GetServerGruntTaskDispatchApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerGruntTaskDispatchApi)?&api:nullptr;
}
