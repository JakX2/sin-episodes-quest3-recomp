#include "sin_compat/fixed_metric_bridge_phase664.hpp"
#include <cstring>
#include <limits>

namespace sin_compat::difficulty::fixed_phase664 {
namespace {
bool image_valid(std::uint32_t base) noexcept {
    return base!=0u && (base&0xfffu)==0u &&
           base<=std::numeric_limits<std::uint32_t>::max()-kOriginalImageSize;
}
bool bits_equal(float a,float b) noexcept {
    std::uint32_t lhs=0u,rhs=0u;
    std::memcpy(&lhs,&a,sizeof lhs);
    std::memcpy(&rhs,&b,sizeof rhs);
    return lhs==rhs;
}
class BusyGuard {
public:
    explicit BusyGuard(bool& busy) noexcept:flag_(busy){busy=true;}
    ~BusyGuard() noexcept {flag_=false;}
private:
    bool& flag_;
};
}
std::uint32_t metric_va(std::uint32_t base,std::size_t index) noexcept {
    if(!image_valid(base)||index>=kOriginalMetricEntries)return 0u;
    return base+kOriginalMetricTableRva+
           static_cast<std::uint32_t>(index)*kOriginalRecordStride;
}
std::uint32_t name_pointer_va(std::uint32_t base,std::size_t index) noexcept {
    if(!image_valid(base)||index>=kOriginalMetricEntries)return 0u;
    return base+kOriginalNameTableRva+
           static_cast<std::uint32_t>(index)*kOriginalNamePointerStride;
}
OriginalMetricBridge::OriginalMetricBridge(IOriginalGameHost& host,
        fixed_phase663::FixedDifficultyRuntime& policy,
        std::uint32_t base) noexcept
    :host_(host),policy_(policy),module_base_(base){}

bool OriginalMetricBridge::original_names_valid(BindResult& result) noexcept {
    result.stage=Stage::ValidateNames;
    if(!image_valid(module_base_)){
        result.code=BindCode::InvalidImageBase;return false;
    }
    const auto max_va=module_base_+kOriginalImageSize;
    try {
        for(std::size_t i=0u;i<kOriginalMetricEntries;++i){
            result.failed_metric=i;
            std::uint32_t ptr=0u;
            if(!host_.read_u32(name_pointer_va(module_base_,i),ptr)){
                result.code=BindCode::NamePointerMissing;return false;
            }
            const auto wanted=original_metrics[i].name;
            if(ptr<module_base_ || ptr>=max_va ||
               wanted.size()+1u>static_cast<std::size_t>(max_va-ptr)){
                result.code=BindCode::NamePointerOutsideImage;return false;
            }
            for(std::size_t j=0u;j<=wanted.size();++j){
                std::uint8_t c=0u;
                if(!host_.read_u8(ptr+static_cast<std::uint32_t>(j),c)){
                    result.code=BindCode::NamePointerMissing;return false;
                }
                const unsigned char expected=j==wanted.size()?0u:
                    static_cast<unsigned char>(wanted[j]);
                if(c!=expected){
                    result.code=BindCode::NameMismatch;return false;
                }
            }
        }
    }catch(...){
        result.code=BindCode::HostException;return false;
    }
    result.failed_metric=kOriginalMetricEntries;
    return true;
}

void OriginalMetricBridge::try_rollback(
        BindResult& result,std::size_t inclusive_last,
        const std::array<float,kOriginalMetricEntries>& prior) noexcept {
    result.recovery_required=true;
    result.stage=Stage::RollbackMetric;
    if(inclusive_last>=kOriginalMetricEntries)inclusive_last=kOriginalMetricEntries-1u;
    for(std::size_t j=inclusive_last+1u;j>0u;--j){
        const auto i=j-1u;
        ++result.rollback_attempted;
        try{
            const auto address=metric_va(module_base_,i);
            float readback=0.f;
            if(host_.write_f32(address,prior[i]) &&
               host_.read_f32(address,readback) &&
               bits_equal(prior[i],readback))++result.rollback_verified;
        }catch(...){}
    }
}
BindResult OriginalMetricBridge::apply_skill(int requested_skill) noexcept {
    BindResult result{};
    result.module_base=module_base_;
    if(busy_){result.code=BindCode::Reentrant;return result;}
    if(!fixed_phase663::valid_skill(requested_skill)){
        result.code=BindCode::InvalidSkill;return result;
    }
    if(!image_valid(module_base_)){
        result.code=BindCode::InvalidImageBase;return result;
    }
    BusyGuard guard(busy_);
    if(!original_names_valid(result))return result;

    std::array<float,kOriginalMetricEntries> original{};
    std::size_t last_attempted=kOriginalMetricEntries;
    try {
        for(std::size_t i=0u;i<kOriginalMetricEntries;++i){
            result.stage=Stage::SnapshotCurrent;
            result.failed_metric=i;
            if(!host_.read_f32(metric_va(module_base_,i),original[i])){
                result.code=BindCode::CurrentValueUnreadable;return result;
            }
        }
        result.failed_metric=kOriginalMetricEntries;

        int current_adaptive=0;
        result.stage=Stage::ReadAdaptiveConvar;
        if(!host_.get_convar_int(fixed_phase663::kOriginalAdaptiveGateConVar,
                                 current_adaptive)){
            result.code=BindCode::ConvarReadFailure;return result;
        }
        result.old_adaptive_enabled=current_adaptive!=0;

        // Original 0x222BC47F conditionally skips an actual update call
        // on dd_enabled==0. We DO NOT assert that it disables all scoring.
        result.stage=Stage::DisableAdaptiveConvar;
        result.recovery_required=true;
        if(!host_.set_convar_int(fixed_phase663::kOriginalAdaptiveGateConVar,0)){
            result.code=BindCode::ConvarWriteFailure;return result;
        }
        result.stage=Stage::VerifyAdaptiveDisabled;
        int disabled=1;
        if(!host_.get_convar_int(fixed_phase663::kOriginalAdaptiveGateConVar,disabled)){
            result.code=BindCode::ConvarReadFailure;return result;
        }
        if(disabled!=0){
            result.code=BindCode::ConvarReadbackMismatch;return result;
        }
        result.adaptive_disabled=true;

        result.stage=Stage::ReadSkill;
        if(!host_.get_convar_int(fixed_phase663::kEngineSkillConVar,
                                 result.old_engine_skill)){
            result.code=BindCode::ConvarReadFailure;return result;
        }
        result.stage=Stage::SelectSkill;
        if(!host_.set_convar_int(fixed_phase663::kEngineSkillConVar,
                                 requested_skill)){
            result.code=BindCode::ConvarWriteFailure;return result;
        }
        result.stage=Stage::VerifySkill;
        int actual_skill=0;
        if(!host_.get_convar_int(fixed_phase663::kEngineSkillConVar,
                                 actual_skill)){
            result.code=BindCode::ConvarReadFailure;return result;
        }
        if(actual_skill!=requested_skill){
            result.code=BindCode::ConvarReadbackMismatch;return result;
        }
        result.skill_selected=true;

        const auto desired=fixed_phase663::make_profile(
            fixed_phase663::level_for_skill(requested_skill)).metrics;
        for(std::size_t i=0u;i<kOriginalMetricEntries;++i){
            result.failed_metric=i;
            result.stage=Stage::WriteCurrentMetric;
            last_attempted=i;
            ++result.attempted_writes;
            if(!host_.write_f32(metric_va(module_base_,i),desired[i])){
                result.code=BindCode::MetricWriteFailure;
                try_rollback(result,i,original);return result;
            }
            result.stage=Stage::VerifyCurrentMetric;
            float observed=0.f;
            if(!host_.read_f32(metric_va(module_base_,i),observed)){
                result.code=BindCode::MetricReadbackFailure;
                try_rollback(result,i,original);return result;
            }
            if(!bits_equal(observed,desired[i])){
                result.code=BindCode::MetricReadbackMismatch;
                try_rollback(result,i,original);return result;
            }
            ++result.confirmed_writes;
        }

        // Publish the policy state ONLY after every real guest slot has
        // been independently confirmed. The original adaptive savegame may
        // have overwritten all fourteen members; unchanged skill still
        // republishes all fields here.
        (void)policy_.reapply_after_archive(requested_skill);
        result.code=BindCode::Completed;
        result.stage=Stage::None;
        result.failed_metric=kOriginalMetricEntries;
        result.published=true;
        result.recovery_required=false;
    }catch(...){
        result.code=BindCode::HostException;
        if(last_attempted<kOriginalMetricEntries){
            try_rollback(result,last_attempted,original);
        }
    }
    return result;
}
} // namespace sin_compat::difficulty::fixed_phase664
