#include "native_server_game_frame.h"
#include <cmath>
#include <cstring>
extern "C" const sin_port::native_weapon_tables::ServerResupplyPrimaryThinkApi* __cdecl SiN_GetServerResupplyPrimaryThinkApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerAntigenPrimaryThinkApi* __cdecl SiN_GetServerAntigenPrimaryThinkApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerEntityEventServiceApi* __cdecl SiN_GetServerEntityEventServiceApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_game_frame {
namespace {
template<class T>T read(const void* a,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(a)+off,sizeof(v));return v;}
bool scope(const Bindings& b,Result& o,const char* name,bool enter) noexcept {
    if(!b.scope||!b.scope(b.context,name,enter))return false;
    if(enter)++o.scopes_entered;else ++o.scopes_closed;return true;
}
bool stage(const Bindings& b,int (__cdecl *fn)(void*) noexcept,unsigned& count) noexcept {
    if(!fn||!fn(b.context))return false;++count;return true;
}
bool actor(void* a,const State& s,const Bindings& b,Result& o) noexcept {
    if(!a){++o.nulls;return true;}++o.visited;
    unsigned bytes=0;
    if(!b.extent||!b.extent(b.context,a,&bytes)||bytes<0x170)return false;
    // 2221c1c0: actors with an edict have additional simulation and
    // prediction logic. Do not silently substitute default think.
    if(read<unsigned>(a,0x16c)){if(!b.edict_simulate||!b.edict_simulate(b.context,a))return false;++o.edict_calls;return true;}
    using namespace native_resupply;
    const auto vt=read<unsigned>(a,0);
    if(vt==resupply_vtable&&b.resupply&&bytes>=0x3b0){
        const auto* api=SiN_GetServerResupplyPrimaryThinkApi(1,sizeof(ServerResupplyPrimaryThinkApi));
        if(!api||!api->run(a,bytes,read<unsigned>(a,0x378),s.globals->tick,s.globals->interval,b.resupply,&o.resupply))return false;
        ++o.native_resupply;return true;
    }
    const bool antigen_class=(vt==antigen_vtable&&bytes>=antigen_extent)||
        (vt==antigen_canister_vtable&&bytes>=antigen_canister_extent)||
        (vt==antigen_gas_vtable&&bytes>=antigen_gas_extent&&read<int>(a,0x398)==1);
    if(antigen_class&&b.antigen){
        const auto* api=SiN_GetServerAntigenPrimaryThinkApi(1,sizeof(ServerAntigenPrimaryThinkApi));
        if(!api||!api->run(a,bytes,read<unsigned>(a,0x378),s.globals->tick,s.globals->interval,b.antigen,&o.antigen))return false;
        ++o.native_antigen;return true;
    }
    if(!b.other_think||!b.other_think(b.context,a))return false;++o.foreign_thinks;return true;
}
bool actors(bool simulating,const State& s,const Bindings& b,Result& o) noexcept {
    constexpr char name[]="Physics_RunThinkFunctions";
    if(!scope(b,o,name,true))return false;
    *s.profile_enabled=*s.think_profile!=0;o.profile_written=1;
    auto& g=*s.globals;const float saved=g.curtime;o.actor_curtime=saved;
    bool ok=stage(b,b.cleanup,o.cleanup_calls);
    auto run_one=[&](void* a){if(!a){++o.nulls;return true;}g.curtime=saved;return actor(a,s,b,o);};
    if(ok&&!simulating){
        // Original loop is strictly index < live maxClients (not <=).
        for(int i=1;i<g.max_clients&&ok;++i){
            if(i>=4096){ok=false;break;}void* a=nullptr;
            if(!b.client||!b.client(b.context,i,&a)){ok=false;break;}
            ++o.client_queries;ok=run_one(a);
        }
    }else if(ok){
        // 22285660/70 increment/decrement depth around a stable
        // snapshot of active pointers. Owner supplies separate table.
        ++*s.iteration_depth;o.iteration_entered=1;
        ActiveList list{};
        ok=b.active&&b.active(b.context,&list)&&list.count<=list.capacity&&list.count<=4096&&list.slot_count<=4096&&
            (!list.count||(list.indices&&list.slots));
        void* snapshot[4096]{};
        if(ok)for(unsigned i=0;i<list.count;++i){
            if(list.indices[i]>=list.slot_count){ok=false;break;}
            snapshot[i]=list.slots[list.indices[i]];
        }
        if(ok){o.snapshot_count=list.count;for(unsigned i=0;i<list.count&&ok;++i)ok=run_one(snapshot[i]);}
        --*s.iteration_depth;o.iteration_closed=1;
    }
    g.curtime=saved;o.curtime_restored=1;
    const bool closed=scope(b,o,name,false);return ok&&closed;
}
bool events(const State& s,const Bindings& b,Result& o) noexcept {
    constexpr char name[]="ServiceEventQueue()";
    if(!scope(b,o,name,true))return false;
    bool ok=false;
    if(b.queue){
        const auto& q=*b.queue;
        if(q.bindings&&q.steps&&q.single_step){
            const auto* api=SiN_GetServerEntityEventServiceApi(1,sizeof(ServerEntityEventServiceApi));
            auto bindings=*q.bindings;bindings.live_now=&s.globals->curtime;bindings.live_single_step=q.single_step;
            ok=api&&api->service(q.sentinel,q.bytes,q.token,s.globals->curtime,*q.single_step!=0,q.steps,&bindings,&o.events);
            if(ok)o.native_events=1;
        }
    }else ok=b.event_service&&b.event_service(b.context);
    if(ok)o.event_called=1;
    const bool closed=scope(b,o,name,false);return ok&&closed;
}
int __cdecl run(bool simulating,const State* s,const Bindings* b,Result* o) noexcept {
    if(!o)return 0;*o={};
    if(!s||!b||!s->globals||!s->level_transition||!s->skip_next||!s->double_step||
       !s->think_profile||!s->profile_enabled||!s->iteration_depth)return 0;
    auto& g=*s->globals;
    if(!std::isfinite(g.curtime)||!std::isfinite(g.frametime)||!std::isfinite(g.interval)||g.interval<=0)return 0;
    constexpr char name[]="CServerGameDLL::GameFrame";
    if(!scope(*b,*o,name,true))return 0;
    auto finish=[&](bool ok){const bool closed=scope(*b,*o,name,false);o->completed=ok&&closed;return int(o->completed);};
    if(*s->level_transition){o->transition=1;return finish(true);}
    if(*s->skip_next){*s->skip_next=false;o->skip_consumed=1;return finish(true);}
    o->original_frametime=g.frametime;
    if(g.max_clients==1&&*s->double_step){*s->skip_next=true;g.frametime=float(double(g.frametime)*2.);o->double_step=1;}
    o->simulation_frametime=g.frametime;
    bool ok=std::isfinite(g.frametime)&&stage(*b,b->cleanup,o->cleanup_calls)&&
        stage(*b,b->pre_systems,o->pre_systems)&&stage(*b,b->pre_actors,o->pre_actors)&&
        stage(*b,b->manager,o->manager)&&stage(*b,b->engine_begin,o->engine_begin)&&
        actors(simulating,*s,*b,*o)&&stage(*b,b->post_systems,o->post_systems)&&events(*s,*b,*o)&&
        stage(*b,b->cleanup,o->cleanup_calls)&&stage(*b,b->post_cleanup,o->post_cleanup);
    if(ok&&b->rules_frame)ok=stage(*b,b->rules_frame,o->rules_called);
    if(ok&&b->trace_report)ok=stage(*b,b->trace_report,o->trace_report);
    if(ok)ok=stage(*b,b->frame_tail,o->tail);
    // On host failure the adapter restores time and scope; unlike a
    // successful frame it must not be blindly replayed after partial effects.
    g.frametime=o->original_frametime;o->frametime_restored=1;
    return finish(ok);
}
const Api api{1,sizeof(Api),&run};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerGameFrameApi=_SiN_GetServerGameFrameApi")
extern "C" __declspec(dllexport) const sin_port::native_game_frame::Api* __cdecl SiN_GetServerGameFrameApi(std::uint32_t v,std::uint32_t n) noexcept {
    using namespace sin_port::native_game_frame;return v==1&&n==sizeof(Api)?&api:nullptr;
}
