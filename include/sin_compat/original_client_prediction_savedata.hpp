#pragma once
// Phase210: independently written, PARTIAL host-neutral contract for two
// SHA-verified ORIGINAL 2006 client.dll prediction/SaveData functions.
// Does NOT implement the original engine snapshot/object serializer.
#include <array>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include "sin_compat/original_allocator_game_callers.hpp"

namespace sin_compat::original_client_prediction_savedata {
namespace game=original_allocator_game_callers;
namespace cross=original_cross_dll_tier0_allocator;

inline constexpr std::uint32_t kOriginalInitPredictableVA=0x240caf40u;
inline constexpr std::uint32_t kOriginalSaveDataVA=0x240c5960u;
inline constexpr std::uint32_t kOriginalSaveDataHelperVA=0x24199ba0u;
inline constexpr std::uint32_t kOriginalSaveDataBuilderVA=0x241978a0u;
inline constexpr std::uint32_t kPredictableFlagObjectOffset=0x2e8u;
inline constexpr std::uint32_t kObjectMemberB4Offset=0xb4u;
inline constexpr std::uint32_t kSaveDataNamedSlots=90u;
inline constexpr std::uint32_t kClientSaveDataSentinelIndex=0xffffffffu;
inline constexpr std::uint32_t kOriginalVirtualInitializeOffset=0x44u;
inline constexpr std::uint32_t kOriginalDescriptorVirtualOffset=0x40u;
inline constexpr std::uint32_t kOriginalArrayVtableOffset=0x24u;
inline constexpr std::uint32_t kClientGlobalObjectVA=0x242f7f78u;
inline constexpr std::uint32_t kInitNameVA=0x242893dcu;
inline constexpr std::uint32_t kSaveDataProfileNameVA=0x24288f74u;
inline constexpr std::uint32_t kSaveDataSlotFormatVA=0x24288f44u;
inline constexpr std::uint32_t kSaveDataOriginalFormatVA=0x24288f5cu;
inline constexpr std::uint32_t kOriginalProfileFallbackVA=0x242d393cu;
inline constexpr const char* kInitName="InitPredictable";
inline constexpr const char* kProfileName="C_BaseEntity::SaveData";

struct PredictableSlice32 {
    // Disjoint ORIGINAL offsets: keep relative meaning, not false layout.
    std::uint32_t member_b4{};
    std::uint8_t predictable_flag_2e8{};
    game::ClientBufferSpan32 savedata{};
};

// In original 0x240C5960, index -1 selects this+0x454 sentinel. Otherwise
// if sentinel is zero, the pointer is null; else signed IDIV by 0x5A
// selects one of 90 DWORDs at this+0x2EC + 4*(index % 90).
// Other negative indices would lead to negative offsets in native x86;
// reject them in this host reference to avoid out-of-bounds reads.
inline std::uint32_t select_savedata_buffer(
    const game::ClientBufferSpan32& savedata,std::int32_t index) {
    if (index==-1)return savedata.sentinel_454;
    if (savedata.sentinel_454==0u)return 0u;
    if (index<0)throw std::out_of_range("host-only negative SaveData slot");
    return savedata.pointers_2ec_to_450[
        static_cast<std::uint32_t>(index)%kSaveDataNamedSlots];
}

inline std::string format_savedata_label(const char* prefix,std::int32_t slot) {
    if (!prefix)throw std::invalid_argument("host-only null SaveData prefix");
    char label[64]{};  // Q_snprintf native destination capacity = 0x40.
    if (slot==-1)
        std::snprintf(label,sizeof(label),"%s SaveData(original)",prefix);
    else
        std::snprintf(label,sizeof(label),"%s SaveData(slot %02i)",prefix,slot);
    return label;
}

// The original helper ENTERS tier0!CVProfile::EnterScope, calls
// vstdlib!Q_snprintf using the original string literals, builds a
// stack-local opaque serializer via 0x241978A0, queries virtual +0x40
// on the object and virtual +0x24 on object+8, calls 0x24199BA0,
// then EXITS tier0!CVProfile::ExitScope. The meaning of these
// serializer callbacks is NOT reconstructed and remains a Host action.
template<class Host>
decltype(auto) save_data_slot(const PredictableSlice32& object,
                              Host& host,const char* prefix,
                              std::int32_t index,std::uint32_t flag) {
    const std::uint32_t selected=select_savedata_buffer(object.savedata,index);
    const auto label=format_savedata_label(prefix,index);
    return host.save_data_opaque(selected,label,index,flag);
}

// Original 0x240CAF40: this+0x2E8 flag is set BEFORE virtual +0x44,
// then returned value is passed to 0x240C8F00; the 91-buffer allocator
// from Phase209 runs BEFORE original global +0x240CAC00 and local
// +0x240C6900(0), which run BEFORE the 90-slot SaveData sweep.
// Host callbacks (except the Phase209 allocator model) deliberately
// stand for unresolved game-engine methods, not invented ABI shims.
template<class Host>
void init_predictable(PredictableSlice32& object,cross::ModuleState& alloc,
                      Host& host) {
    object.predictable_flag_2e8=1u;
    const auto init_value=host.query_virtual_44();
    host.initialize_240c8f00(init_value);
    game::client_init_91_buffers(object.savedata,alloc,host);
    host.global_240cac00(object.member_b4);
    host.local_240c6900(0u);
    for (std::int32_t i=0;i<static_cast<std::int32_t>(kSaveDataNamedSlots);++i)
        (void)save_data_slot(object,host,kInitName,i,0u);
}
} // namespace sin_compat::original_client_prediction_savedata
