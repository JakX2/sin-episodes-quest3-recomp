#pragma once
// Bounded and independently owned SiN Episodes 2006 Stats message
// sequence: StaticStat 27, DynamicStat 28, ResetHUD 5.
// Original 0x241FCA60 ResetHUD handler:
//   static-array cleanup 0x241FBEC0
//   secondary collection cleanup 0x241FC740
//   dynamic named-tree cleanup 0x241FF990
//   rebuild 55 static-value objects 0x241FF420
//   rebuild 11 dynamic slots 0x241FF8E0
//   mark owner byte +0xac (global 0x24378E94) as changed = 1.
// Independently model the KNOWN value ownership and reset order.
// Secondary collection purpose, actual original CUtlRBTree ABI,
// server network dispatch and HUD callback side effects remain unknown.
#include "client_stats_owned_dynamic_values.h"
#include <cstddef>

namespace sin_independent::stats_owned_messages {
enum class Result {
    StaticUpdated, StaticUnchanged, DynamicCreated, DynamicUpdated,
    DynamicUnchanged, ResetApplied, NotInitialized, UnknownMessage,
    MessageHeaderRefused, ValueRefused
};
inline const char* ResultName(Result value) noexcept {
    switch(value){
        case Result::StaticUpdated:return "owned-static-value-updated";
        case Result::StaticUnchanged:return "owned-static-extremum-unchanged";
        case Result::DynamicCreated:return "owned-dynamic-name-created";
        case Result::DynamicUpdated:return "owned-dynamic-name-reused";
        case Result::DynamicUnchanged:return "owned-dynamic-extremum-unchanged";
        case Result::ResetApplied:return "owned-stats-reset-reconstructed";
        case Result::NotInitialized:return "owned-stats-not-initialized";
        case Result::UnknownMessage:return "unknown-stats-message";
        case Result::MessageHeaderRefused:return "stats-message-header-refused";
        case Result::ValueRefused:return "stats-value-mutation-refused";
    }
    return "unknown";
}
class OwnedStatsRuntime final {
public:
    bool Initialize() noexcept {
        initialized_=false;
        hooks_.Clear();
        dynamic_.Clear();
        if(!static_.Initialize())return false;
        for(std::size_t i=0;i<3;++i){
            if(!hooks_.RegisterStatsStep(stats_system::kInitSteps[4+i]))
                return false;
        }
        initialized_=hooks_.Complete();
        reset_dirty_=true;
        reset_count_=0;
        return initialized_;
    }
    bool initialized() const noexcept{return initialized_;}
    bool resetDirty() const noexcept{return reset_dirty_;}
    void ClearResetDirty() noexcept {reset_dirty_=false;}
    std::size_t resetCount() const noexcept{return reset_count_;}
    const stats_value_mutation::OwnedStaticValues& staticValues() const noexcept{
        return static_;
    }
    const stats_dynamic_values::OwnedDynamicValues& dynamicValues() const noexcept{
        return dynamic_;
    }
    const stats_messages::OwnedRegistry& hooks() const noexcept{
        return hooks_;
    }
    // The original ResetHUD callback does not decode the byte inside
    // its C++ body. The game's original shared user-message protocol
    // declares ResetHUD payload SIZE=1. Our independent envelope
    // reader accepts exactly eight logical bits and advances that
    // extent, without inventing a byte meaning.
    Result Handle(int messageId,stats_bitbuf::Cursor& stream) noexcept {
        if(!initialized_)return Result::NotInitialized;
        if(messageId!=5 && messageId!=27 && messageId!=28)
            return Result::UnknownMessage;
        stats_bitbuf::Cursor candidate=stream;
        if(messageId==5){
            stats_bitbuf::ParsedHeader header{};
            if(stats_bitbuf::InspectBitHeader(hooks_,5,candidate,&header)!=
                   stats_messages::HeaderStatus::KnownHeaderOnly)
                return Result::MessageHeaderRefused;
            // The original cleanup path drops allocated static values,
            // clears the dynamic tree, reconstructs 55 static values
            // plus 11 empty dynamic type buckets and marks the owner
            // as changed. Independent static storage is fixed-size,
            // so reinitialize it without touching original pointers.
            if(!static_.Initialize())return Result::ValueRefused;
            dynamic_.Clear();
            reset_dirty_=true;
            ++reset_count_;
            stream=candidate;
            return Result::ResetApplied;
        }
        stats_wire_values::Frame frame{};
        if(stats_wire_values::InspectFrameForRegisteredId(
               hooks_,messageId,candidate,&frame)!=
               stats_wire_values::Status::DecodedRawFieldsOnly)
            return Result::MessageHeaderRefused;
        if(messageId==27){
            const auto updated=static_.ApplyStatic(frame);
            if(updated==stats_value_mutation::Result::UpdatedOwnedValue){
                // Per-value dirty marker is set by OwnedValue::Apply;
                // do not fabricate the original global ResetHUD flag.
                stream=candidate;
                return Result::StaticUpdated;
            }
            if(updated==stats_value_mutation::Result::ExtremaDidNotAdvance){
                stream=candidate;
                return Result::StaticUnchanged;
            }
            return Result::ValueRefused;
        }
        const auto dynamicStatus=dynamic_.Apply(frame,true);
        switch(dynamicStatus){
            case stats_dynamic_values::Status::CreatedAndUpdated:
                stream=candidate;
                return Result::DynamicCreated;
            case stats_dynamic_values::Status::ReusedAndUpdated:
                stream=candidate;
                return Result::DynamicUpdated;
            case stats_dynamic_values::Status::CreatedWithoutExtremumAdvance:
            case stats_dynamic_values::Status::ExistingWithoutExtremumAdvance:
                stream=candidate;
                return Result::DynamicUnchanged;
            default:
                return Result::ValueRefused;
        }
    }
private:
    stats_value_mutation::OwnedStaticValues static_{};
    stats_dynamic_values::OwnedDynamicValues dynamic_{};
    stats_messages::OwnedRegistry hooks_{};
    bool initialized_=false,reset_dirty_=false;
    std::size_t reset_count_=0;
};
} // namespace sin_independent::stats_owned_messages
