#pragma once
#include "native_server_activity_assignment.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_npc_transition_export[]="SiN_GetServerNpcTransitionApi";
struct ServerTransitionEntry {unsigned entry_node,exit_node;bool reversible;};
struct ServerTransitionView {
    const ServerTransitionEntry* sequences;
    unsigned count,node_count;
    const std::uint8_t* matrix; // non-composite, one-based nodes, row-major
    unsigned matrix_bytes;
    bool composite; // original 2211b670 directly returns goal node
};
struct ServerTransitionChoice {
    int sequence,direction;
    unsigned scanned,missing_edge;
};
struct ServerNpcTransitionBindings {
    ServerNpcActivityAssignmentBindings assignment;
    void* context;
    // Resolved global studio sequence nodes, no file loading.
    int (__cdecl *nodes)(void*,void*,const ServerTransitionView**) noexcept;
    void (__cdecl *missing_edge)(void*,void*,int from,int goal) noexcept;
};
struct ServerNpcTransitionResult {
    ServerNpcActivityAssignmentResult assignment;
    ServerNpcActivitySelectionResult pending;
    ServerTransitionChoice transition;
    unsigned skipped,reselected,intermediate,fallback_request;
};
struct ServerNpcTransitionApi {
    std::uint32_t version,size;
    // 220eab40 and 220ed8e0. Direction null in original means forward-only.
    int (__cdecl *choose)(const ServerTransitionView*,int from,int goal,int direction,bool forward_only,ServerTransitionChoice*) noexcept;
    // 22049c20: set pending activity; 2 ignored; 0 dispatches +4dc;
    // 154 stored without recomputing pending sequence.
    int (__cdecl *set_pending)(void*,unsigned,int,const ServerNpcTransitionBindings*,ServerNpcTransitionResult*) noexcept;
    // 22049da0 -> 22049ce0, pending-activity update and assignment.
    int (__cdecl *update)(void*,unsigned,const ServerNpcTransitionBindings*,ServerNpcTransitionResult*) noexcept;
};
using GetServerNpcTransitionApi=const ServerNpcTransitionApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
