#pragma once
#include "native_server_startup.h"
#include "native_server_dynamic_resupply.h"
namespace sin_port::native_level {
inline constexpr char export_name[]="SiN_GetServerLevelApi";
inline constexpr char level_export[]="SiN_InvokeServerLevelInitBoundary",activate_export[]="SiN_InvokeServerActivateBoundary";
struct ChapterSpec {const char* map;const char* title;};
inline constexpr ChapterSpec chapters[]{
    {"SE1_intro01","#SE1_Chapter1_Title"},{"SE1_intro02","#SE1_Chapter1_Title"},{"SE1_intro03","#SE1_Chapter1_Title"},
    {"SE1_docks01","#SE1_Chapter2_Title"},{"SE1_docks02","#SE1_Chapter2_Title"},{"SE1_docks03","#SE1_Chapter2_Title"},
    {"SE1_docks04","#SE1_Chapter2_Title"},{"SE1_docks05","#SE1_Chapter2_Title"},
    {"SE1_U4Lab01","#SE1_Chapter3_Title"},{"SE1_U4Lab02","#SE1_Chapter3_Title"},{"SE1_U4Lab03","#SE1_Chapter3_Title"},
    {"SE1_U4Lab04","#SE1_Chapter3_Title"},{"SE1_U4Lab05","#SE1_Chapter3_Title"},
    {"SE1_pit01","#SE1_Chapter4_Title"},{"SE1_pit02","#SE1_Chapter4_Title"},
    {"SE1_highrise01","#SE1_Chapter5_Title"},{"SE1_highrise02","#SE1_Chapter5_Title"},{"SE1_highrise03","#SE1_Chapter5_Title"},
    {"SE1_finale01","#SE1_Chapter6_Title"},{"SE1_finale02","#SE1_Chapter6_Title"}
};
struct ChapterBindings {
    void* context;
    int (__cdecl *game_argument)(void*,const char*,const char*,const char**) noexcept;
    int (__cdecl *current)(void*,const char**) noexcept;
    int (__cdecl *set)(void*,const char*) noexcept;
};
struct ChapterResult {
    unsigned completed,matched,match_index,prefix_found,current_read,changed;
    char title[64],prefix[64],chapter[32];
};
using Startup=native_server_startup::State;
struct LevelArgs {const char* map;const char* entities;const char* old_level;const char* landmark;bool load_game,background;};
struct State {
    Startup* startup;
    bool* transition; // same byte used by native GameFrame
    bool* restore_finalize; // 225721f4
    int* load_kind; // original globals+38
    const char* const* current_map; // live globals+2c resolved by owner
    void* transition_vector;int transition_capacity,transition_grow,transition_count;
    void* transition_cache;
    unsigned* map_pair_words; // 224e2b34..40
    bool busy=false,level_ready=false,activation_attempted=false,activated=false,failed=false;
};
struct Bindings {
    void* context;
    int (__cdecl *scope)(void*,const char*,bool) noexcept;
    int (__cdecl *pre_level)(void*) noexcept;
    int (__cdecl *chapter)(void*,const char*) noexcept;
    int (__cdecl *filename)(void*,char*,unsigned) noexcept;
    int (__cdecl *map_event)(void*,int,const char*,const char*) noexcept;
    int (__cdecl *transition_warning)(void*) noexcept;
    int (__cdecl *cleanup)(void*) noexcept;
    int (__cdecl *release)(void*,void*) noexcept;
    int (__cdecl *load_saved)(void*,const char*,bool,bool*) noexcept;
    int (__cdecl *parse_entities)(void*,const char*,void*,bool) noexcept;
    int (__cdecl *restore_transition)(void*,const char*,const char*) noexcept;
    int (__cdecl *finalize_restore)(void*) noexcept;
    int (__cdecl *autosave_enabled)(void*,bool*) noexcept;
    int (__cdecl *find_autosave)(void*,void**) noexcept;
    int (__cdecl *queue_direct)(void*,void*,const char*,float,void*,void*,int) noexcept;
    int (__cdecl *post_level)(void*) noexcept;
    int (__cdecl *motd)(void*) noexcept;
    // Activation traverses live linked entity list, not active snapshot.
    int (__cdecl *delete_queue_count)(void*,unsigned*) noexcept;
    int (__cdecl *delete_warning)(void*) noexcept;
    int (__cdecl *next_entity)(void*,void*,void**) noexcept;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *entity_scope)(void*,bool) noexcept;
    int (__cdecl *activate)(void*,void*) noexcept;
    int (__cdecl *system_activate)(void*,void*) noexcept;
    int (__cdecl *set_activation_flag)(void*,int) noexcept;
    int (__cdecl *local_player)(void*,void**) noexcept;
    int (__cdecl *health_event)(void*,void*,int,int) noexcept;
    int (__cdecl *activation_cvar)(void*,bool*) noexcept;
    int (__cdecl *activation_cvar_set)(void*,int) noexcept;
    int (__cdecl *manager_activate)(void*) noexcept;
    const native_resupply::LifecycleBindings* resupply;
    const ChapterBindings* native_chapter;
};
struct Result {
    unsigned completed,scope_entered,scope_closed,pre_level,chapter,map_event,transition_reset,transition_released,
        transition_warning,cleanup,load_called,load_success,parsed,restored,finalized,autosave_found,
        queued,post_level,motd,reset_words,activation_skipped,delete_warning,visited,skipped,activated,
        resupply_activated,systems,entity_scopes,entity_closes,activation_flag,health_event,cvar_set,manager;
    int descriptor,load_kind;
    native_resupply::LifecycleResult resupply;
    ChapterResult chapter_result;
};
struct Api {
    std::uint32_t version,size;
    int (__cdecl *level_init)(State*,const LevelArgs*,const Bindings*,Result*) noexcept;
    int (__cdecl *server_activate)(State*,const Bindings*,Result*) noexcept;
    int (__cdecl *unlock_chapter)(const char*,const ChapterBindings*,ChapterResult*) noexcept;
};
using GetApi=const Api* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
using InvokeLevel=int (__cdecl *)(State*,const LevelArgs*,const Bindings*,Result*) noexcept;
using InvokeActivate=int (__cdecl *)(State*,void*,int,int,const Bindings*,Result*) noexcept;
}
