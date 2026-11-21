#pragma once
// Phase 532: actual 2006 SiN Druglab Grunt per-variant Spawn logic.
// Original server.dll SHA256:
// 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650
// Original CDruglabGrunt Spawn-like method 0x22369E40, complete original
// 1246 bytes SHA256 99dbbb09eeebe19d7260e707c4fb917ab4c4941f1b066d6368eba53b485ccff7,
// primary native vtable 0x2249AAA4 slot 25 (+0x64). This method runs
// AFTER the shared Phase 531 factory construction. It selects weapon names,
// modifies ORIGINAL fields +0x3B0/+0x98/+0x9C with game-network hooks, and
// calls original inherited Spawn hooks. The numeric fields' semantic
// property names are NOT yet proven, so keep original offsets explicit.
//
// An INDEPENDENT host must supply real engine-owned NPC operations.
// This adapter does not instantiate CBaseNPC, use original x86 pointers,
// run Source AI/nav/schedules, or read physics/models/props.
#include "sin_compat/original_druglab_grunt_factory.hpp"
#include <cstdint>
#include <string_view>

namespace sin_compat::original_druglab_grunt::spawn {

enum class DirtyPath : std::uint8_t {
    NoNetworkMarker,
    NativeField200, // source original field_1F0 set: set byte field_200
    EdictStateBit0  // otherwise when original edict field_16C exists
};
enum class Stage : std::uint8_t {
    None, WeaponSymbol, Field3B0, Virtual68, Virtual1CThen6C,
    InheritedSpawn, Field98, Field9C
};
enum class Code : std::uint8_t {
    Applied, UnsupportedClass, NonFiniteOrOutOfRangeGlobal,
    HostRejected, Reentrant
};
struct Snapshot {
    // Original object +0x0C classname and +0xA24 weapon symbol.
    std::string_view classname;
    std::string_view weapon_name;
    std::int32_t original_field_3B0=0;
    std::int32_t original_field_98=0;
    std::int32_t original_field_9C=0;
    bool original_flag_1F0=false;
    bool original_edict_16C_present=false;
    // Source original per-NPC ConVar-global pointer +0x28 float, read
    // separately for pistol/scattergun/assault rifle. Host reads live
    // original-equivalent data, not fabricated engine default settings.
    float variant_float_28=0.0f;
};
struct Plan {
    Variant variant=Variant::Unsupported;
    std::string_view desired_weapon{};
    std::int32_t desired_field_3B0=0;
    std::int32_t truncated_global_integer=0;
    DirtyPath dirty_path=DirtyPath::NoNetworkMarker;
    bool overwrite_weapon=false;
    bool update_field_3B0=false;
    bool update_field_98=false;
    bool update_field_9C=false;
};
struct Result {
    Code code=Code::UnsupportedClass;
    Stage failed_stage=Stage::None;
    Plan plan{};
    bool ok() const noexcept {return code==Code::Applied;}
};

// This is a *behavior plan* computed from original observed entity state.
// The original game used x87 FISTP with RC=truncate toward zero; for
// finite, in-range numbers, the clean-room plan uses matching conversion.
// Out-of-range/NaN is refused rather than claiming x87 indefinite-int parity.
Code plan_spawn(const Snapshot& current,Plan& result) noexcept;

class IHost {
public:
    virtual ~IHost()=default;
    virtual bool assign_original_weapon_symbol(std::string_view name)=0;
    // Recreates original dirty byte/edict bit and field+0x3B0 update,
    // including no dirty marker if the previous value was identical.
    virtual bool commit_original_field_3B0(std::int32_t target,
                                            DirtyPath dirty)=0;
    virtual bool native_virtual_68()=0;
    // Original vslot +0x1C returns an engine string/name pointer that is
    // forwarded to vslot +0x6C. Both must be run in source order.
    virtual bool native_virtual_1C_then_6C()=0;
    virtual bool inherited_npc_spawn_2235F2A0()=0;
    // Both original virtual setters happen BEFORE the raw original field
    // assignment. The host must perform each pair without forging a
    // Source networkable object or a shared ConVar.
    virtual bool virtual_1B0_then_write_field_98(std::int32_t value)=0;
    virtual bool virtual_1B4_then_write_field_9C(std::int32_t value)=0;
};

class SpawnReference final {
public:
    explicit SpawnReference(IHost& host) noexcept:host_(host){}
    SpawnReference(const SpawnReference&)=delete;
    SpawnReference& operator=(const SpawnReference&)=delete;
    Result apply(const Snapshot& current) noexcept;
private:
    IHost& host_;
    bool busy_=false;
};

} // namespace sin_compat::original_druglab_grunt::spawn
