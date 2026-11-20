#pragma once
// 2006 SiN Episodes original DynamicStat named-value hierarchy,
// independently owned. Original private SHA-pinned 0x241FFCD0 builds
// a tree lookup key using snprintf("%s%i", name, statId, 256);
// if missing and allowCreate==true it calls 0x24206430, which chooses
// the original 16-case type factory from statID's compiled table and
// wraps dynamic IDs 55..65 with a separate 0x110-byte value object.
//
// This is NOT original C_StatsSystem, a Valve CUtlRBTree comparator,
// an engine bf_read, or an original allocator. We use conservative
// byte-exact keys and REJECT overflow instead of truncating names.
// Real game state and original DLL pointers are never touched.
#include "client_stats_owned_value_mutation.h"
#include <cstddef>
#include <cstring>
#include <map>
#include <string>
#include <utility>

namespace sin_independent::stats_dynamic_values {
inline constexpr unsigned kMinDynamicStatId=55u;
inline constexpr unsigned kMaxDynamicStatId=65u;
inline constexpr std::size_t kOriginalFormattedKeyBufferBytes=256u;
inline constexpr std::size_t kConservativeMaxNames=4096u;
inline constexpr std::size_t kMaxDynamicNameLength=253u; // 2-digit ID + NUL

enum class Status {
    CreatedAndUpdated, ReusedAndUpdated, CreatedWithoutExtremumAdvance,
    ExistingWithoutExtremumAdvance, MissingAndCreationDisabled,
    InvalidDynamicFrame, BadOriginalType, NameTooLongOrMalformed,
    CapacityRefused, AllocationRefused
};
inline const char* StatusName(Status status) noexcept {
    switch(status){
        case Status::CreatedAndUpdated:return "owned-dynamic-created";
        case Status::ReusedAndUpdated:return "owned-dynamic-reused";
        case Status::CreatedWithoutExtremumAdvance:
            return "owned-dynamic-created-extremum-unchanged";
        case Status::ExistingWithoutExtremumAdvance:
            return "owned-dynamic-existing-extremum-unchanged";
        case Status::MissingAndCreationDisabled:return "dynamic-name-not-found";
        case Status::InvalidDynamicFrame:return "invalid-dynamic-frame";
        case Status::BadOriginalType:return "unverified-original-stat-type";
        case Status::NameTooLongOrMalformed:return "dynamic-name-refused";
        case Status::CapacityRefused:return "dynamic-capacity-refused";
        case Status::AllocationRefused:return "dynamic-allocation-refused";
    }
    return "unknown";
}
class OwnedDynamicValues final {
public:
    std::size_t size() const noexcept{return named_.size();}
    void Clear() noexcept{named_.clear();}
    // Original formatting suffix is the decimal stat ID without any
    // separator, e.g. "pistol55". All valid dynamic IDs 55..65 are two
    // digits, so suffix cannot collide across distinct dynamic IDs
    // within the accepted untruncated subset.
    static bool MakeKey(unsigned statId,const char* name,
                        std::size_t length,std::string& out) {
        out.clear();
        if(statId<kMinDynamicStatId || statId>kMaxDynamicStatId ||
           !name || length>kMaxDynamicNameLength)
            return false;
        // Never dereference name[length]: Find's input may be a
        // non-NUL-terminated buffer with exactly length readable bytes.
        if(std::memchr(name,'\0',length)!=nullptr)return false;
        out.assign(name,length);
        out+=std::to_string(statId);
        return out.size()+1u<=kOriginalFormattedKeyBufferBytes;
    }
    Status Apply(const stats_wire_values::Frame& frame,
                 bool allowCreate=true) noexcept {
        if(frame.message_id!=28 || frame.stat_id<
                static_cast<int>(kMinDynamicStatId) ||
           frame.stat_id>static_cast<int>(kMaxDynamicStatId) ||
           !frame.has_dynamic_text)
            return Status::InvalidDynamicFrame;
        const int originalType=stats_original_types::FactoryTypeForStatId(
            static_cast<unsigned>(frame.stat_id));
        if(originalType<0 ||
           frame.factory_type!=static_cast<unsigned>(originalType))
            return Status::BadOriginalType;
        if(frame.dynamic_text_bytes>kMaxDynamicNameLength ||
           frame.dynamic_text_bytes>=frame.dynamic_text.size())
            return Status::NameTooLongOrMalformed;
        if(frame.dynamic_text[frame.dynamic_text_bytes]!='\0')
            return Status::NameTooLongOrMalformed;
        try {
            std::string key;
            if(!MakeKey(static_cast<unsigned>(frame.stat_id),
                        frame.dynamic_text.data(),frame.dynamic_text_bytes,key))
                return Status::NameTooLongOrMalformed;
            const auto existing=named_.find(key);
            if(existing!=named_.end()){
                const auto outcome=existing->second.Apply(frame);
                if(outcome==stats_value_mutation::Result::UpdatedOwnedValue)
                    return Status::ReusedAndUpdated;
                if(outcome==stats_value_mutation::Result::ExtremaDidNotAdvance)
                    return Status::ExistingWithoutExtremumAdvance;
                return Status::InvalidDynamicFrame;
            }
            if(!allowCreate)return Status::MissingAndCreationDisabled;
            if(named_.size()>=kConservativeMaxNames)
                return Status::CapacityRefused;
            stats_value_mutation::OwnedValue value;
            if(!value.Initialize(static_cast<unsigned>(originalType)))
                return Status::BadOriginalType;
            const auto result=value.Apply(frame);
            if(result!=stats_value_mutation::Result::UpdatedOwnedValue &&
               result!=stats_value_mutation::Result::ExtremaDidNotAdvance)
                return Status::InvalidDynamicFrame;
            named_.emplace(std::move(key),value);
            return result==stats_value_mutation::Result::UpdatedOwnedValue?
                Status::CreatedAndUpdated:
                Status::CreatedWithoutExtremumAdvance;
        }catch(...){
            // The original tree/allocator would not necessarily recover
            // from OOM. This independent port refuses instead of
            // publishing a malformed half-created dynamic record.
            return Status::AllocationRefused;
        }
    }
    const stats_value_mutation::OwnedValue* Find(
        unsigned statId,const char* name,std::size_t length) const noexcept {
        try {
            std::string key;
            if(!MakeKey(statId,name,length,key))return nullptr;
            const auto found=named_.find(key);
            return found==named_.end()?nullptr:&found->second;
        }catch(...){return nullptr;}
    }
private:
    std::map<std::string,stats_value_mutation::OwnedValue> named_{};
};
} // namespace sin_independent::stats_dynamic_values
