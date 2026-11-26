#include "native_server_startup.h"
#include <cstring>
namespace sin_port::native_server_startup {
namespace {
int __cdecl append(Registry* r,void* system) noexcept {
    if(!r||!system||r->count>=registry_limit)return 0;
    r->systems[r->count++]=system;return 1;
}
int __cdecl register_auto(Registry* r,Pending* node) noexcept {
    if(!r||!node||!node->object||r->count>registry_limit)return 0;
    if(r->initialized)return append(r,node->object);
    // Reject malformed/cyclic pending list before mutating.
    unsigned n=0;for(auto* p=r->pending;p;p=p->next)if(p==node||++n>=registry_limit)return 0;
    node->next=r->pending;r->pending=node;return 1;
}
int __cdecl initialize(Registry* r,const RegistryBindings* b,RegistryResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!r||!b||!b->init||r->draining||r->initialized||r->count>registry_limit)return 0;
    r->draining=true;
    auto finish=[&](bool ok){r->draining=false;o->completed=ok;return int(ok);};
    for(auto* p=r->pending;p;p=p->next){
        if(++o->visited>registry_limit||!p->object)return finish(false);
        o->failed=p->object;bool found=false;
        for(unsigned i=0;i<r->count;++i)if(r->systems[i]==p->object){found=true;break;}
        if(found){if(!b->duplicate||!b->duplicate(b->context,p->object))return finish(false);++o->duplicates;}
        else{if(!append(r,p->object))return finish(false);++o->appended;}
    }
    r->pending=nullptr;r->initialized=true;o->initialized=1;
    // Original loop reads live count after each callback; late auto
    // registration appends directly and participates in the same pass.
    for(unsigned i=0;i<r->count;++i){
        if(i>=registry_limit||r->count>registry_limit||!r->systems[i])return finish(false);
        bool al=false;o->failed=r->systems[i];++o->init_calls;
        if(!b->init(b->context,o->failed,&al)||!al)return finish(false);
    }
    o->failed=nullptr;return finish(true);
}
int __cdecl post_connect(State* s,const Bindings* b,Result* o) noexcept {
    if(!o)return 0;*o={};
    if(!s||!b||s->attempted||s->busy||s->registry.initialized||!b->event_manager||
        !b->cvar_interface||!b->console||!b->get_system||!b->register_external||!b->registry.init||
        !b->scene_repository||!b->model_sounds_repository||!b->allocate_manager||
        !b->construct_manager||!b->engine_complete)return 0;
    s->busy=s->attempted=true;s->engine_server=b->engine_server;
    auto finish=[&](bool ok){s->busy=false;s->complete=ok;o->completed=ok;return int(ok);};
    o->stage=Stage::PostConnect;
    if(!s->post.Initialize(b->globals,b->engine,b->physics,b->filesystem))return finish(false);
    o->post_connect=1;o->stage=Stage::InitialEvents;
    if(!s->post.LoadInitialEvents(b->event_manager,o->initial_events))return finish(false);
    o->stage=Stage::Console;
    if(!b->console(b->context,b->cvar_interface,&s->developer,&s->cheats))return finish(false);o->console_called=1;
    o->stage=Stage::Cheats;if(!s->cheats)return finish(false);
    o->stage=Stage::ExternalService;
    for(const auto& spec:external_services){
        void* p=nullptr;if(!b->get_system(b->context,&spec,&p)||!p||!b->register_external(b->context,p))return finish(false);
        ++o->external_registered;
    }
    o->stage=Stage::SystemGetter;
    for(const auto& spec:explicit_systems){
        void* p=nullptr;if(!b->get_system(b->context,&spec,&p)||!append(&s->registry,p))return finish(false);
        ++o->systems_appended;
    }
    o->stage=Stage::ModEvents;
    auto** table=*reinterpret_cast<void***>(b->event_manager);
    if(!table||!table[1])return finish(false);
    o->mod_events=reinterpret_cast<sin_independent::LoadEventsFromFile>(table[1])(b->event_manager,"resource/ModEvents.res");
    o->stage=Stage::InitializeSystems;
    if(!initialize(&s->registry,&b->registry,&o->registry))return finish(false);
    o->stage=Stage::SceneRepository;o->scene_called=1;
    if(!b->scene_repository(b->context,&o->scene_ok)||!o->scene_ok)return finish(false);
    o->stage=Stage::ModelSoundsRepository;o->model_sounds_called=1;
    if(!b->model_sounds_repository(b->context,&o->model_sounds_ok)||!o->model_sounds_ok)return finish(false);
    o->stage=Stage::DebugOverlay;
    s->debug_overlay=b->engine("VDebugOverlay003",nullptr);o->debug_queried=1;
    o->stage=Stage::ManagerAllocate;
    if(!b->allocate_manager(b->context,0x510,&s->partial_allocation))return finish(false);
    if(s->partial_allocation){
        o->manager_allocated=1;o->stage=Stage::ManagerConstruct;
        if(!b->construct_manager(b->context,s->partial_allocation,&s->manager)||!s->manager)return finish(false);
        o->manager_constructed=1;
    }
    // Original null allocation still reaches engine +4 and returns true.
    o->stage=Stage::EngineComplete;
    if(!b->engine_complete(b->context))return finish(false);o->engine_completed=1;
    o->stage=Stage::Done;return finish(true);
}
int __cdecl game_init(State* s,const GameInitBindings* b,GameInitResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!s||!s->complete||s->busy||s->game_init_attempted||!s->engine_server||!b||!b->reset||!b->release)return 0;
    auto& q=*b->reset;
    if(q.index_capacity>q.index_bytes/12u||(q.index_capacity&&!q.index_records)||
       q.allocation_count>q.allocation_capacity||(q.allocation_count&&!q.allocations)||
       !q.init_flags||!q.default_record)return 0;
    auto** table=*reinterpret_cast<void***>(s->engine_server);
    if(!table||!table[36]||!table[37])return 0;
    s->game_init_attempted=s->busy=true;
    auto finish=[&](bool ok){s->busy=false;s->game_init_complete=ok;o->completed=ok;return int(ok);};
    // Original vector: externally allocated storage (grow == -1) survives.
    q.vector_count=0;
    if(q.vector_grow!=-1){
        if(q.vector){if(!b->release(b->context,q.vector))return finish(false);q.vector=nullptr;o->vector_released=1;}
        q.vector_capacity=0;
    }
    q.vector_cache=q.vector;
    if(q.index_capacity&&!(*q.init_flags&1)){
        *q.init_flags|=1;q.default_record[0]=q.default_record[1]=q.default_record[2]=0xffff;
        q.default_record[3]=1;o->default_initialized=1;
    }
    std::uint16_t previous=0xffff;
    for(unsigned i=q.index_capacity;i>0;){
        --i;auto* record=q.index_records+12u*i;auto index=static_cast<std::uint16_t>(i);
        std::memcpy(record,&index,2);std::memcpy(record+2,&previous,2);previous=index;
    }
    q.index_first=0xffff;q.index_used=0;q.index_free=q.index_capacity?0:0xffff;o->index_rebuilt=1;
    for(unsigned i=0;i<q.allocation_count;++i){
        if(i>=q.allocation_capacity||!b->release(b->context,q.allocations[i]))return finish(false);
        ++o->allocations_released;
    }
    q.allocation_count=0;
    using Queue=void (__thiscall *)(void*,const char*);
    using Execute=void (__thiscall *)(void*);
    reinterpret_cast<Queue>(table[36])(s->engine_server,"exec game.cfg\n");o->command_queued=1;
    reinterpret_cast<Execute>(table[37])(s->engine_server);o->command_executed=1;
    return finish(true);
}
const Api api{1,sizeof(Api),&register_auto,&append,&initialize,&post_connect,&game_init};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerStartupApi=_SiN_GetServerStartupApi")
extern "C" __declspec(dllexport) const sin_port::native_server_startup::Api* __cdecl SiN_GetServerStartupApi(std::uint32_t v,std::uint32_t n) noexcept {
    using namespace sin_port::native_server_startup;return v==1&&n==sizeof(Api)?&api:nullptr;
}
