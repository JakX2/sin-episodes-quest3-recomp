#pragma once
// Compose the BSP lifecycle with the actual loaded native module. The engine
// must allocate genuine class shells and own ordinary KeyValues/activation.
// Phase529's observed native trigger fields must not be guessed from BSP keys.
#include "server_fixed_difficulty_service.h"
#include "sin_compat/source_map_runtime.hpp"
#include <map>
#include <string>

namespace sin_port::map_host {
namespace a=fixed_service;
using sin_compat::EntityRef;
class ITriggerShellState {
public:
    virtual ~ITriggerShellState()=default;
    virtual bool observe_spawn(EntityRef,a::NativeTriggerSpawn&) const=0;
    virtual bool observe_touch(EntityRef,a::NativeTriggerTouch&) const=0;
};

class NativeBehaviorHost final:public sin_compat::map::ISourceMapEntityHost {
public:
    NativeBehaviorHost(sin_compat::map::ISourceMapEntityHost& engine,
                       const a::MapEntityApi& native,std::uint32_t epoch) noexcept
        :engine_(engine),native_(native),epoch_(epoch){}
    NativeBehaviorHost(sin_compat::map::ISourceMapEntityHost& engine,
                       const a::MapEntityApi& native,std::uint32_t epoch,
                       const a::CampaignTriggerApi& triggers,
                       const ITriggerShellState& fields,
                       const a::NativeTriggerHost& callbacks,void* context) noexcept
        :engine_(engine),native_(native),epoch_(epoch),triggers_(&triggers),
         fields_(&fields),callbacks_(&callbacks),context_(context){}
    NativeBehaviorHost(const NativeBehaviorHost&)=delete;
    NativeBehaviorHost& operator=(const NativeBehaviorHost&)=delete;
    // Underlying engine, fields, callbacks and loaded module outlive this
    // adapter. Unload runtime and reconcile deferred removals before ending
    // the level or unloading the DLL. All operations are on the game thread.
    bool is_live(EntityRef e) const override {return engine_.is_live(e);}
    bool create_entity(std::string_view classname,EntityRef& e) override {
        const bool trigger=classname=="trigger_dynamic_difficulty"||classname=="trigger_secret";
        if(trigger&&!trigger_ready())return false;
        if(!engine_.create_entity(classname,e))return false;
        if(!engine_.is_live(e))return false;
        const bool weapon=classname=="weapon_magnum"||classname=="weapon_assault_rifle"||classname=="weapon_scattergun";
        if(!trigger&&!weapon&&classname!="env_global"&&classname!="info_challenge")return true;
        const std::string owned(classname);
        if(native_.create(ref(e),owned.c_str())!=a::MapApplied)return false;
        try {
            if(attached_.emplace(e.index,Attachment{e.generation,trigger}).second)return true;
        }catch(...){}
        (void)native_.remove(ref(e));
        // SourceMapRuntime records live handles even on create failure.
        return false;
    }
    bool key_value(EntityRef e,std::string_view key,std::string_view value) override {
        const auto* entry=attached(e);
        if(!entry)return engine_.key_value(e,key,value);
        if(!engine_.is_live(e))return false;
        // Phase529: native trigger shell receives all keys, including
        // targetname and the recovered datamap fields, in source order.
        if(entry->trigger&&!engine_.key_value(e,key,value))return false;
        const std::string k(key),v(value);
        const auto result=native_.keyvalue(ref(e),k.c_str(),v.c_str());
        if(result==a::MapUnhandledKey)return entry->trigger||engine_.key_value(e,key,value);
        return result==a::MapApplied;
    }
    bool dispatch_spawn(EntityRef e) override {
        const auto* entry=attached(e);
        if(entry&&entry->trigger){
            a::NativeTriggerSpawn snapshot{};
            a::NativeTriggerReport report{};
            if(!trigger_ready()||!engine_.is_live(e)||!fields_->observe_spawn(e,snapshot))return false;
            // NativeTriggerHost::base_spawn is the sole base Spawn call.
            // Do not run generic dispatch_spawn as well. Suppressed
            // entities require a specialized loader; ordinary runtime
            // explicitly refuses them rather than fabricating activation.
            return triggers_->spawn(ref(e),&snapshot,callbacks_,context_,&report)&&
                report.code==a::TriggerHandled&&report.base_initialized&&!report.removal_queued&&engine_.is_live(e);
        }
        if(!engine_.dispatch_spawn(e)||!engine_.is_live(e))return false;
        if(!entry)return true;
        return native_.spawn(ref(e))==a::MapApplied;
    }
    bool activate(EntityRef e) override {return engine_.activate(e);}
    bool remove_entity(EntityRef e) override {
        if(engine_.is_live(e))(void)engine_.remove_entity(e);
        if(engine_.is_live(e))return false;
        return notify_deleted(e);
    }
    // Called after the engine's deferred removal really completed, outside
    // a DLL callback. SourceMapRuntime skips already-dead host entities, so
    // the native host must explicitly forward this destruction notification.
    bool notify_deleted(EntityRef e) {
        if(engine_.is_live(e))return false;
        if(!attached(e))return true;
        if(!native_.remove(ref(e)))return false;
        attached_.erase(e.index);
        return true;
    }
    // A failed snapshot/call returns false without an invented report.
    bool touch(EntityRef e,EntityRef actor,a::NativeTriggerReport& report) {
        const auto* entry=attached(e);
        a::NativeTriggerTouch snapshot{};
        if(!entry||!entry->trigger||!trigger_ready()||!engine_.is_live(e)||
           !fields_->observe_touch(e,snapshot))return false;
        return triggers_->touch(ref(e),ref(actor),&snapshot,callbacks_,context_,&report)!=0;
    }
    bool retry_remove(EntityRef e,a::NativeTriggerReport& report) {
        const auto* entry=attached(e);
        if(!entry||!entry->trigger||!trigger_ready()||!engine_.is_live(e))return false;
        return triggers_->retry_remove(ref(e),callbacks_,context_,&report)!=0;
    }
    // Host handles MapUnhandledInput with its own base entity I/O.
    int input(EntityRef e,const char* name,a::MapFloatOutput sink=nullptr,
              void* context=nullptr) {
        if(!engine_.is_live(e))return a::MapRejected;
        if(!attached(e))return a::MapUnhandledInput;
        return native_.input(ref(e),name,sink,context);
    }
    std::size_t attached_count() const noexcept {return attached_.size();}
private:
    struct Attachment {std::uint32_t generation;bool trigger;};
    a::MapEntityRef ref(EntityRef e) const noexcept {return {epoch_,e.index,e.generation};}
    const Attachment* attached(EntityRef e) const noexcept {
        const auto it=attached_.find(e.index);
        return it!=attached_.end()&&it->second.generation==e.generation?&it->second:nullptr;
    }
    bool trigger_ready() const noexcept {
        return triggers_&&fields_&&callbacks_&&triggers_->version==a::trigger_version&&
            triggers_->size==sizeof(*triggers_)&&triggers_->spawn&&triggers_->touch&&triggers_->retry_remove&&
            callbacks_->version==a::trigger_version&&callbacks_->size==sizeof(*callbacks_)&&
            callbacks_->is_live&&callbacks_->is_player&&callbacks_->base_spawn&&callbacks_->warning&&
            callbacks_->owner_index&&callbacks_->special_action&&callbacks_->publish&&callbacks_->queue_remove;
    }
    sin_compat::map::ISourceMapEntityHost& engine_;
    const a::MapEntityApi& native_;
    std::uint32_t epoch_;
    const a::CampaignTriggerApi* triggers_=nullptr;
    const ITriggerShellState* fields_=nullptr;
    const a::NativeTriggerHost* callbacks_=nullptr;
    void* context_=nullptr;
    std::map<std::uint32_t,Attachment> attached_;
};
} // namespace sin_port::map_host
