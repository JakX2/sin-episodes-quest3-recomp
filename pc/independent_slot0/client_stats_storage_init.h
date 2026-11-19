#pragma once
// Original SiN 2006 C_StatsSystem early Init storage, independently owned.
// Original functions: 0x241FF420 (55 static 8-byte records) and
// 0x241FF8E0 (11 dynamic 20-byte zero-initialized records).
// These host types are NOT substituted for engine-owned 2006 objects.
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include "client_stats_system_init.h"

namespace sin_independent::stats_storage {
constexpr std::size_t kStaticSlots=55;
constexpr std::size_t kDynamicSlots=11;
constexpr std::size_t kStaticDefinitionCases=16;
struct StaticRecord {
    void* definition=nullptr;
    void* runtime=nullptr;
};
struct DynamicRecord {
    std::uint32_t fields[5]={};
};
using DefinitionFactory=void* (*)(void*,std::size_t) noexcept;
using AllowAllocation=bool (*)(void*,bool,std::size_t) noexcept;
// The original DLL returns success from outer Stats Init even when a
// particular malloc returns null, leaving that vector slot null.
class Storage final {
public:
    Storage()=default;
    Storage(const Storage&)=delete;
    Storage& operator=(const Storage&)=delete;
    Storage(Storage&&)=delete;
    Storage& operator=(Storage&&)=delete;
    void ResetStatic(void* ctx,DefinitionFactory factory=nullptr,
                     AllowAllocation allowed=nullptr) noexcept {
        for(auto& item:statics_)item.reset();
        static_count_=0;
        for(std::size_t index=0;index<kStaticSlots;++index){
            auto* record=(!allowed || allowed(ctx,false,index))?
                new(std::nothrow) StaticRecord:nullptr;
            if(record) {
                // The native factory is called for *every* successful
                // allocation. Its 0..15 dispatch contains specializations,
                // while 16..54 resolve to null in the original factory.
                record->definition=factory?factory(ctx,index):nullptr;
                record->runtime=nullptr;
            }
            statics_[index].reset(record);
            ++static_count_; // original appends even a null malloc result
        }
    }
    void ResetDynamic(void* ctx=nullptr,
                      AllowAllocation allowed=nullptr) noexcept {
        for(auto& item:dynamics_)item.reset();
        dynamic_count_=0;
        for(std::size_t index=0;index<kDynamicSlots;++index){
            auto* record=(!allowed || allowed(ctx,true,index))?
                new(std::nothrow) DynamicRecord:nullptr;
            if(record)for(auto& field:record->fields)field=0;
            dynamics_[index].reset(record);
            ++dynamic_count_;
        }
    }
    std::size_t staticCount() const noexcept{return static_count_;}
    std::size_t dynamicCount() const noexcept{return dynamic_count_;}
    const StaticRecord* staticAt(std::size_t index) const noexcept {
        return index<static_count_?statics_[index].get():nullptr;
    }
    const DynamicRecord* dynamicAt(std::size_t index) const noexcept {
        return index<dynamic_count_?dynamics_[index].get():nullptr;
    }
    StaticRecord* mutableStatic(std::size_t index) noexcept {
        return index<static_count_?statics_[index].get():nullptr;
    }
    DynamicRecord* mutableDynamic(std::size_t index) noexcept {
        return index<dynamic_count_?dynamics_[index].get():nullptr;
    }
private:
    std::array<std::unique_ptr<StaticRecord>,kStaticSlots> statics_{};
    std::array<std::unique_ptr<DynamicRecord>,kDynamicSlots> dynamics_{};
    std::size_t static_count_=0;
    std::size_t dynamic_count_=0;
};
using RemainingStep=bool (*)(void*,const stats_system::Step&) noexcept;
// Connect the first two actually reconstructed helpers to the existing
// seven-call original trace. The later five operations remain injected;
// host returns false on a missing or rejecting dependency. The retail
// function itself unconditionally returns true, so fail-closed behavior
// here is a deliberate independent-runtime safety difference.
inline bool ExecuteKnownStages(Storage& storage,void* system,void* registry,
                               void* ctx,DefinitionFactory factory,
                               AllowAllocation allowed,
                               RemainingStep unknown) noexcept {
    if(!system || !registry || !unknown)return false;
    for(const auto& step:stats_system::kInitSteps){
        switch(step.kind){
            case stats_system::StepKind::InitializeStaticData:
                storage.ResetStatic(ctx,factory,allowed);
                break;
            case stats_system::StepKind::InitializeDynamicData:
                storage.ResetDynamic(ctx,allowed);
                break;
            case stats_system::StepKind::RegisterCallback:
                if(!unknown(registry,step))return false;
                break;
            default:
                if(!unknown(system,step))return false;
                break;
        }
    }
    return true;
}
} // namespace sin_independent::stats_storage
