// Phase 490: source-verified original ShadowMgr Init event order.
// Original game renderer and callback registration NOT invoked.
#include "client_shadow_original_init_owned.h"
#include <cstdio>

namespace s=sin_independent::shadow_original_init;
static int Fail(int n){
    std::fprintf(stderr,"ShadowMgr original owned Init fixture failed %d\n",n);
    return n;
}
int main(){
    static_assert(s::kOriginalInitBytes==146,"source Init length");
    static_assert(s::ValidateEvidence(),"source material/slot contract");
    const s::Plan disabled=s::MakePlan(false);
    if(disabled.actions!=8 || disabled.optional_refresh_requested ||
       !disabled.original_returns_true)return Fail(1);
    const auto& a=disabled.ordered;
    if(a[0].operation!=s::Operation::ClearObjectByteCd ||
       a[0].field_or_virtual!=0xCD ||
       a[1].operation!=s::Operation::InitializeOwnedMaterialRef ||
       a[1].field_or_virtual!=0x14 ||
       a[1].text0!="decals/simpleshadow" ||
       a[1].text1!="Decal textures" ||
       a[2].operation!=s::Operation::CallObjectVirtual24Vector ||
       a[2].field_or_virtual!=24 ||
       a[2].arg0!=0x3DCCCCCDu ||
       a[2].arg1!=0x3DCCCCCDu ||
       a[2].arg2!=0xBF800000u ||
       a[3].operation!=s::Operation::CallObjectVirtual27Float ||
       a[3].field_or_virtual!=27 ||
       a[3].arg0!=0x42480000u ||
       a[4].operation!=s::Operation::CallObjectVirtual28Float ||
       a[4].field_or_virtual!=28 ||
       a[4].arg0!=0x3BA3D70Au ||
       a[5].operation!=s::Operation::SetObjectDwordD8ToEight ||
       a[5].field_or_virtual!=0xD8 ||
       a[5].arg0!=8 ||
       a[6].operation!=s::Operation::RegisterCallbackViaExternalVirtual116 ||
       a[6].field_or_virtual!=116 ||
       a[6].arg0!=0x24115E70u ||
       a[7].operation!=s::Operation::OriginalReturnTrue ||
       a[7].arg0!=1)return Fail(2);

    const s::Plan enabled=s::MakePlan(true);
    if(enabled.actions!=9 || !enabled.optional_refresh_requested ||
       enabled.ordered[6].operation!=
           s::Operation::ConditionalRefresh0x24118610 ||
       enabled.ordered[6].field_or_virtual!=0x24118610u ||
       enabled.ordered[7].operation!=
           s::Operation::RegisterCallbackViaExternalVirtual116 ||
       enabled.ordered[8].operation!=s::Operation::OriginalReturnTrue)
        return Fail(3);
    s::OwnedModel model{};
    if(model.flag_cd!=0xFFu ||
       s::ApplyPlanToOwnedModel(disabled,model)!=
            s::Result::AppliedOwnedModel ||
       model.flag_cd!=0 ||
       model.value_d8!=8 ||
       !model.material_reference_described ||
       !model.vector_configuration_described ||
       !model.float27_described ||
       !model.float28_described ||
       !model.callback_registration_described ||
       model.optional_refresh_described)
        return Fail(4);
    if(s::ApplyPlanToOwnedModel(enabled,model)!=
           s::Result::AppliedOwnedModel ||
       !model.optional_refresh_described)return Fail(5);

    // A caller must not turn a *guessed* alternative configuration
    // into a successful owned Init merely by retaining valid counts.
    const s::OwnedModel prior=model;
    s::Plan forged=enabled;
    forged.ordered[2].arg2=0x3F800000u; // change -1.0 to +1.0
    if(s::ApplyPlanToOwnedModel(forged,model)!=s::Result::PlanRefused ||
       model.flag_cd!=prior.flag_cd ||
       model.optional_refresh_described!=prior.optional_refresh_described)
        return Fail(6);
    forged=enabled;
    forged.ordered[1].text0="decals/unknown";
    if(s::ApplyPlanToOwnedModel(forged,model)!=s::Result::PlanRefused)
        return Fail(7);
    forged=enabled;
    forged.ordered[7].arg0=0;
    if(s::ApplyPlanToOwnedModel(forged,model)!=s::Result::PlanRefused)
        return Fail(8);
    forged=enabled;
    forged.actions=8;
    if(s::ApplyPlanToOwnedModel(forged,model)!=s::Result::PlanRefused)
        return Fail(9);
    forged=disabled;
    forged.optional_refresh_requested=true;
    if(s::ApplyPlanToOwnedModel(forged,model)!=s::Result::PlanRefused)
        return Fail(10);
    std::puts("PASS source-derived ShadowMgr Init: original material, 3 virtual settings, two field writes, conditional helper, 116-slot callback, return true and forged-plan refusal");
    return 0;
}
