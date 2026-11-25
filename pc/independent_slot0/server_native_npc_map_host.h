#pragma once
#include "native_npc_map.h"
#include "native_npc_dialogue.h"
#include "sin_compat/source_map_runtime.hpp"
#include <map>
#include <cstring>
namespace sin_port::map_host {
namespace npc=sin_port::native_weapon_tables;
using sin_compat::EntityRef;
enum class NpcMapResult {Unhandled,Applied,Rejected,Failed};
class INpcMapShells {
public:
    virtual ~INpcMapShells()=default;
    // The actual engine owns full NPC construction, behavior registration,
    // spawn, activation and destruction; it must invoke the previously
    // recovered native spawn slices at their proper engine boundaries.
    virtual npc::HologramControl* control(EntityRef)=0;
    virtual const npc::NpcMapBindings* bindings(EntityRef)=0;
    virtual bool parse_key(EntityRef,unsigned type,std::string_view,std::uint32_t&)=0;
    virtual const npc::JessicaInteractionBindings* interaction_bindings(EntityRef){return nullptr;}
    virtual const npc::NpcMapDialogueBindings* dialogue_bindings(EntityRef){return nullptr;}
    virtual const npc::TalkManagerRuntimeBindings* manager_bindings(EntityRef){return nullptr;}
    virtual const npc::IntegratedNpcMapDialogueBindings* scene_bindings(EntityRef){return nullptr;}
    virtual const npc::NativeGeneratedResponseBindings* generated_scene_bindings(EntityRef){return nullptr;}
    virtual const npc::ScriptActorSpeechBindings* script_speech_bindings(EntityRef){return nullptr;}
};
class NativeNpcMapHost final:public sin_compat::map::ISourceMapEntityHost {
public:
    explicit NativeNpcMapHost(sin_compat::map::ISourceMapEntityHost& engine):engine_(engine){}
    NativeNpcMapHost(sin_compat::map::ISourceMapEntityHost& engine,INpcMapShells& shells,const npc::NpcMapApi& api,npc::NpcMapSession& session,const npc::JessicaInteractionApi* interaction=nullptr,const npc::NpcMapDialogueApi* dialogue=nullptr,const npc::ManagedJessicaObservationApi* managed=nullptr,const npc::IntegratedNpcMapDialogueApi* integrated=nullptr,const npc::NativeGeneratedMapDialogueApi* generated=nullptr,const npc::ScriptNpcMapDialogueApi* script=nullptr,const npc::ScriptResponseFinderApi* finder=nullptr):
        engine_(engine),shells_(&shells),api_(&api),session_(&session),interaction_(interaction),dialogue_(dialogue),managed_(managed),integrated_(integrated),generated_(generated),script_(script),script_finder_(finder){}
    // The loaded finder API must outlive this host. Explicit close after
    // successful map unload is preferred; destructor covers abandoned startup.
    ~NativeNpcMapHost() override{
        if(script_store_.owner&&script_finder_&&script_finder_->release)
            (void)script_finder_->release(&script_store_);
    }
    NativeNpcMapHost(const NativeNpcMapHost&)=delete;
    NativeNpcMapHost& operator=(const NativeNpcMapHost&)=delete;
    // Engine filesystem supplies the root and include bytes; the loaded
    // DLL copies them into a registry owned by this map host.
    // A single registry shares match-once/group cursors across NPCs.
    int open_script_session(const char* data,unsigned bytes,const npc::ScriptResponseInclude* include=nullptr){
        if(!script_ready()||script_busy_||!entries_.empty()||script_store_.owner||script_store_.system)return 0;
        ScriptGuard guard(script_busy_);
        return script_finder_->create("talker/response_rules.txt",data,bytes,include,&script_store_);
    }
    int close_script_session(){
        if(!script_ready()||script_busy_||!entries_.empty()||!script_store_.owner||!script_store_.system)return 0;
        ScriptGuard guard(script_busy_);
        return script_finder_->release(&script_store_);
    }
    bool script_session_open()const noexcept{return script_store_.owner&&script_store_.system;}
    // Bind a shared scene admission service before opening a registry.
    // The callback context is borrowed (e.g. a persistent SceneCacheOwner)
    // and must outlive the session. Do not clear/rebind its owner while
    // NPCs or this host can use it. Cache persistence across maps is allowed.
    bool bind_script_scene_filter(const npc::ResponseSceneFilterBindings& filter) noexcept{
        if(!script_ready()||script_busy_||!entries_.empty()||script_store_.owner||script_store_.system||
           shared_scene_filter_bound_||!filter.storage||!filter.resolve_name||!filter.metadata)return false;
        shared_scene_filter_=filter;shared_scene_filter_bound_=true;return true;
    }
    bool unbind_script_scene_filter() noexcept{
        if(script_busy_||!entries_.empty()||script_store_.owner||script_store_.system||!shared_scene_filter_bound_)return false;
        shared_scene_filter_={};shared_scene_filter_bound_=false;return true;
    }
    // Shared raw playback resource is independent of metadata admission.
    // The caller owns the cache and must keep it alive until unbound.
    bool bind_script_raw_scene_cache(const npc::NamedSceneCacheBindings& cache) noexcept{
        if(!script_ready()||script_busy_||!entries_.empty()||script_store_.owner||script_store_.system||
           shared_raw_cache_bound_||!cache.ensure||!cache.find||!cache.lock||!cache.reload||!cache.unlock)return false;
        shared_raw_cache_=cache;shared_raw_cache_bound_=true;return true;
    }
    bool unbind_script_raw_scene_cache() noexcept{
        if(script_busy_||!entries_.empty()||script_store_.owner||script_store_.system||!shared_raw_cache_bound_)return false;
        shared_raw_cache_={};shared_raw_cache_bound_=false;return true;
    }
    bool is_live(EntityRef e)const override{return engine_.is_live(e);}
    bool create_entity(std::string_view name,EntityRef& e)override{
        const char* names[]={"npc_jessica","npc_count","npc_generic_actor","npc_jc"};
        unsigned i=0;while(i<4&&name!=names[i])++i;
        if(i<4&&!ready())return false;
        if(!engine_.create_entity(name,e)||!engine_.is_live(e))return false;
        if(i==4)return true;
        const auto kind=static_cast<npc::NpcMapKind>(i);
        if(!control(e,kind))return false;
        return entries_.emplace(e.index,Entry{e.generation,kind,Stage::Keys}).second;
    }
    bool key_value(EntityRef e,std::string_view key,std::string_view value)override{
        auto* a=find(e);if(!a)return engine_.key_value(e,key,value);
        if(a->stage!=Stage::Keys||!engine_.is_live(e))return false;
        const auto kind=a->kind;const auto* f=field(kind,key,npc::NpcMapFieldKind::Key);
        if(!f)return engine_.key_value(e,key,value);
        std::uint32_t word=0;if(!shells_->parse_key(e,f->type,value,word))return false;
        a=find(e);if(!a||a->stage!=Stage::Keys)return false;
        auto* c=control(e,kind);return c&&api_->key(kind,c,f->name,f->type,word)==1;
    }
    bool dispatch_spawn(EntityRef e)override{return transition(e,false);}
    bool activate(EntityRef e)override{return transition(e,true);}
    bool remove_entity(EntityRef e)override{
        if(engine_.is_live(e))(void)engine_.remove_entity(e);
        return notify_deleted(e);
    }
    bool notify_deleted(EntityRef e){
        if(engine_.is_live(e))return false;
        if(find(e))entries_.erase(e.index);
        return true;
    }
    NpcMapResult input(EntityRef e,std::string_view key,unsigned type,std::uint32_t word){
        if(!engine_.is_live(e))return NpcMapResult::Rejected;
        auto* a=find(e);if(!a)return NpcMapResult::Unhandled;
        if(a->stage!=Stage::Active)return NpcMapResult::Rejected;
        const auto kind=a->kind;const auto* f=field(kind,key,npc::NpcMapFieldKind::Input);
        if(!f)return NpcMapResult::Unhandled;
        auto* c=control(e,kind);const auto* b=shells_->bindings(e);
        return c&&b?status(api_->input(kind,c,b,f->name,type,word)):NpcMapResult::Rejected;
    }
    NpcMapResult observe(EntityRef e,const void* record,unsigned bytes){
        if(!engine_.is_live(e))return NpcMapResult::Rejected;
        auto* a=find(e);if(!a||a->kind==npc::NpcMapKind::Jc)return NpcMapResult::Unhandled;
        if(a->stage!=Stage::Active)return NpcMapResult::Rejected;
        const auto kind=a->kind;auto* c=control(e,kind);const auto* b=shells_->bindings(e);
        if(!c||!b)return NpcMapResult::Rejected;
        if(script_&&(kind==npc::NpcMapKind::Jessica||kind==npc::NpcMapKind::Count)){
            if(script_->version!=npc::version||script_->size!=sizeof(*script_)||!script_->observe)return NpcMapResult::Rejected;
            if(script_finder_&&(!script_session_open()||script_busy_))return NpcMapResult::Rejected;
            ScriptGuard guard(script_busy_);
            const auto* d=shells_->dialogue_bindings(e);
            const auto* speech=shells_->script_speech_bindings(e);
            const auto* manager=kind==npc::NpcMapKind::Jessica?shells_->manager_bindings(e):nullptr;
            a=find(e);
            if(!d||!speech||!a||a->kind!=kind||a->stage!=Stage::Active||!engine_.is_live(e)||
               (managed_&&kind==npc::NpcMapKind::Jessica&&!manager))return NpcMapResult::Rejected;
            // Configured script route supersedes older synthetic finder routes.
            // No fallback on missing bindings or transport failure.
            if(script_finder_){
                // Shell supplies actor-specific callbacks only. No borrowed
                // external registry may override the map-owned registry.
                if(speech->selection.finder.store.owner||speech->selection.finder.store.system)return NpcMapResult::Rejected;
                auto bound=*speech;
                bound.selection.finder.store=script_store_;
                if(shared_scene_filter_bound_){
                    // Refuse a shell-owned competing filter, never silently
                    // combine callback tables with different contexts.
                    auto& local=bound.selection.finder.host.scene_filter;
                    if(local.context||local.storage||local.resolve_name||local.metadata)return NpcMapResult::Rejected;
                    local=shared_scene_filter_;
                }
                // The DLL consumes these tables synchronously; only the
                // persistent engine leaves and resource owner are borrowed.
                npc::NativeGeneratedResponseBindings generated{};
                npc::NativeNamedPlaybackBindings named{};
                if(shared_raw_cache_bound_){
                    if(!bound.generated||!bound.generated->named)return NpcMapResult::Rejected;
                    generated=*bound.generated;named=*generated.named;
                    auto& local=named.precache.load.cache;
                    if(local.context||local.ensure||local.find||local.lock||local.reload||local.unlock)return NpcMapResult::Rejected;
                    local=shared_raw_cache_;generated.named=&named;bound.generated=&generated;
                }
                return status(script_->observe(kind,c,session_,b,d,manager,&bound,record,bytes));
            }
            return status(script_->observe(kind,c,session_,b,d,manager,speech,record,bytes));
        }
        if(generated_&&(kind==npc::NpcMapKind::Jessica||kind==npc::NpcMapKind::Count)){
            if(generated_->version!=npc::version||generated_->size!=sizeof(*generated_)||!generated_->observe)return NpcMapResult::Rejected;
            const auto* d=shells_->dialogue_bindings(e);
            const auto* scene=shells_->generated_scene_bindings(e);
            const auto* manager=kind==npc::NpcMapKind::Jessica?shells_->manager_bindings(e):nullptr;
            a=find(e);
            if(!d||!scene||!a||a->stage!=Stage::Active||!engine_.is_live(e)||
               (managed_&&kind==npc::NpcMapKind::Jessica&&!manager))return NpcMapResult::Rejected;
            // A configured native route must not silently fall back even
            // when legacy integrated playback is also available.
            return status(generated_->observe(kind,c,session_,b,d,manager,scene,record,bytes));
        }
        if(integrated_&&(kind==npc::NpcMapKind::Jessica||kind==npc::NpcMapKind::Count)){
            if(integrated_->version!=npc::version||integrated_->size!=sizeof(*integrated_)||!integrated_->observe)return NpcMapResult::Rejected;
            const auto* d=shells_->dialogue_bindings(e);
            const auto* scene=shells_->scene_bindings(e);
            const auto* manager=kind==npc::NpcMapKind::Jessica?shells_->manager_bindings(e):nullptr;
            a=find(e);
            if(!d||!scene||!a||a->stage!=Stage::Active||!engine_.is_live(e)||
               (managed_&&kind==npc::NpcMapKind::Jessica&&!manager))return NpcMapResult::Rejected;
            // Configured integrated route must not silently use legacy audio.
            return status(integrated_->observe(kind,c,session_,b,d,manager,scene,record,bytes));
        }
        if(managed_&&kind==npc::NpcMapKind::Jessica){
            if(managed_->version!=npc::version||managed_->size!=sizeof(*managed_)||!managed_->observe)return NpcMapResult::Rejected;
            const auto* d=shells_->dialogue_bindings(e);
            const auto* m=shells_->manager_bindings(e);
            a=find(e);
            if(!d||!m||!a||a->stage!=Stage::Active||!engine_.is_live(e))return NpcMapResult::Rejected;
            // No legacy fallback after managed routing is configured.
            return status(managed_->observe(c,&session_->attention,&d->jessica,m,record,bytes));
        }
        if(dialogue_&&(kind==npc::NpcMapKind::Jessica||kind==npc::NpcMapKind::Count)){
            if(dialogue_->version!=npc::version||dialogue_->size!=sizeof(*dialogue_)||!dialogue_->observe)return NpcMapResult::Rejected;
            const auto* d=shells_->dialogue_bindings(e);
            a=find(e);
            if(!d||!a||a->stage!=Stage::Active||!engine_.is_live(e))return NpcMapResult::Rejected;
            return status(dialogue_->observe(kind,c,session_,b,d,record,bytes));
        }
        return status(api_->observe(kind,c,session_,b,record,bytes));
    }
    NpcMapResult select_count(EntityRef e,int* result){
        if(!engine_.is_live(e))return NpcMapResult::Rejected;
        auto* a=find(e);if(!a||a->kind!=npc::NpcMapKind::Count)return NpcMapResult::Unhandled;
        if(a->stage!=Stage::Active)return NpcMapResult::Rejected;
        auto* c=control(e,a->kind);const auto* b=shells_->bindings(e);
        return c&&b?status(api_->select_count(c,session_,b,result)):NpcMapResult::Rejected;
    }
    // Engine-installed interaction callback must supply its generation-checked
    // entity identity; never reinterpret a retired NPC pointer as a live shell.
    NpcMapResult interaction(EntityRef e,unsigned* original_result){
        if(!engine_.is_live(e))return NpcMapResult::Rejected;
        auto* a=find(e);if(!a||a->kind!=npc::NpcMapKind::Jessica)return NpcMapResult::Unhandled;
        if(a->stage!=Stage::Active||!original_result)return NpcMapResult::Rejected;
        if(!interaction_)return NpcMapResult::Unhandled;
        if(interaction_->version!=npc::version||interaction_->size!=sizeof(*interaction_)||!interaction_->invoke)return NpcMapResult::Rejected;
        auto* c=control(e,npc::NpcMapKind::Jessica);const auto* b=shells_->interaction_bindings(e);
        a=find(e);
        if(!a||a->stage!=Stage::Active||!engine_.is_live(e)||!c||!b)return NpcMapResult::Rejected;
        return status(interaction_->invoke(c,b,original_result));
    }
    std::size_t attached_count()const noexcept{return entries_.size();}
private:
    struct ScriptGuard {
        bool& busy;bool previous;
        explicit ScriptGuard(bool& b):busy(b),previous(b){busy=true;}
        ~ScriptGuard(){busy=previous;}
    };
    bool script_ready()const noexcept{
        return script_&&script_->version==npc::version&&script_->size==sizeof(*script_)&&script_->observe&&
            script_finder_&&script_finder_->version==npc::version&&script_finder_->size==sizeof(*script_finder_)&&
            script_finder_->create&&script_finder_->release&&script_finder_->find;
    }
    enum class Stage {Keys,Spawning,Spawned,Activating,Active,Failed};
    struct Entry {std::uint32_t generation;npc::NpcMapKind kind;Stage stage;};
    Entry* find(EntityRef e){auto i=entries_.find(e.index);return i!=entries_.end()&&i->second.generation==e.generation?&i->second:nullptr;}
    static NpcMapResult status(int s){return s==1?NpcMapResult::Applied:s<0?NpcMapResult::Failed:NpcMapResult::Rejected;}
    static unsigned char lower(unsigned char c){return c>='A'&&c<='Z'?static_cast<unsigned char>(c+32):c;}
    static bool equal(std::string_view a,const char* b){
        if(!b||a.size()!=std::strlen(b))return false;
        for(std::size_t i=0;i<a.size();++i)if(lower(static_cast<unsigned char>(a[i]))!=lower(static_cast<unsigned char>(b[i])))return false;
        return true;
    }
    bool ready()const {
        if(script_finder_&&(!script_ready()||!script_session_open()||script_busy_))return false;
        if(!shells_||!api_||!session_||api_->version!=npc::version||api_->size!=sizeof(*api_)||
           !api_->schema||!api_->key||!api_->input||!api_->observe||!api_->select_count)return false;
        constexpr const char* names[]={"npc_jessica","npc_count","npc_generic_actor","npc_jc"};
        constexpr unsigned sizes[]={0x1458,0x103c,0x1014,0x1000},counts[]={4,2,5,0};
        for(unsigned i=0;i<4;++i){
            const auto* s=api_->schema(static_cast<npc::NpcMapKind>(i));
            if(!s||!s->classname||std::strcmp(s->classname,names[i])||s->bytes!=sizes[i]||s->count!=counts[i]||(s->count&&!s->fields))return false;
        }
        return true;
    }
    const npc::NpcMapField* field(npc::NpcMapKind kind,std::string_view name,npc::NpcMapFieldKind type)const{
        const auto* s=api_->schema(kind);
        for(unsigned i=0;i<s->count;++i)if(s->fields[i].kind==type&&equal(name,s->fields[i].name))return &s->fields[i];
        return nullptr;
    }
    npc::HologramControl* control(EntityRef e,npc::NpcMapKind kind){
        if(!engine_.is_live(e))return nullptr;
        auto* c=shells_->control(e);
        return c&&c->object&&c->bytes>=api_->schema(kind)->bytes?c:nullptr;
    }
    bool transition(EntityRef e,bool activate){
        auto* a=find(e);if(!a)return activate?engine_.activate(e):engine_.dispatch_spawn(e);
        if(a->stage!=(activate?Stage::Spawned:Stage::Keys)||!control(e,a->kind))return false;
        a->stage=activate?Stage::Activating:Stage::Spawning;
        // Unlike the complete radio spawn adapter, NPC reconstruction
        // covers selected slices only. Do not replace full engine Spawn.
        const bool ok=activate?engine_.activate(e):engine_.dispatch_spawn(e);
        a=find(e);if(!a)return false;
        if(!ok||!engine_.is_live(e)){a->stage=Stage::Failed;return false;}
        a->stage=activate?Stage::Active:Stage::Spawned;return true;
    }
    sin_compat::map::ISourceMapEntityHost& engine_;
    INpcMapShells* shells_=nullptr;const npc::NpcMapApi* api_=nullptr;npc::NpcMapSession* session_=nullptr;
    const npc::JessicaInteractionApi* interaction_=nullptr;
    const npc::NpcMapDialogueApi* dialogue_=nullptr;
    const npc::ManagedJessicaObservationApi* managed_=nullptr;
    const npc::IntegratedNpcMapDialogueApi* integrated_=nullptr;
    const npc::NativeGeneratedMapDialogueApi* generated_=nullptr;
    const npc::ScriptNpcMapDialogueApi* script_=nullptr;
    const npc::ScriptResponseFinderApi* script_finder_=nullptr;
    npc::ScriptResponseStore script_store_{};
    npc::ResponseSceneFilterBindings shared_scene_filter_{};
    npc::NamedSceneCacheBindings shared_raw_cache_{};
    bool shared_scene_filter_bound_=false,shared_raw_cache_bound_=false,script_busy_=false;
    std::map<std::uint32_t,Entry> entries_;
};
}
