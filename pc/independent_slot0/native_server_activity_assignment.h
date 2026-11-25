#pragma once
#include "native_server_activity_translate.h"
#include "native_server_sequence_reset.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_npc_activity_assignment_export[]="SiN_GetServerNpcActivityAssignmentApi";
struct ServerNpcActivityAssignmentBindings {
    ServerNpcActivityTranslateBindings translation;
    ServerSequenceResetBindings reset;
    ServerSequenceDictionaryBindings dictionary;
    void* context;
    // 222a5470 resolves active weapon and calls +3d8 if present.
    int (__cdecl *weapon_notify)(void*,void*,std::uint32_t,int,float) noexcept;
    // +5c0, combined actor/weapon capabilities. Used by verified Grunt +630.
    int (__cdecl *capabilities)(void*,void*,unsigned*) noexcept;
    // +908 component virtual +4c, called after publishing requested activity.
    int (__cdecl *component_notify)(void*,void*,std::uint32_t) noexcept;
    // Other classes only: +4e0, +630, +280 and +4e4.
    int (__cdecl *locomotion)(void*,void*,int,bool*) noexcept;
    int (__cdecl *view_vector)(void*,void*,int,float*) noexcept;
    int (__cdecl *view_dirty)(void*,void*) noexcept;
    int (__cdecl *activity_changed)(void*,void*,int) noexcept;
};
struct ServerNpcActivityAssignmentInput {int requested,sequence,translated,weapon_activity;};
struct ServerNpcActivityAssignmentResult {
    ServerNpcActivitySelectionResult selected;
    ServerSequenceDictionaryResult sequence;
    unsigned skipped,assigned,precleared_cycle,weapon_notified,view_changed,activity_changed,component_notified;
};
struct ServerNpcActivityAssignmentApi {
    std::uint32_t version,size;
    // 220417d0, without updating the pending selection fields.
    int (__cdecl *assign)(void*,unsigned,const ServerNpcActivityAssignmentInput*,const ServerNpcActivityAssignmentBindings*,ServerNpcActivityAssignmentResult*) noexcept;
    // 22049b50 and verified Grunt/Jetpack +4dc 22358c30.
    int (__cdecl *request)(void*,unsigned,int,const ServerNpcActivityAssignmentBindings*,ServerNpcActivityAssignmentResult*) noexcept;
};
using GetServerNpcActivityAssignmentApi=const ServerNpcActivityAssignmentApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
