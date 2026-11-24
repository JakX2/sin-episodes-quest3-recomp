#pragma once
// Compose with NativeBehaviorHost or an ordinary real engine host.
// The factory owns construction and destruction of compatible radio
// objects; this adapter owns only the BSP lifecycle routing.
#include "native_radio_entity.h"
#include "sin_compat/source_map_runtime.hpp"
#include <map>
#include <string>
#include <cstring>
namespace sin_port::map_host {
namespace radio=sin_port::native_weapon_tables;
using sin_compat::EntityRef;
class IRadioShellState {
public:
    virtual ~IRadioShellState()=default;
    // Return the persistent control of an ALREADY constructed, genuine
    // radio shell. Its serial handle must match e. Never a temporary copy.
    virtual radio::HologramControl* radio_control(EntityRef e)=0;
    // Existing engine FIELD_SOUNDNAME / FIELD_STRING / FIELD_INTEGER
    // parser (including pooled strings), without assigning the field.
    virtual bool parse_radio_key(EntityRef,const radio::RadioField&,std::string_view,radio::RadioValue&)=0;
};
enum class RadioMapInput {Unhandled,Applied,Rejected,Failed};
class NativeRadioMapHost final:public sin_compat::map::ISourceMapEntityHost {
public:
    NativeRadioMapHost(sin_compat::map::ISourceMapEntityHost& engine) noexcept:engine_(engine){}
    NativeRadioMapHost(sin_compat::map::ISourceMapEntityHost& engine,const radio::RadioEntityApi& api,
          IRadioShellState& shells,const radio::RadioEntityBindings& bindings,radio::RadioDispatcher& dispatcher) noexcept
        :engine_(engine),api_(&api),shells_(&shells),bindings_(&bindings),dispatcher_(&dispatcher){}
    NativeRadioMapHost(const NativeRadioMapHost&)=delete;
    NativeRadioMapHost& operator=(const NativeRadioMapHost&)=delete;
    bool is_live(EntityRef e) const override {return engine_.is_live(e);}
    bool create_entity(std::string_view name,EntityRef& e) override {
        const bool wants_radio=name=="env_radio_message";
        if(wants_radio&&!ready())return false;
        if(!engine_.create_entity(name,e)||!engine_.is_live(e))return false;
        if(!wants_radio)return true;
        // A duplicate index is not a new attachment. SourceMapRuntime
        // records the live handle before rollback if the host failed.
        if(!control(e))return false;
        return entries_.emplace(e.index,Entry{e.generation,Stage::Keys}).second;
    }
    bool key_value(EntityRef e,std::string_view key,std::string_view value) override {
        auto* entry=find(e);
        if(!entry)return engine_.key_value(e,key,value);
        if(entry->stage!=Stage::Keys||!engine_.is_live(e))return false;
        const auto* schema=api_->schema();
        for(unsigned i=0;i<schema->field_count;++i){
            const auto& field=schema->fields[i];
            if(field.kind!=radio::RadioFieldKind::Key||key!=field.name)continue;
            radio::RadioValue parsed{};
            if(!shells_->parse_radio_key(e,field,value,parsed))return false;
            auto* c=control(e);
            return c&&api_->key(c,field.name,&parsed)==1;
        }
        // Preserve all other keys including repeated output connections
        // and targetname, in the engine's ordinary KeyValue path.
        return engine_.key_value(e,key,value);
    }
    bool dispatch_spawn(EntityRef e) override {
        auto* entry=find(e);
        if(!entry)return engine_.dispatch_spawn(e);
        if(entry->stage!=Stage::Keys)return false;
        auto* c=control(e);if(!c)return false;
        // No second generic Spawn: native API runs the host base helpers.
        if(api_->spawn(c,bindings_)!=1||!engine_.is_live(e))return false;
        entry=find(e);if(!entry)return false;
        entry->stage=Stage::Spawned;return true;
    }
    bool activate(EntityRef e) override {
        auto* entry=find(e);
        if(!entry)return engine_.activate(e);
        if(entry->stage!=Stage::Spawned)return false;
        auto* c=control(e);if(!c)return false;
        if(api_->activate(c,bindings_)!=1||!engine_.is_live(e))return false;
        entry=find(e);if(!entry)return false;
        entry->stage=Stage::Active;return true;
    }
    bool remove_entity(EntityRef e) override {
        // Factory owns destruction exactly once, even after partial
        // initialization. Do not call api.destroy here as well.
        if(engine_.is_live(e))(void)engine_.remove_entity(e);
        return notify_deleted(e);
    }
    bool notify_deleted(EntityRef e) {
        if(engine_.is_live(e))return false;
        if(find(e))entries_.erase(e.index);
        // Dispatcher uses serial-validated handles; engine invalidation
        // is the authority. Never clear other/pending by raw index.
        return true;
    }
    RadioMapInput input(EntityRef e,std::string_view name,const radio::RadioValue* value) {
        if(!engine_.is_live(e))return RadioMapInput::Rejected;
        auto* entry=find(e);
        if(!entry)return RadioMapInput::Unhandled;
        if(entry->stage!=Stage::Active)return RadioMapInput::Rejected;
        const auto* schema=api_->schema();
        for(unsigned i=0;i<schema->field_count;++i){
            const auto& field=schema->fields[i];
            if(field.kind!=radio::RadioFieldKind::Input||name!=field.name)continue;
            auto* c=control(e);if(!c)return RadioMapInput::Rejected;
            const int status=api_->input(c,dispatcher_,bindings_,field.name,value);
            return status==1?RadioMapInput::Applied:status<0?RadioMapInput::Failed:RadioMapInput::Rejected;
        }
        return RadioMapInput::Unhandled;
    }
    std::size_t attached_count() const noexcept {return entries_.size();}
private:
    enum class Stage {Keys,Spawned,Active};
    struct Entry {std::uint32_t generation;Stage stage;};
    Entry* find(EntityRef e) noexcept {
        auto i=entries_.find(e.index);
        return i!=entries_.end()&&i->second.generation==e.generation?&i->second:nullptr;
    }
    bool ready() const noexcept {
        if(!api_||!shells_||!bindings_||!dispatcher_||api_->version!=radio::version||
           api_->size!=sizeof(*api_)||!api_->schema||!api_->key||!api_->spawn||!api_->activate||!api_->input)return false;
        const auto* s=api_->schema();
        return s&&s->classname&&!std::strcmp(s->classname,"env_radio_message")&&
            s->object_bytes==radio::radio_entity_bytes&&s->field_count==10&&s->fields&&
            bindings_->message.precache&&bindings_->message.string&&bindings_->spawn_helper&&bindings_->base_activate;
    }
    radio::HologramControl* control(EntityRef e) {
        if(!engine_.is_live(e))return nullptr;
        auto* c=shells_->radio_control(e);
        return c&&c->object&&c->bytes>=radio::radio_entity_bytes?c:nullptr;
    }
    sin_compat::map::ISourceMapEntityHost& engine_;
    const radio::RadioEntityApi* api_=nullptr;
    IRadioShellState* shells_=nullptr;
    const radio::RadioEntityBindings* bindings_=nullptr;
    radio::RadioDispatcher* dispatcher_=nullptr;
    std::map<std::uint32_t,Entry> entries_;
};
}
