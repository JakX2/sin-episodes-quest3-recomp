#pragma once
#include "native_server_grunt_animation.h"
#include "native_server_activity_metadata.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_npc_activity_translate_export[]="SiN_GetServerNpcActivityTranslateApi";
enum ServerNpcActivityClass : unsigned {NpcActivityHosted=0,NpcActivityGrunt=1,NpcActivityJetpack=2};
struct ServerNpcActivityTranslateBindings {
    void* context;
    unsigned actor_class;
    const ServerGruntRegistration* ids;
    ServerGruntActivityBindings grunt;
    ServerJetpackActivityBindings jetpack;
    // Original +33c. Absent weapon returns input and clears required flag.
    int (__cdecl *weapon)(void*,void*,int,bool*,int*) noexcept;
    // +338 only for other, not yet reconstructed actor types.
    int (__cdecl *other)(void*,void*,int,int*) noexcept;
    // 1 resolved view, 0 missing model, negative failure.
    int (__cdecl *metadata)(void*,void*,const ServerActivityCompositeView**) noexcept;
    ServerActivityDictionary* activities;
    ServerEventDictionary* events;
    ServerSequenceChoiceBindings random;
    // Original required-weapon warning/cache is an optional diagnostic leaf.
    void (__cdecl *missing_required)(void*,void*,int) noexcept;
    // Original special activity 15 path, 2204fb20 (scripted sequence).
    int (__cdecl *scripted_sequence)(void*,void*,int*) noexcept;
    // Original missing-sequence warning/cache, optional diagnostic leaf.
    void (__cdecl *missing_sequence)(void*,void*,int) noexcept;
};
struct ServerNpcActivityTranslateResult {
    int activity,initial_weapon,last_npc,final_weapon;
    unsigned iterations,probes,required_missing;
};
struct ServerNpcActivitySelectionResult {
    ServerNpcActivityTranslateResult translation;
    int activity,sequence;
    unsigned selections;
};
struct ServerNpcActivityTranslateApi {
    std::uint32_t version,size;
    // 22049870
    int (__cdecl *translate)(void*,unsigned,int,const ServerNpcActivityTranslateBindings*,ServerNpcActivityTranslateResult*) noexcept;
    // 22049a60: translation then final weighted choice; scripted and run/walk fallback.
    int (__cdecl *select)(void*,unsigned,int,const ServerNpcActivityTranslateBindings*,ServerNpcActivitySelectionResult*) noexcept;
};
using GetServerNpcActivityTranslateApi=const ServerNpcActivityTranslateApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
