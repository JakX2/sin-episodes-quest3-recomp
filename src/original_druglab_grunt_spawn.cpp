#include "sin_compat/original_druglab_grunt_spawn.hpp"
#include <cmath>
#include <limits>

namespace sin_compat::original_druglab_grunt::spawn {

Code plan_spawn(const Snapshot& current,Plan& output) noexcept {
    Plan candidate{};
    candidate.variant=GruntFactoryReference::classify(current.classname);
    switch(candidate.variant) {
    case Variant::Base:
        // Retail 0x22369E40: a classname that matches none of the three
        // weapon variants branches straight to the common inherited Spawn
        // sequence at 0x2236A0D4, then skips all per-weapon numeric fields.
        // The exact default classname is factory-authenticated separately.
        candidate.desired_field_3B0=current.original_field_3B0;
        output=candidate;
        return Code::Applied;
    case Variant::Pistol:
        candidate.desired_weapon="weapon_npc_pistol";
        candidate.desired_field_3B0=1;
        break;
    case Variant::Scattergun:
        candidate.desired_weapon="weapon_scattergun";
        candidate.desired_field_3B0=0;
        break;
    case Variant::AssaultRifle:
        candidate.desired_weapon="weapon_assault_rifle";
        candidate.desired_field_3B0=2;
        break;
    case Variant::Unsupported:
        return Code::UnsupportedClass;
    }
    // All three exact-original variants run their own ConVar +0x28
    // f32 through x87 FISTP with RC=11 (truncate toward zero).
    // A real x87 exception produces a special indefinite integer for
    // invalid inputs. We REFUSE these rather than invent that branch.
    const double f=static_cast<double>(current.variant_float_28);
    constexpr double kLow=-2147483648.0;
    constexpr double kHigh=2147483648.0;
    if(!std::isfinite(f) || f<kLow || f>=kHigh)
        return Code::NonFiniteOrOutOfRangeGlobal;
    candidate.truncated_global_integer=
        static_cast<std::int32_t>(std::trunc(f));
    candidate.overwrite_weapon=
        current.weapon_name!=candidate.desired_weapon;
    candidate.update_field_3B0=
        current.original_field_3B0!=candidate.desired_field_3B0;
    candidate.dirty_path=!candidate.update_field_3B0?
        DirtyPath::NoNetworkMarker:
        current.original_flag_1F0?DirtyPath::NativeField200:
        current.original_edict_16C_present?DirtyPath::EdictStateBit0:
        DirtyPath::NoNetworkMarker;
    candidate.update_field_98=
        current.original_field_98!=candidate.truncated_global_integer;
    candidate.update_field_9C=
        current.original_field_9C!=candidate.truncated_global_integer;
    output=candidate;
    return Code::Applied;
}

Result SpawnReference::apply(const Snapshot& current) noexcept {
    if(busy_)return {Code::Reentrant};
    struct RestoreBusy {
        bool& b;
        explicit RestoreBusy(bool& flag) noexcept:b(flag){b=true;}
        ~RestoreBusy() noexcept {b=false;}
    } guard(busy_);

    Result result{};
    result.code=plan_spawn(current,result.plan);
    if(result.code!=Code::Applied)return result;

    // The 1246-byte original virtual method distinguishes these stages.
    // Host refusal is fail-closed; do not pretend to atomically undo
    // unknown native original entity/netprop side effects.
    result.code=Code::HostRejected;
    try {
        if(result.plan.overwrite_weapon) {
            result.failed_stage=Stage::WeaponSymbol;
            if(!host_.assign_original_weapon_symbol(
                    result.plan.desired_weapon))return result;
        }
        if(result.plan.update_field_3B0) {
            result.failed_stage=Stage::Field3B0;
            if(!host_.commit_original_field_3B0(
                    result.plan.desired_field_3B0,
                    result.plan.dirty_path))return result;
        }
        result.failed_stage=Stage::Virtual68;
        if(!host_.native_virtual_68())return result;
        result.failed_stage=Stage::Virtual1CThen6C;
        if(!host_.native_virtual_1C_then_6C())return result;
        result.failed_stage=Stage::InheritedSpawn;
        if(!host_.inherited_npc_spawn_2235F2A0())return result;

        // The next original branch is class-specific and happens
        // AFTER the common inherited NPC Spawn, not in the factory.
        if(result.plan.update_field_98) {
            result.failed_stage=Stage::Field98;
            if(!host_.virtual_1B0_then_write_field_98(
                    result.plan.truncated_global_integer))return result;
        }
        if(result.plan.update_field_9C) {
            result.failed_stage=Stage::Field9C;
            if(!host_.virtual_1B4_then_write_field_9C(
                    result.plan.truncated_global_integer))return result;
        }
    } catch (...) {return result;}
    result.failed_stage=Stage::None;
    result.code=Code::Applied;
    return result;
}

} // namespace sin_compat::original_druglab_grunt::spawn
