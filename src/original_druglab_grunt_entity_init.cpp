#include "sin_compat/original_druglab_grunt_entity_init.hpp"

namespace sin_compat::original_druglab_grunt::entity_init {
Result EntityInitializer::initialize(const Input& input) noexcept {
    if(busy_)return {Code::Reentrant};
    struct RestoreBusy {
        bool& b;
        explicit RestoreBusy(bool& value) noexcept:b(value){b=true;}
        ~RestoreBusy() noexcept{b=false;}
    } guard(busy_);
    Result result{};
    try {
        if(input.has_classname_argument) {
            result.failed_stage=Stage::ClassnameSymbol;
            if(!host_.intern_native_classname(input.classname))
                return result;
        }

        // Original CBaseEntity +0x70 helper 0x2213B140 tests bit9 of
        // DWORD this+0xB8 FIRST. Its special path skips edict attach
        // but still calls the two base-initialization helpers.
        if((input.native_flags_B8&0x200u)!=0u) {
            result.special_bit9=true;
            result.skipped_edict_attachment=true;
            result.failed_stage=Stage::SpecialBit9;
            if(!host_.special_bit9_native_registration())
                return result;
        } else {
            // Original bit10 skip condition. If clear, networkable
            // subobject owner+0x160 receives the existing global edict
            // context or requests a new engine edict when null.
            result.skipped_edict_attachment=
                (input.native_flags_B8&0x400u)!=0u;
            if(!result.skipped_edict_attachment) {
                result.failed_stage=Stage::AttachNetworkable;
                bool attached=false;
                try {
                    attached=host_.attach_native_networkable(
                        input.factory_edict_context_present);
                } catch (...) {}
                result.attachment_invoked=true;
                // Clear the temporary current-edict global even when
                // host attachment fails; original code only reaches
                // clear after the native helper returns normally.
                result.failed_stage=attached?
                    Stage::ClearEdictContext:Stage::AttachNetworkable;
                bool cleared=false;
                try {cleared=host_.clear_original_factory_edict_context();}
                catch (...) {}
                if(!attached||!cleared)return result;
            }

            result.failed_stage=Stage::BacklinkEdict;
            if(host_.has_live_native_edict()) {
                if(!host_.link_edict_networkable_backpointer())
                    return result;
                result.edict_backlink_written=true;
            }
        }

        result.failed_stage=Stage::BaseInitBF30;
        if(!host_.base_initialize_2214BF30_zero())return result;
        result.failed_stage=Stage::BaseInitB280;
        if(!host_.base_initialize_2214B280())return result;
        // The wrapper 0x22031B10 invokes this NPC-specific call
        // AFTER base +0x70 returns, even along the early bit9 path.
        result.failed_stage=Stage::GruntPostInit3F8;
        if(!host_.invoke_real_grunt_postinit_slot3F8())
            return result;
    } catch (...) {return result;}
    result.failed_stage=Stage::None;
    result.code=Code::Applied;
    return result;
}
} // namespace sin_compat::original_druglab_grunt::entity_init
