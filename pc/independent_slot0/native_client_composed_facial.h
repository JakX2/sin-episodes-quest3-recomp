#pragma once
#include "native_client_facial_frame.h"
#include "native_client_vfe_phoneme.h"
#include "native_client_flex_rule.h"
namespace sin_port::native_weapon_tables {
inline constexpr char client_composed_facial_export[]="SiN_GetClientComposedFacialApi";
struct ClientComposedFacialInput {
    bool studio_present,rules_enabled,smoothing_enabled,adaptive;
    float now,blink_duration,delay,window,dt;
    const ClientFacialController* controllers;unsigned controller_count;
    const ClientFacialFrameBindings* blink;
    const ClientVfePhonemeBindings* speech;
    const ClientFlexRule* rules;unsigned rule_count,descriptor_count;
    const float* previous;
};
struct ClientComposedFacialOutput {
    float global[128],current[128],smoothed[128];
    unsigned descriptor_count;
    bool descriptors_valid,smoothed_valid;
};
struct ClientComposedFacialApi {
    std::uint32_t version,size;
    // 0 invalid, 1 processed, 2 no studio header. Eye-target update and
    // renderer publication remain outside this numerical pipeline.
    int (__cdecl *run)(void*,unsigned,const ClientComposedFacialInput*,ClientComposedFacialOutput*) noexcept;
};
using GetClientComposedFacialApi=const ClientComposedFacialApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
