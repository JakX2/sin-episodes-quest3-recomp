#pragma once
// Original 2006 SiN difficulty manager's archive numeric field contract.
// Independent C++17: NOT Ritual source, complete Source savegame/ABI or Quest module.
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
namespace sin_compat::difficulty {
inline constexpr std::size_t archive_metric_count=14;
inline constexpr std::size_t archive_modifier_pair_count=10;
inline constexpr std::size_t archive_modifier_scalar_count=20;
inline constexpr std::size_t archive_advisor_object_count=6;
struct ManagerStateArchiveFields {
    std::array<float,archive_metric_count> metrics{};
    // Per index the original uses selector 1, then selector 3.
    std::array<std::array<float,2>,archive_modifier_pair_count> modifiers{};
};
inline std::vector<float> snapshot_metric_scalars(const ManagerStateArchiveFields& state) {
    return {state.metrics.begin(),state.metrics.end()};
}
inline std::vector<float> snapshot_modifier_scalars(const ManagerStateArchiveFields& state) {
    std::vector<float> out;out.reserve(archive_modifier_scalar_count);
    for(const auto& pair:state.modifiers) {
        out.push_back(pair[0]);out.push_back(pair[1]);
    }
    return out;
}
// Applies only values already consumed by an external host archive reader.
// Its wire/disk container and message framing remain unknown.
inline bool apply_metric_scalars(ManagerStateArchiveFields& state,
                                 std::int32_t declared_count,
                                 const std::vector<float>& values) {
    if(declared_count<0 || static_cast<std::size_t>(declared_count)>values.size()) return false;
    const auto limit=std::min(archive_metric_count,static_cast<std::size_t>(declared_count));
    for(std::size_t i=0;i<limit;++i)state.metrics[i]=values[i];
    return true;
}
// Original reader uses signed count / 2, capped at ten complete pairs.
// An odd trailing scalar is ignored by this state copy, not a complete pair.
inline bool apply_modifier_scalars(ManagerStateArchiveFields& state,
                                   std::int32_t declared_scalar_count,
                                   const std::vector<float>& values) {
    if(declared_scalar_count<0 ||
       static_cast<std::size_t>(declared_scalar_count)>values.size()) return false;
    const auto limit=std::min(archive_modifier_pair_count,
        static_cast<std::size_t>(declared_scalar_count/2));
    for(std::size_t i=0;i<limit;++i) {
        state.modifiers[i][0]=values[2*i];
        state.modifiers[i][1]=values[2*i+1];
    }
    return true;
}
// Advisor objects have opaque polymorphic implementations not recovered here.
inline std::size_t advisor_objects_to_restore(std::int32_t reported_count) {
    if(reported_count<=0)return 0;
    return std::min(archive_advisor_object_count,static_cast<std::size_t>(reported_count));
}
}
