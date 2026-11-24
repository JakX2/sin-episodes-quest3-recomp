#pragma once
#include "native_npc_map.h"
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
};
class NativeNpcMapHost final:public sin_compat::map::ISourceMapEntityHost {
public:
    explicit NativeNpcMapHost(sin_compat::map::ISourceMapEntityHost& engine):engine_(engine){}
    NativeNpcMapHost(sin_compat::map::ISourceMapEntityHost& engine,INpcMapShells& shells,const npc::NpcMapApi& api,npc::NpcMapSession& session,const npc::JessicaInteractionApi* interaction=nullptr):
        engine_(engine),shells_(&shells),api_(&api),session_(&session),interaction_(interaction){}
    NativeNpcMapHost(const NativeNpcMapHost&)=delete;
    NativeNpcMapHost& operator=(const NativeNpcMapHost&)=delete;
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
        return c&&b?status(api_->observe(kind,c,session_,b,record,bytes)):NpcMapResult::Rejected;
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
    std::map<std::uint32_t,Entry> entries_;
};
}
