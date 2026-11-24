#pragma once
#include "native_vehicle_lifecycle.h"
#include "native_vehicle_map.h"
#include "sin_compat/source_map_runtime.hpp"
#include <map>
#include <cstring>
namespace sin_port::map_host {
namespace veh=sin_port::native_weapon_tables;
using sin_compat::EntityRef;
enum class VehicleMapResult {Unhandled,Applied,Rejected,Failed};
class IVehicleMapShells {
public:
    virtual ~IVehicleMapShells()=default;
    // Factory has already initialized the appropriate native scalar
    // defaults and genuine base shell. Control lifetime is generation-bound.
    virtual veh::HologramControl* control(EntityRef)=0;
    virtual bool parse_key(EntityRef,unsigned type,std::string_view value,std::uint32_t& word)=0;
};
class NativeVehicleMapHost final:public sin_compat::map::ISourceMapEntityHost {
public:
    explicit NativeVehicleMapHost(sin_compat::map::ISourceMapEntityHost& engine):engine_(engine){}
    NativeVehicleMapHost(sin_compat::map::ISourceMapEntityHost& engine,IVehicleMapShells& shells,
        const veh::VehicleMapApi& map,const veh::VehicleLifecycleApi& life,const veh::VehicleMapBindings& vb,
        const veh::VehicleLifecycleBindings& lb,const veh::MusicRadioMapApi& music,const veh::MusicRadioApi& playback,
        const veh::MusicRadioMapBindings& mb)
        :engine_(engine),shells_(&shells),map_(&map),life_(&life),vb_(&vb),lb_(&lb),music_(&music),playback_(&playback),mb_(&mb){}
    NativeVehicleMapHost(const NativeVehicleMapHost&)=delete;
    NativeVehicleMapHost& operator=(const NativeVehicleMapHost&)=delete;
    bool is_live(EntityRef e)const override{return engine_.is_live(e);}
    bool create_entity(std::string_view name,EntityRef& e)override{
        const auto kind=name=="prop_vehicle_sin"?Kind::Vehicle:name=="env_radio"?Kind::Music:Kind::Ordinary;
        if(kind!=Kind::Ordinary&&!ready())return false;
        if(!engine_.create_entity(name,e)||!engine_.is_live(e))return false;
        if(kind==Kind::Ordinary)return true;
        if(!control(e,kind))return false;
        return entries_.emplace(e.index,Entry{e.generation,kind,Stage::Keys}).second;
    }
    bool key_value(EntityRef e,std::string_view key,std::string_view value)override{
        auto* entry=find(e);
        if(!entry)return engine_.key_value(e,key,value);
        if(entry->stage!=Stage::Keys||!engine_.is_live(e))return false;
        const auto kind=entry->kind;
        const char* name=nullptr;unsigned type=0;
        if(kind==Kind::Vehicle){
            const auto* s=map_->schema();
            for(unsigned i=0;i<s->field_count;++i){const auto& f=s->fields[i];if(f.kind==veh::VehicleFieldKind::Key&&equal(key,f.name)){name=f.name;type=f.type;break;}}
        }else{
            const auto* s=music_->schema();
            for(unsigned i=0;i<s->count;++i){const auto& f=s->fields[i];if(f.kind==veh::MusicFieldKind::Key&&equal(key,f.name)){name=f.name;type=f.type;break;}}
        }
        if(!name)return engine_.key_value(e,key,value); // output connections and all base keys
        std::uint32_t word=0;
        if(!shells_->parse_key(e,type,value,word))return false;
        entry=find(e);if(!entry||entry->stage!=Stage::Keys)return false;
        auto* c=control(e,kind);if(!c)return false;
        return (kind==Kind::Vehicle?map_->key(c,name,type,word):music_->key(c,name,type,word))==1;
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
    VehicleMapResult input(EntityRef e,std::string_view key,unsigned type,std::uint32_t word,void* activator){
        if(!engine_.is_live(e))return VehicleMapResult::Rejected;
        auto* entry=find(e);if(!entry)return VehicleMapResult::Unhandled;
        if(entry->stage!=Stage::Active)return VehicleMapResult::Rejected;
        const auto kind=entry->kind;const char* name=nullptr;
        if(kind==Kind::Vehicle){
            const auto* s=map_->schema();
            for(unsigned i=0;i<s->field_count;++i){const auto& f=s->fields[i];if(f.kind==veh::VehicleFieldKind::Input&&equal(key,f.name)){name=f.name;break;}}
        }else{
            const auto* s=music_->schema();
            for(unsigned i=0;i<s->count;++i){const auto& f=s->fields[i];if(f.kind==veh::MusicFieldKind::Input&&equal(key,f.name)){name=f.name;break;}}
        }
        if(!name)return VehicleMapResult::Unhandled;
        auto* c=control(e,kind);if(!c)return VehicleMapResult::Rejected;
        return status(kind==Kind::Vehicle?map_->input(c,vb_,name,type,word,activator):music_->input(c,mb_,name,type,word));
    }
    VehicleMapResult radio_think(EntityRef e){
        if(!engine_.is_live(e))return VehicleMapResult::Rejected;
        auto* entry=find(e);if(!entry||entry->kind!=Kind::Music)return VehicleMapResult::Unhandled;
        if(entry->stage!=Stage::Active)return VehicleMapResult::Rejected;
        auto* c=control(e,Kind::Music);
        return c?status(playback_->tick(c,&mb_->playback)):VehicleMapResult::Rejected;
    }
    std::size_t attached_count()const noexcept{return entries_.size();}
private:
    enum class Kind {Ordinary,Vehicle,Music};
    enum class Stage {Keys,Spawning,Spawned,Activating,Active,Failed};
    struct Entry {std::uint32_t generation;Kind kind;Stage stage;};
    static unsigned char lower(unsigned char c){return c>='A'&&c<='Z'?static_cast<unsigned char>(c+32):c;}
    // Normalize schema spelling at the host boundary (FGD uses volume
    // while datamap uses Volume); preserve unknown keys verbatim.
    static bool equal(std::string_view a,const char* b){
        if(!b||a.size()!=std::strlen(b))return false;
        for(std::size_t i=0;i<a.size();++i)if(lower(static_cast<unsigned char>(a[i]))!=lower(static_cast<unsigned char>(b[i])))return false;
        return true;
    }
    static VehicleMapResult status(int s){return s==1?VehicleMapResult::Applied:s<0?VehicleMapResult::Failed:VehicleMapResult::Rejected;}
    Entry* find(EntityRef e){
        auto i=entries_.find(e.index);return i!=entries_.end()&&i->second.generation==e.generation?&i->second:nullptr;
    }
    template<class Api> static bool abi(const Api* a){return a&&a->version==veh::version&&a->size==sizeof(Api);}
    bool ready()const{
        if(!shells_||!abi(map_)||!abi(life_)||!abi(music_)||!abi(playback_)||!vb_||!lb_||!mb_||
           !map_->schema||!map_->key||!map_->input||!life_->spawn||!life_->activate||
           !music_->schema||!music_->key||!music_->input||!music_->spawn||!music_->activate||!playback_->tick)return false;
        const auto* v=map_->schema();const auto* m=music_->schema();
        return v&&m&&v->fields&&m->fields&&v->field_count==40&&m->count==12&&m->object_bytes==veh::music_radio_bytes&&
            v->classname&&m->classname&&!std::strcmp(v->classname,"prop_vehicle_sin")&&!std::strcmp(m->classname,"env_radio");
    }
    veh::HologramControl* control(EntityRef e,Kind kind){
        if(!engine_.is_live(e)||!shells_)return nullptr;
        auto* c=shells_->control(e);
        return c&&c->object&&c->bytes>=(kind==Kind::Vehicle?veh::vehicle_object_bytes:veh::music_radio_bytes)?c:nullptr;
    }
    bool transition(EntityRef e,bool activate){
        auto* entry=find(e);
        if(!entry)return activate?engine_.activate(e):engine_.dispatch_spawn(e);
        if(entry->stage!=(activate?Stage::Spawned:Stage::Keys))return false;
        const auto kind=entry->kind;
        auto* c=control(e,kind);if(!c)return false;
        entry->stage=activate?Stage::Activating:Stage::Spawning;
        const int result=kind==Kind::Vehicle?(activate?life_->activate(c,lb_):life_->spawn(c,lb_)):
            (activate?music_->activate(c,mb_):music_->spawn(c,mb_));
        entry=find(e);
        if(!entry)return false;
        if(result!=1||!engine_.is_live(e)){entry->stage=Stage::Failed;return false;}
        entry->stage=activate?Stage::Active:Stage::Spawned;return true;
    }
    sin_compat::map::ISourceMapEntityHost& engine_;
    IVehicleMapShells* shells_=nullptr;
    const veh::VehicleMapApi* map_=nullptr;
    const veh::VehicleLifecycleApi* life_=nullptr;
    const veh::VehicleMapBindings* vb_=nullptr;
    const veh::VehicleLifecycleBindings* lb_=nullptr;
    const veh::MusicRadioMapApi* music_=nullptr;
    const veh::MusicRadioApi* playback_=nullptr;
    const veh::MusicRadioMapBindings* mb_=nullptr;
    std::map<std::uint32_t,Entry> entries_;
};
}
