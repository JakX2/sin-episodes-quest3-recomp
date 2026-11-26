#include "native_server_level.h"
#include "server_fixed_difficulty_service.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
extern "C" const sin_port::native_resupply::Api* __cdecl SiN_GetServerDynamicResupplyApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerEntityNamedEventQueueApi* __cdecl SiN_GetServerEntityNamedEventQueueApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" const sin_port::native_map::Api* __cdecl SiN_GetServerMapEntityApi(std::uint32_t,std::uint32_t) noexcept;
int __cdecl SinParseMapWithEventsInternal(const char*,unsigned,bool,const sin_port::native_map::PipelineBindings*,
    sin_port::native_map::PipelineResult*,void*) noexcept;
int __cdecl SinPublishMapEventInternal(void*,int,const char*,const char*) noexcept;
int __cdecl SinPublishActivationHealthInternal(void*,const void*,unsigned,int) noexcept;
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
int __cdecl unlock_chapter(const char* map,const ChapterBindings* b,ChapterResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!map||!b)return 0;
    auto done=[&](){o->completed=1;return 1;};
    auto fold=[](unsigned char v){return v>='A'&&v<='Z'?v+32:v;};
    const ChapterSpec* match=nullptr;
    for(unsigned i=0;i<20;++i){
        const char* p=map;const char* q=chapters[i].map;
        while(*q&&*p&&fold(*p)==fold(*q)){++p;++q;}
        if(!*q){match=&chapters[i];o->matched=1;o->match_index=i;break;}
    }
    if(!match)return done();
    std::snprintf(o->title,sizeof(o->title),"%s",match->title);
    for(char& v:o->title){if(v=='\r'||v=='\n')v=0;}
    const char* game=nullptr;
    if(!b->game_argument||!b->game_argument(b->context,"-game","se1",&game)||!game)return 0;
    std::snprintf(o->prefix,sizeof(o->prefix),"#%s_chapter",game);
    const char* number=std::strstr(o->title,o->prefix);
    if(!number)return done(); // preserve original case-sensitive strstr
    o->prefix_found=1;
    std::snprintf(o->chapter,sizeof(o->chapter),"%s",number+std::strlen(o->prefix));
    if(char* tail=std::strstr(o->chapter,"_title"))*tail=0;
    const char* current=nullptr;
    if(!b->current||!b->current(b->context,&current))return 0;
    if(!current)current="";o->current_read=1;
    const int old_value=std::atoi(current),new_value=std::atoi(o->chapter);
    bool less=old_value<new_value;
    if(old_value==new_value){
        const char* p=current;const char* q=o->chapter;
        while(*p&&*q&&fold(*p)==fold(*q)){++p;++q;}
        less=fold(*p)<fold(*q);
    }
    if(less){if(!b->set||!b->set(b->context,o->chapter))return 0;o->changed=1;}
    return done();
}
bool queue_autosave(const Bindings& b,Result& o,void* target,const char* input,float delay,native_weapon_tables::ServerEntityNamedEventQueueResult& result) noexcept {
    if(b.queue_direct){if(!b.queue_direct(b.context,target,input,delay,nullptr,nullptr,0))return false;}
    else{
        const auto* q=b.native_queue;
        if(!q||!q->now||!q->queue||!q->intern||!b.extent||q->entry_count>sin_independent::kEntityHandleCapacity||(!q->entries&&q->entry_count))return false;
        unsigned n=0;if(!b.extent(b.context,target,&n)||n<0x37c)return false;
        const unsigned handle=read<unsigned>(target,0x378);
        if(sin_independent::EntityHandleView(q->entries,q->entry_count).Resolve(handle)!=target)return false;
        unsigned token=0;if(!q->intern(q->context,input,&token)||!token)return false;
        unsigned variant[5]{0,0,0,0xffffffffu,0};
        using namespace native_weapon_tables;
        const auto* api=SiN_GetServerEntityNamedEventQueueApi(1,sizeof(ServerEntityNamedEventQueueApi));
        if(!api||!api->enqueue_direct(q->sentinel,q->bytes,q->token,*q->now,handle,token,0xffffffffu,0xffffffffu,variant,sizeof(variant),delay,0,q->queue,&result))return false;
    }
    ++o.queued;return true;
}
bool event_completed(int code) noexcept {
    using namespace fixed_service;
    return code==EventFired||code==EventUnavailable||code==EventFireReturnedFalse;
}
bool map_event(const Bindings& b,void* manager,int descriptor,const char* map,const char* filename) noexcept {
    if(manager)return event_completed(SinPublishMapEventInternal(manager,descriptor,map,filename));
    return b.map_event&&b.map_event(b.context,descriptor,map,filename);
}
int level_init_impl(State* s,const LevelArgs* a,const Bindings* b,Result* o,void* event_manager) noexcept {
    if(!o)return 0;*o={};
    if(!ready(s,b,o)||!a||!a->map||s->level_ready||!b->scope)return 0;
    s->busy=true;
    constexpr char scope_name[]="CServerGameDLL::LevelInit";
    if(!b->scope(b->context,scope_name,true)){s->busy=false;return 0;}o->scope_entered=1;
    auto finish=[&](bool ok){const bool closed=b->scope(b->context,scope_name,false)!=0;o->scope_closed=closed;s->busy=false;s->failed=!(ok&&closed);s->level_ready=o->completed=ok&&closed;return int(o->completed);};
    if(!stage(*b,b->pre_level,o->pre_level))return finish(false);
    if(b->chapter){if(!b->chapter(b->context,a->map))return finish(false);}
    else if(!unlock_chapter(a->map,b->native_chapter,&o->chapter_result))return finish(false);
    o->chapter=1;
    *s->load_kind=a->load_game?(a->old_level?2:1):(a->background?3:0);
    constexpr int descriptors[]{1,2,4,3};o->load_kind=*s->load_kind;o->descriptor=descriptors[o->load_kind];
    char filename[260]{};
    if(!b->filename||!b->filename(b->context,filename,sizeof(filename))||!std::memchr(filename,0,sizeof(filename))||
       !map_event(*b,event_manager,o->descriptor,*s->current_map?*s->current_map:"",filename))return finish(false);o->map_event=1;
    auto parse=[&](){
        if(b->parse_entities){if(!b->parse_entities(b->context,a->entities,nullptr,false))return false;}
        else{
            if(!a->entities||!b->native_map_pipeline||!b->entity_text_bytes)return false;
            const auto* api=SiN_GetServerMapEntityApi(1,sizeof(native_map::Api));
            if(event_manager){
                if(!SinParseMapWithEventsInternal(a->entities,b->entity_text_bytes,false,b->native_map_pipeline,&o->map_pipeline,event_manager))return false;
            }else if(!api||!api->parse_all(a->entities,b->entity_text_bytes,false,b->native_map_pipeline,&o->map_pipeline))return false;
        }
        o->parsed=1;return true;
    };
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
                    if(!queue_autosave(*b,*o,target,"Save",1.f,o->save_event)||!queue_autosave(*b,*o,target,"Kill",1.1f,o->kill_event))return finish(false);
                }
            }
        }
    }else if(!parse())return finish(false);
    if(!stage(*b,b->post_level,o->post_level)||!stage(*b,b->motd,o->motd)||!stage(*b,b->cleanup,o->cleanup))return finish(false);
    s->map_pair_words[0]=s->map_pair_words[2]=0;s->map_pair_words[1]=s->map_pair_words[3]=0xffffffffu;
    *s->restore_finalize=false;o->reset_words=1;
    return finish(true);
}
int __cdecl level_init(State* s,const LevelArgs* a,const Bindings* b,Result* o) noexcept {
    return level_init_impl(s,a,b,o,nullptr);
}
int server_activate_impl(State* s,const Bindings* b,Result* o,void* event_manager) noexcept {
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
    if(*s->load_kind!=3){if(!map_event(*b,event_manager,7,*s->current_map?*s->current_map:"",""))return finish(false);o->map_event=1;}
    void* player=nullptr;if(!b->local_player||!b->local_player(b->context,&player))return finish(false);
    if(player){
        unsigned n=0;if(!b->extent||!b->extent(b->context,player,&n)||n<0xa0)return finish(false);
        const int health=read<int>(player,0x9c);
        if(event_manager){
            if(!event_completed(SinPublishActivationHealthInternal(event_manager,player,n,health)))return finish(false);
        }else if(!b->health_event||!b->health_event(b->context,player,21,health))return finish(false);
        o->health_event=1;
    }
    bool enabled=false;if(!b->activation_cvar||!b->activation_cvar(b->context,&enabled))return finish(false);
    if(!enabled){if(!b->activation_cvar_set||!b->activation_cvar_set(b->context,0))return finish(false);o->cvar_set=1;}
    if(!stage(*b,b->manager_activate,o->manager))return finish(false);
    return finish(true);
}
int __cdecl server_activate(State* s,const Bindings* b,Result* o) noexcept {
    return server_activate_impl(s,b,o,nullptr);
}
const Api api{1,sizeof(Api),&level_init,&server_activate,&unlock_chapter};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerLevelApi=_SiN_GetServerLevelApi")
extern "C" __declspec(dllexport) const sin_port::native_level::Api* __cdecl SiN_GetServerLevelApi(std::uint32_t v,std::uint32_t n) noexcept {
    using namespace sin_port::native_level;return v==1&&n==sizeof(Api)?&api:nullptr;
}

int __cdecl SinLevelInitWithEventsInternal(sin_port::native_level::State* s,const sin_port::native_level::LevelArgs* a,
    const sin_port::native_level::Bindings* b,sin_port::native_level::Result* o,void* manager) noexcept {
    return manager&&b&&!b->parse_entities&&sin_port::native_level::level_init_impl(s,a,b,o,manager);
}

int __cdecl SinServerActivateWithEventsInternal(sin_port::native_level::State* s,const sin_port::native_level::Bindings* b,
    sin_port::native_level::Result* o,void* manager) noexcept {
    return manager&&sin_port::native_level::server_activate_impl(s,b,o,manager);
}
