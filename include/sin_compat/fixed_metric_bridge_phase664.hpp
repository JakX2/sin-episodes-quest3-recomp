#pragma once
// Phase664: a verified, relocation-aware adapter to the ORIGINAL SiN 2006
// server.dll 14-record gameplay-variable table. The live engine/guest memory
// and ConVar operations are supplied by the host; this code does not patch a
// process or inject anything and is not a complete server.dll replacement.
//
// Original server 0x222D3340 returns &table[index*28], 0x222D33E0 reads
// current float, and 0x222D3440 writes current float (no bounds check).
// Original static table VA 0x22523478, pointer name table 0x2245F04C.
// The adapter adds strict bounds and original name-table checks. No Source
// ABI or unrelated HL2 physics/material code is implemented here.
#include "fixed_difficulty_phase663.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace sin_compat::difficulty::fixed_phase664 {
inline constexpr std::uint32_t kOriginalImageBase=0x22000000u;
inline constexpr std::uint32_t kOriginalImageSize=0x00628000u;
inline constexpr std::uint32_t kOriginalMetricTableRva=0x00523478u;
inline constexpr std::uint32_t kOriginalNameTableRva=0x0045f04cu;
inline constexpr std::uint32_t kOriginalRecordStride=28u;
inline constexpr std::uint32_t kOriginalNamePointerStride=4u;
inline constexpr std::uint32_t kOriginalMetricFieldOffset=0u;
inline constexpr std::size_t kOriginalMetricEntries=original_metric_count;
inline constexpr std::uint32_t kOriginalIndexToRecordVA=0x222d3340u;
inline constexpr std::uint32_t kOriginalIndexToFloatVA=0x222d33e0u;
inline constexpr std::uint32_t kOriginalFloatSetterVA=0x222d3440u;

class IOriginalGameHost {
public:
    virtual ~IOriginalGameHost()=default;
    // module_base comes from a trusted original PE32 image of the exact
    // 2006 owner DLL. Host must establish code/image provenance separately.
    // Read memory from actual loaded PE32 guest addresses. For native Win32
    // retail debugging the host can use OS-checked ReadProcessMemory.
    virtual bool read_u32(std::uint32_t address,std::uint32_t& out)=0;
    virtual bool read_u8(std::uint32_t address,std::uint8_t& out)=0;
    virtual bool read_f32(std::uint32_t address,float& out)=0;
    virtual bool write_f32(std::uint32_t address,float value)=0;
    // Must route to original real Source ConVar operations, not simply
    // update an unrelated host-side shadow counter.
    virtual bool get_convar_int(std::string_view name,int& out)=0;
    virtual bool set_convar_int(std::string_view name,int value)=0;
};

enum class BindCode : std::uint8_t {
    Completed, InvalidSkill, InvalidImageBase, InvalidImageLayout,
    NamePointerMissing, NamePointerOutsideImage, NameMismatch,
    CurrentValueUnreadable, ConvarReadFailure, ConvarWriteFailure,
    ConvarReadbackMismatch, MetricWriteFailure, MetricReadbackFailure,
    MetricReadbackMismatch, HostException, Reentrant
};
enum class Stage : std::uint8_t {
    None, ValidateNames, SnapshotCurrent, ReadAdaptiveConvar,
    DisableAdaptiveConvar, VerifyAdaptiveDisabled, ReadSkill,
    SelectSkill, VerifySkill, WriteCurrentMetric, VerifyCurrentMetric,
    RollbackMetric
};
struct BindResult {
    BindCode code=BindCode::InvalidImageBase;
    Stage stage=Stage::None;
    std::size_t failed_metric=kOriginalMetricEntries;
    std::size_t attempted_writes=0u;
    std::size_t confirmed_writes=0u;
    std::size_t rollback_attempted=0u;
    std::size_t rollback_verified=0u;
    bool published=false;
    bool recovery_required=false;
    bool adaptive_disabled=false;
    bool skill_selected=false;
    bool old_adaptive_enabled=false;
    int old_engine_skill=0;
    std::uint32_t module_base=0u;
};

std::uint32_t metric_va(std::uint32_t runtime_module_base,
                        std::size_t index) noexcept;
std::uint32_t name_pointer_va(std::uint32_t runtime_module_base,
                              std::size_t index) noexcept;

class OriginalMetricBridge final {
public:
    OriginalMetricBridge(IOriginalGameHost& host,
                         fixed_phase663::FixedDifficultyRuntime& policy,
                         std::uint32_t runtime_module_base) noexcept;
    // This is a full republish even for unchanged skill, to defeat the
    // original adaptive 14-float save/restore overwrite.
    // After failure, callers MUST abort game startup/restore (even if
    // attempted rollback succeeded). No false atomicity claim.
    BindResult apply_skill(int skill) noexcept;
private:
    bool original_names_valid(BindResult& result) noexcept;
    void try_rollback(BindResult& result,std::size_t inclusive_last,
                      const std::array<float,kOriginalMetricEntries>& prior) noexcept;
    IOriginalGameHost& host_;
    fixed_phase663::FixedDifficultyRuntime& policy_;
    std::uint32_t module_base_;
    bool busy_=false;
};

} // namespace sin_compat::difficulty::fixed_phase664
