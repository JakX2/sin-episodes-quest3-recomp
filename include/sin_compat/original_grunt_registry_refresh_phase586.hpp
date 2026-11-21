#pragma once
// Authored original-free behavioral reference. Not Ritual engine ABI.
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace sin_compat::phase586 {
constexpr std::size_t direct_record_stride = 0xD4; // original x86 0x2211BD00
struct Entry {
    std::string original_name;
    uint8_t flags=0;
    int32_t numeric_key=-1;
};
struct Registry {
    std::vector<Entry> entries;
    uint32_t stamp=0;
};
struct NameResolvers {
    std::function<int32_t(const std::string&)> original_220e6cb0;
    std::function<int32_t(const std::string&)> original_220e6fe0;
};
inline std::optional<std::size_t> IndexWithOriginalDirectClamp(int32_t i,std::size_t count) {
    // Native returns address of index zero on invalid index; a host needs an
    // actual record to safely dereference. No slot -> explicit nullopt.
    if(count==0)return std::nullopt;
    return (i<0 || static_cast<std::size_t>(i)>=count)?0u:static_cast<std::size_t>(i);
}
inline std::size_t DirectRecordByteOffset(int32_t index,std::size_t count) {
    const auto clamped=IndexWithOriginalDirectClamp(index,count);
    return clamped ? direct_record_stride*(*clamped) : 0;
}
inline void PrepareRecord(Entry& e,NameResolvers& hooks) {
    e.flags=static_cast<uint8_t>(e.flags|0x10u); // happens before name lookup
    if(e.original_name.empty())return;
    int32_t id=hooks.original_220e6cb0(e.original_name);
    if(id==-1)id=hooks.original_220e6fe0(e.original_name);
    e.numeric_key=id;
}
inline void RefreshAll(Registry* r,uint32_t new_stamp,NameResolvers& resolvers,
                       const std::function<void(std::size_t,Entry&)>& post_record) {
    if(!r)return;
    for(std::size_t i=0;i<r->entries.size();++i){
        PrepareRecord(r->entries[i],resolvers);
        post_record(i,r->entries[i]); // native 0x220EA730
    }
    r->stamp=new_stamp; // native 0x2211C180(registry,revision)
}
// Original 0x2211C110 returns the minimum revision among shared parts.
inline uint32_t Revision(uint32_t local_revision,bool paged,
                         const std::vector<uint32_t>& section_revisions) {
    uint32_t result=local_revision;
    if(paged && section_revisions.size()>1){
        for(std::size_t i=1;i<section_revisions.size();++i)
            if(section_revisions[i]<result)result=section_revisions[i];
    }
    return result;
}
}