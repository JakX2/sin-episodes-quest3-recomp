#pragma once
// Independently owned registration surface for the last three original
// 2006 C_StatsSystem::Init calls. These are CUserMessages::HookMessage,
// NOT commands or the engine cvar registration API. No retail object,
// original handler pointer, or network payload is executed here.
// The user-message IDs / signed sizes come from the independently audited
// identical 39-entry original client/server protocol table.
#include "sin_user_message_catalog.h"
#include "client_stats_system_init.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace sin_independent::stats_messages {
enum class HookKind { StaticStat, DynamicStat, ResetHUD };
struct HookSpec final {
    const char* name;
    int message_id;
    int size_bytes;
    HookKind kind;
    std::uint32_t original_callsite;
    std::uint32_t original_callback_va;
};
inline constexpr std::array<HookSpec,3> kOriginalStatsHooks{{
    {"StaticStat",27,-1,HookKind::StaticStat,0x241FCADFu,0x241FB7A0u},
    {"DynamicStat",28,-1,HookKind::DynamicStat,0x241FCAF4u,0x241FB7E0u},
    {"ResetHUD",5,1,HookKind::ResetHUD,0x241FCB09u,0x241FCA60u}
}};
static_assert(kOriginalStatsHooks.size()==3,"three original stats user-message hooks");

// The original CUserMessages::HookMessage lookup locates the entry by name
// and appends the callback pointer to the message's own hook vector. This
// minimal owned registry only tracks ONE known Stats callback per message.
// It does not claim to implement arbitrary shared hook fanout.
class OwnedRegistry final {
public:
    bool RegisterStatsStep(const stats_system::Step& step) noexcept {
        if(step.kind!=stats_system::StepKind::RegisterCallback ||
           !step.argument || count_>=kOriginalStatsHooks.size())
            return false;
        const auto& expected=kOriginalStatsHooks[count_];
        if(std::strcmp(step.argument,expected.name)!=0 ||
           step.original_callsite!=expected.original_callsite ||
           step.original_callback_va!=expected.original_callback_va ||
           step.original_callee!=0x241CFF70u ||
           FindUserMessage(step.argument)!=expected.message_id ||
           kUserMessages[expected.message_id].bytes!=expected.size_bytes)
            return false;
        // This is independent metadata binding. No original callback
        // address is cast, invoked, or passed to a proprietary allocator.
        bound_[count_]=expected.message_id;
        ++count_;
        return true;
    }
    bool Complete() const noexcept {
        return count_==3 && bound_[0]==27 && bound_[1]==28 && bound_[2]==5;
    }
    std::size_t count() const noexcept {return count_;}
    int at(std::size_t index) const noexcept {
        return index<count_?bound_[index]:-1;
    }
    void Clear() noexcept {count_=0;bound_.fill(-1);}
private:
    std::array<int,3> bound_{{-1,-1,-1}};
    std::size_t count_=0;
};
// Coordinator calls its generic callback for stage 4 (unknown original
// layout UI link, intentionally skipped) and for stages 5-7 (known stats
// HookMessage registration). The coordinator passes its REGISTRY pointer
// for the latter. The reset of other runtime state remains unimplemented.
inline bool RegisterKnownStatsStep(void* object,
                                   const stats_system::Step& step) noexcept {
    if(step.kind==stats_system::StepKind::LoadDisplayLayout)
        return true; // external UI/lifecycle side effects still unknown
    if(step.kind!=stats_system::StepKind::RegisterCallback || !object)
        return false;
    return static_cast<OwnedRegistry*>(object)->RegisterStatsStep(step);
}

// Original callback StaticStat starts by reading a 16-bit unsigned
// statistic ID from bf_read. IDs 0..54 are static records. The callback
// DynamicStat also consumes a 16-bit ID, then accepts IDs >=55 and
// dispatches into the eleven dynamic records. The independent parser
// additionally rejects indices outside the known 0..65 catalog.
// The remainder of each payload is opaque: NOT YET a value decoder.
enum class HeaderStatus {
    KnownHeaderOnly, InvalidMessage, MissingHook, PayloadTruncated,
    PayloadExceedsLimit, StaticIdRefused, DynamicIdRefused,
    ResetLengthRefused
};
inline const char* HeaderName(HeaderStatus value) noexcept {
    switch(value) {
        case HeaderStatus::KnownHeaderOnly:return "known-header-only";
        case HeaderStatus::InvalidMessage:return "invalid-message";
        case HeaderStatus::MissingHook:return "unbound-user-message";
        case HeaderStatus::PayloadTruncated:return "truncated-header";
        case HeaderStatus::PayloadExceedsLimit:return "payload-exceeds-limit";
        case HeaderStatus::StaticIdRefused:return "static-id-out-of-range";
        case HeaderStatus::DynamicIdRefused:return "dynamic-id-out-of-range";
        case HeaderStatus::ResetLengthRefused:return "reset-payload-length-refused";
    }
    return "unknown";
}
struct Header final {
    int message_id=-1;
    int stat_id=-1;
    const std::uint8_t* opaque_value=nullptr;
    std::size_t opaque_bytes=0;
};
inline HeaderStatus InspectHeader(const OwnedRegistry& registry,int message_id,
                                  const std::uint8_t* bytes,std::size_t length,
                                  Header* out=nullptr) noexcept {
    if(out)*out={};
    if(!bytes || message_id<0 || message_id>=static_cast<int>(kUserMessageCount))
        return HeaderStatus::InvalidMessage;
    if(length>256u*1024u)return HeaderStatus::PayloadExceedsLimit;
    HookKind kind=HookKind::StaticStat;
    bool known=false;
    for(std::size_t i=0;i<kOriginalStatsHooks.size();++i) {
        if(kOriginalStatsHooks[i].message_id==message_id) {
            if(registry.at(i)!=message_id)return HeaderStatus::MissingHook;
            kind=kOriginalStatsHooks[i].kind;
            known=true;
            break;
        }
    }
    if(!known)return HeaderStatus::InvalidMessage;
    if(kind==HookKind::ResetHUD) {
        if(length!=1)return HeaderStatus::ResetLengthRefused;
        if(out){out->message_id=message_id;out->opaque_value=bytes;
                out->opaque_bytes=length;}
        return HeaderStatus::KnownHeaderOnly;
    }
    if(length<2)return HeaderStatus::PayloadTruncated;
    const unsigned id=static_cast<unsigned>(bytes[0]) |
                      (static_cast<unsigned>(bytes[1])<<8);
    if(kind==HookKind::StaticStat && id>=55)
        return HeaderStatus::StaticIdRefused;
    if(kind==HookKind::DynamicStat && (id<55 || id>=66))
        return HeaderStatus::DynamicIdRefused;
    if(out){
        out->message_id=message_id;
        out->stat_id=static_cast<int>(id);
        out->opaque_value=bytes+2;
        out->opaque_bytes=length-2;
    }
    return HeaderStatus::KnownHeaderOnly;
}
} // namespace sin_independent::stats_messages
