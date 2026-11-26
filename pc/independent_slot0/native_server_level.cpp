#include "native_server_level.h"
#include <cstring>
extern "C" const sin_port::native_resupply::Api* __cdecl SiN_GetServerDynamicResupplyApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_level {
namespace {
template<class T>T read(const void* a,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(a)+off,sizeof(v));return v;}
bool ready(State* s,const Bindings* b,Result* o) noexcept {
    return s&&b&&o&&s->startup&&s->startup->game_init_complete&&!s->busy&&!s->failed&&
        s->transition&&s->restore_finalize&&s->load_kind&&s->current_map&&s->map_pair_words;
}
bool stage(const Bindings& b,int (__cdecl *fn)(void*) noexcept,unsigned& n) noexcept {
    if(!fn||!fn(b.context))return false;++n;return true;
}
bool transition_reset(State& s,const Bindings& b,Result& o) noexcept {
    if(*s.transition){
        if(!stage(b,b.transition_warning,o.transition_warning)||!stage(b,b.cleanup,o.cleanup))return false;
    }
    s.transition_count=0;
    if(s.transition_grow!=-1){
        if(s.transition_vector){if(!b.release||!b.release(b.context,s.transition_vector))return false;s.transition_vector=nullptr;o.transition_released=1;}
        s.transition_capacity=0;
    }
    s.transition_cache=s.transition_vector;*s.transition=true;o.transition_reset=1;return true;
}
int __cdecl level_init(State* s,const LevelArgs* a,const Bindings* b,Result* o) noexcept {
    if(!o)return 0;*o={};
    if(!ready(s,b,o)||!a||!a->map||s->level_ready||!b->scope)return 0;
    s->busy=true;
    constexpr char scope_name[]="CServerGameDLL::LevelInit";
    if(!b->scope(b->context,scope_name,true)){s->busy=false;return 0;}o->scope_entered=1;
    auto finish=[&](bool ok){const bool closed=b->scope(b->context,scope_name,false)!=0;o->scope_closed=closed;s->busy=false;s->failed=!(ok&&closed);s->level_ready=o->completed=ok&&closed;return int(o->completed);};
    if(!stage(*b,b->pre_level,o->pre_level)||!b->chapter||!b->chapter(b->context,a->map))return finish(false);o->chapter=1;
    *s->load_kind=a->load_game?(a->old_level?2:1):(a->background?3:0);
    constexpr int descriptors[]{1,2,4,3};o->load_kind=*s->load_kind;o->descriptor=descriptors[o->load_kind];
    char filename[260]{};
    if(!b->filename||!b->filename(b->context,filename,sizeof(filename))||!std::memchr(filename,0,sizeof(filename))||
       !b->map_event||!b->map_event(b->context,o->descriptor,*s->current_map?*s->current_map:"",filename))return finish(false);o->map_event=1;
    auto parse=[&](){if(!b->parse_entities||!b->parse_entities(b->context,a->entities,nullptr,false))return false;o->parsed=1;return true;};
    if(a->load_game){
        if(!transition_reset(*s,*b,*o))return finish(false);
        bool loaded=false;
        if(!b->load_saved||!b->load_saved(b->context,a->map,true,&loaded))return finish(false);
        o->load_called=1;o->load_success=loaded;
        if(!loaded&&!parse())return finish(false);
        if(a->old_level){if(!b->restore_transition||!b->restore_transition(b->context,a->old_level,a->landmark))return finish(false);o->restored=1;}
        if(*s->restore_finalize&&!stage(*b,b->finalize_restore,o->finalized))return finish(false);
        if(a->old_level){
            bool enabled=false;if(!b->autosave_enabled||!b->autosave_enabled(b->context,&enabled))return finish(false);
            if(enabled){
                void* target=nullptr;if(!b->find_autosave||!b->find_autosave(b->context,&target))return finish(false);
                if(target){
                    o->autosave_found=1;
                    if(!b->queue_direct||!b->queue_direct(b->context,target,"Save",1.f,nullptr,nullptr,0))return finish(false);++o->queued;
                    if(!b->queue_direct(b->context,target,"Kill",1.1f,nullptr,nullptr,0))return finish(false);++o->queued;
                }
            }
        }
    }else if(!parse())return finish(false);
    if(!stage(*b,b->post_level,o->post_level)||!stage(*b,b->motd,o->motd)||!stage(*b,b->cleanup,o->cleanup))return finish(false);
    s->map_pair_words[0]=s->map_pair_words[2]=0;s->map_pair_words[1]=s->map_pair_words[3]=0xffffffffu;
    *s->restore_finalize=false;o->reset_words=1;
    return finish(true);
}
int __cdecl server_activate(State* s,const Bindings* b,Result* o) noexcept {
    if(!o)return 0;*o={};
    if(!ready(s,b,o)||!s->level_ready||s->activation_attempted)return 0;
    // Original transition guard returns before any callbacks.
    if(*s->transition){o->activation_skipped=o->completed=1;return 1;}
    s->activation_attempted=s->busy=true;
    auto finish=[&](bool ok){s->busy=false;s->failed=!ok;s->activated=o->completed=ok;return int(ok);};
    unsigned pending=0;
    if(!b->delete_queue_count||!b->delete_queue_count(b->context,&pending))return finish(false);
    if(pending&&!stage(*b,b->delete_warning,o->delete_warning))return finish(false);
    if(!b->next_entity||!b->extent||!b->entity_scope)return finish(false);
    void* current=nullptr;
    for(unsigned i=0;;++i){
        void* next=nullptr;if(!b->next_entity(b->context,current,&next))return finish(false);
        if(!next)break;if(i>=4096||next==current)return finish(false);current=next;++o->visited;
        unsigned n=0;if(!b->extent(b->context,current,&n)||n<0xbc)return finish(false);
        if(read<unsigned>(current,0xb8)&2u){++o->skipped;continue;}
        if(!b->entity_scope(b->context,true))return finish(false);++o->entity_scopes;
        bool ok=false;
        if(read<unsigned>(current,0)==native_resupply::resupply_vtable&&b->resupply&&n>=0x3b0){
            const auto* api=SiN_GetServerDynamicResupplyApi(1,sizeof(native_resupply::Api));
            ok=api&&api->activate(current,n,b->resupply,&o->resupply);if(ok)++o->resupply_activated;
        }else ok=b->activate&&b->activate(b->context,current);
        const bool closed=b->entity_scope(b->context,false)!=0;if(closed)++o->entity_closes;
        if(!ok||!closed)return finish(false);++o->activated;
    }
    const unsigned count=s->startup->registry.count; // original snapshot count, live slots
    if(count>native_server_startup::registry_limit)return finish(false);
    for(unsigned i=0;i<count;++i){
        if(!b->entity_scope(b->context,true))return finish(false);++o->entity_scopes;
        bool ok=b->system_activate&&b->system_activate(b->context,s->startup->registry.systems[i]);
        const bool closed=b->entity_scope(b->context,false)!=0;if(closed)++o->entity_closes;
        if(!ok||!closed)return finish(false);++o->systems;
    }
    if(!b->set_activation_flag||!b->set_activation_flag(b->context,0))return finish(false);o->activation_flag=1;
    if(*s->load_kind!=3){if(!b->map_event||!b->map_event(b->context,7,*s->current_map?*s->current_map:"",""))return finish(false);o->map_event=1;}
    void* player=nullptr;if(!b->local_player||!b->local_player(b->context,&player))return finish(false);
    if(player){
        unsigned n=0;if(!b->extent(b->context,player,&n)||n<0xa0||!b->health_event||!b->health_event(b->context,player,21,read<int>(player,0x9c)))return finish(false);o->health_event=1;
    }
    bool enabled=false;if(!b->activation_cvar||!b->activation_cvar(b->context,&enabled))return finish(false);
    if(!enabled){if(!b->activation_cvar_set||!b->activation_cvar_set(b->context,0))return finish(false);o->cvar_set=1;}
    if(!stage(*b,b->manager_activate,o->manager))return finish(false);
    return finish(true);
}
const Api api{1,sizeof(Api),&level_init,&server_activate};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerLevelApi=_SiN_GetServerLevelApi")
extern "C" __declspec(dllexport) const sin_port::native_level::Api* __cdecl SiN_GetServerLevelApi(std::uint32_t v,std::uint32_t n) noexcept {
    using namespace sin_port::native_level;return v==1&&n==sizeof(Api)?&api:nullptr;
}
