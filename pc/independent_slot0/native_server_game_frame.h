#pragma once
#include "native_server_npc_active_weapon_step.h"
#include "native_server_map_entity.h"
namespace sin_port::native_game_frame {
using namespace native_weapon_tables;
inline constexpr char export_name[]="SiN_GetServerGameFrameApi";
inline constexpr char boundary_export[]="SiN_InvokeServerGameFrameBoundary";
inline constexpr unsigned original_vtable=0x22442efc,original_slot=4,original_entry=0x221e9080;
// Live CGlobalVars prefix from the original frame and tick routines.
struct Globals {
    float realtime;int framecount;float absoluteframetime,curtime,frametime;
    int max_clients,tick;float interval;
};
static_assert(sizeof(Globals)==0x20&&offsetof(Globals,curtime)==0xc&&offsetof(Globals,frametime)==0x10&&offsetof(Globals,tick)==0x18);
struct State {
    Globals* globals;
    bool* level_transition; // 225721f5
    bool* skip_next; // 22572468
    const int* double_step; // live root +2c, 22559770
    const int* think_profile; // live root +2c, 22574990
    bool* profile_enabled; // 225144c0
    unsigned* iteration_depth; // 22589a34
};
// The original snapshot indexes the separate active-entity table, not
// the serial-handle table. Owner must retain live actor storage during
// the iteration; deferred cleanup is performed after the snapshot.
struct ActiveList {
    const std::uint16_t* indices;
    unsigned count,capacity;
    void* const* slots;
    unsigned slot_count;
};
struct EventQueue {
    void* sentinel;unsigned bytes,token;int* steps;
    const std::uint8_t* single_step;
    const ServerEntityEventServiceBindings* bindings;
};
struct Bindings {
    void* context;
    int (__cdecl *scope)(void*,const char*,bool) noexcept;
    int (__cdecl *cleanup)(void*) noexcept; // 22162aa0
    int (__cdecl *pre_systems)(void*) noexcept; // 221f5660
    int (__cdecl *pre_actors)(void*) noexcept; // 222f0ed0
    int (__cdecl *manager)(void*) noexcept; // 2254b888 +c
    int (__cdecl *engine_begin)(void*) noexcept; // 22572194 +8
    int (__cdecl *active)(void*,ActiveList*) noexcept;
    int (__cdecl *client)(void*,int,void**) noexcept;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *edict_simulate)(void*,void*) noexcept; // complete 2221c1c0 edict branch
    int (__cdecl *other_think)(void*,void*) noexcept; // foreign no-edict actor
    int (__cdecl *post_systems)(void*) noexcept; // 221f5670
    int (__cdecl *event_service)(void*) noexcept; // fallback; native queue wins
    int (__cdecl *post_cleanup)(void*) noexcept; // 222587c0
    int (__cdecl *rules_frame)(void*) noexcept; // optional live rules +6c
    int (__cdecl *trace_report)(void*) noexcept; // optional live debug
    int (__cdecl *frame_tail)(void*) noexcept; // 222848b0
    const ServerResupplyPrimaryThinkBindings* resupply;
    const ServerAntigenPrimaryThinkBindings* antigen;
    const EventQueue* queue;
    // Opt-in edict-backed stationary branch. Engine +14/+18 scope is
    // separate from profiler scopes and must be paired even on failure.
    int (__cdecl *edict_scope)(void*,bool) noexcept;
    const native_map::AutoPrimaryBindings* logic_auto;
    const native_map::TimerPrimaryBindings* logic_timer;
};
struct Result {
    unsigned completed,transition,skip_consumed,double_step,profile_written,cleanup_calls,
        pre_systems,pre_actors,manager,engine_begin,snapshot_count,visited,nulls,client_queries,
        edict_calls,foreign_thinks,native_resupply,native_antigen,post_systems,event_called,
        native_events,post_cleanup,rules_called,trace_report,tail,scopes_entered,scopes_closed,
        iteration_entered,iteration_closed,curtime_restored,frametime_restored;
    unsigned stationary_calls,stationary_deduplicated,edict_scopes_entered,edict_scopes_closed,named_context_fallbacks;
    float original_frametime,simulation_frametime,actor_curtime;
    ServerResupplyPrimaryThinkResult resupply;
    ServerAntigenPrimaryThinkResult antigen;
    ServerEntityEventServiceResult events;
    unsigned native_auto;
    native_map::AutoPrimaryResult logic_auto;
    unsigned native_timer;
    native_map::TimerPrimaryResult logic_timer;
};
struct Api {
    std::uint32_t version,size;
    int (__cdecl *run)(bool,const State*,const Bindings*,Result*) noexcept;
};
using GetApi=const Api* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
// One synchronous call through actual ServerGameDLL003 slot4. No
// persistent binding or admission to retail initialization.
using InvokeBoundary=int (__cdecl *)(bool,const State*,const Bindings*,Result*) noexcept;
}
