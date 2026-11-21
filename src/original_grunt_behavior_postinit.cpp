#include "sin_compat/original_grunt_behavior_postinit.hpp"

namespace sin_compat::original_druglab_grunt::behavior_postinit {
Result Registrar::initialize() noexcept {
    if (busy_) return {Code::Reentrant};
    struct Guard {
        bool& busy;
        explicit Guard(bool& b) noexcept : busy(b) { busy=true; }
        ~Guard() noexcept {busy=false;}
    } guard(busy_);
    Result result{};
    try {
        for (std::size_t i=0; i<kOriginalBehaviors.size(); ++i) {
            const Spec spec=kOriginalBehaviors[i];
            result.behavior_index=static_cast<std::uint8_t>(i);
            result.failed_operation=Operation::AppendToCollection;
            if (!host_.append_to_collection(spec,kBehaviorArrayOffset,
                                            kBehaviorArrayCountField)) return result;
            result.failed_operation=Operation::BindNPC;
            if (!host_.bind_to_npc(spec)) return result;
            result.failed_operation=Operation::StoreOwnerSubobject;
            if (!host_.set_owner_subobject_reference(spec,
                    kBehaviorLinkOwnerSubobject,kBehaviorLinkFieldWithinBehavior)) return result;
            ++result.registered_count;
        }
        result.behavior_index=static_cast<std::uint8_t>(kOriginalBehaviors.size());
        result.failed_operation=Operation::BindSentenceOwner;
        if (!host_.bind_sentence_owner(kEmbeddedSentenceStateOffset,
                                       kSentenceOwnerPointerField)) return result;
        result.failed_operation=Operation::InitializeSentenceParameters;
        if (!host_.initialize_sentence_parameters(kEmbeddedSentenceStateOffset,
                                                   kOriginalSentenceKey)) return result;
    } catch (...) {return result;}
    result.code=Code::Success;
    result.failed_operation=Operation::None;
    return result;
}
} // namespace sin_compat::original_druglab_grunt::behavior_postinit