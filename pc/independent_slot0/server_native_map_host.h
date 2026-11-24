#pragma once
// Composition of Phase146 SourceMapRuntime and Phase671 native DLL API.
// The underlying host MUST allocate engine-owned shells for the two port
// classes, handle base keys/output connections, and perform base spawn only.
// This layer is not a replacement for a real Source entity factory.
#include "server_fixed_difficulty_service.h"
#include "sin_compat/source_map_runtime.hpp"
#include <map>
#include <string>

namespace sin_port::map_host {
namespace a=fixed_service;
using sin_compat::EntityRef;
class NativeBehaviorHost final:public sin_compat::map::ISourceMapEntityHost {
public:
    NativeBehaviorHost(sin_compat::map::ISourceMapEntityHost& engine,
                       const a::MapEntityApi& native,std::uint32_t epoch) noexcept
        :engine_(engine),native_(native),epoch_(epoch){}
    NativeBehaviorHost(const NativeBehaviorHost&)=delete;
    NativeBehaviorHost& operator=(const NativeBehaviorHost&)=delete;
    // Underlying engine and loaded module outlive this adapter. Caller
    // unloads SourceMapRuntime before ending the level/unloading the DLL.
    bool is_live(EntityRef e) const override {return engine_.is_live(e);}
    bool create_entity(std::string_view classname,EntityRef& e) override {
        if(!engine_.create_entity(classname,e))return false;
        if(!engine_.is_live(e))return false;
        if(classname!="env_global"&&classname!="info_challenge")return true;
        const std::string owned(classname);
        if(native_.create(ref(e),owned.c_str())!=a::MapApplied)return false;
        try {
            if(attached_.emplace(e.index,e.generation).second)return true;
        }catch(...){}
        (void)native_.remove(ref(e));
        // SourceMapRuntime records live handles even on create failure.
        return false;
    }
    bool key_value(EntityRef e,std::string_view key,std::string_view value) override {
        if(!attached(e))return engine_.key_value(e,key,value);
        if(!engine_.is_live(e))return false;
        const std::string k(key),v(value);
        const auto result=native_.keyvalue(ref(e),k.c_str(),v.c_str());
        if(result==a::MapUnhandledKey)return engine_.key_value(e,key,value);
        return result==a::MapApplied;
    }
    bool dispatch_spawn(EntityRef e) override {
        if(!engine_.dispatch_spawn(e)||!engine_.is_live(e))return false;
        if(!attached(e))return true;
        // SourceMapRuntime's ordinary-entity contract requires a live
        // entity after spawn. RemoveSelf needs a specialized native map
        // loader; fail explicitly instead of inventing successful spawn.
        return native_.spawn(ref(e))==a::MapApplied;
    }
    bool activate(EntityRef e) override {return engine_.activate(e);}
    bool remove_entity(EntityRef e) override {
        const bool removed=engine_.remove_entity(e);
        if(engine_.is_live(e))return false;
        if(!attached(e))return removed;
        const bool detached=native_.remove(ref(e))!=0;
        if(detached)attached_.erase(e.index);
        return detached;
    }
    // Forward a class-specific input only for an attached live shell.
    // Caller handles MapUnhandledInput using its own base entity I/O.
    int input(EntityRef e,const char* name,a::MapFloatOutput sink=nullptr,
              void* context=nullptr) {
        if(!engine_.is_live(e))return a::MapRejected;
        if(!attached(e))return a::MapUnhandledInput;
        return native_.input(ref(e),name,sink,context);
    }
    std::size_t attached_count() const noexcept {return attached_.size();}
private:
    a::MapEntityRef ref(EntityRef e) const noexcept {return {epoch_,e.index,e.generation};}
    bool attached(EntityRef e) const noexcept {
        const auto it=attached_.find(e.index);
        return it!=attached_.end()&&it->second==e.generation;
    }
    sin_compat::map::ISourceMapEntityHost& engine_;
    const a::MapEntityApi& native_;
    std::uint32_t epoch_;
    std::map<std::uint32_t,std::uint32_t> attached_;
};
} // namespace sin_port::map_host
