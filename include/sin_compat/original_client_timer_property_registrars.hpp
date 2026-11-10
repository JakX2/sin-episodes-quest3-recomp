#pragma once
// Phase162: independent C++17 metadata model of seven exact 2006
// SiN Episodes client.dll CHudBreathTimer / CHudPoisonTimer VGUI property
// registration functions. Native class registry and parser not ported.
// Every name, default, original field resolver and 0/1 kind is a checked
// original 2006 x86 program-data fact, not a modern SDK guess.
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace sin_compat::original_client_timer_properties {
enum class TimerClass : std::uint8_t { Breath, Poison };
enum class PropertyKind : std::uint8_t { Color=0, String=1 };

struct Property {
    TimerClass owner;
    std::string_view owner_name;
    std::string_view external_name;
    std::string_view member_name;
    std::string_view native_type;
    std::string_view native_default;
    PropertyKind native_kind_byte;
    std::uint32_t registration_function_va;
    std::uint32_t registration_guard_va;
    std::uint32_t native_field_accessor_va;
    std::uint32_t panel_relative_field_offset;
    std::uint32_t complete_object_field_offset;
};

// Byte-verified registration metadata and original field offset helpers.
// Full object offset = secondary Panel displacement +0x14 plus panel
// relative field offset. The color is four bytes and ends at Panel+0xA6.
// Native TextureName properties use type "string", not a texture pointer.
inline constexpr std::array<Property,7> kOriginalProperties{{
 {TimerClass::Breath,"CHudBreathTimer","BreathTimerColor",
  "m_breathTimerColor","Color","0 0 0 0",PropertyKind::Color,
  0x24230DE0u,0x2437FAA6u,0x242309F0u,0xA3u,0xB7u},
 {TimerClass::Breath,"CHudBreathTimer","BreathBarForeground",
  "m_breathBarForegroundTextureName","string","Breath_bar_foreground",
  PropertyKind::String,0x24230E50u,0x2437FAA7u,0x24230A10u,0xA8u,0xBCu},
 {TimerClass::Breath,"CHudBreathTimer","BreathBarBackground",
  "m_breathBarBackgroundTextureName","string","Breath_bar_background",
  PropertyKind::String,0x24230EC0u,0x2437FAA8u,0x24230A30u,0xC9u,0xDDu},
 {TimerClass::Poison,"CHudPoisonTimer","PoisonTimerColor",
  "m_poisonTimerColor","Color","255 255 255 0",PropertyKind::Color,
  0x24236450u,0x2437FC82u,0x24236030u,0xA3u,0xB7u},
 {TimerClass::Poison,"CHudPoisonTimer","MutagenBarForeground",
  "m_mutagenBarForegroundTextureName","string","Mutagen_bar_foreground",
  PropertyKind::String,0x242364C0u,0x2437FC83u,0x24236050u,0xA8u,0xBCu},
 {TimerClass::Poison,"CHudPoisonTimer","MutagenBarBackground",
  "m_mutagenBarBackgroundTextureName","string","Mutagen_bar_background",
  PropertyKind::String,0x24236530u,0x2437FC84u,0x24236070u,0xC9u,0xDDu},
 {TimerClass::Poison,"CHudPoisonTimer","MutagenBarInactive",
  "m_mutagenBarInactiveTextureName","string","Mutagen_bar_inactive",
  PropertyKind::String,0x242365A0u,0x2437FC85u,0x24236090u,0xEAu,0xFEu},
}};
static_assert(kOriginalProperties[0].native_kind_byte==PropertyKind::Color,
              "Original Color registration uses kind byte 0");
static_assert(kOriginalProperties[3].native_default=="255 255 255 0",
              "Poison Color native default is NOT Breath's default");
static_assert(kOriginalProperties[6].complete_object_field_offset==0xFEu,
              "Last Poison/Mutagen property lives at complete +0xFE");

// Original registrar branch: compare one per-property global guard byte
// with zero; nonzero => return immediately. On zero, WRITE guard=1
// BEFORE calling the native property registry (0x24011DC0 and
// 0x2400A300). Host callback models only one registration *event*;
// actual descriptor creation, parsing and VGUI side effects remain
// native engine behavior and are deliberately not fabricated.
struct RegistryState {
    std::array<std::uint8_t,7> one_time_flags{};
};
template<class ObservePropertyRegistration>
bool register_original_once(RegistryState& state,std::size_t index,
                            ObservePropertyRegistration&& on_register) {
    if(index>=kOriginalProperties.size())return false;
    if(state.one_time_flags[index]!=0)return false;
    state.one_time_flags[index]=1; // original store BEFORE callback
    on_register(kOriginalProperties[index]);
    return true;
}
template<class ObservePropertyRegistration>
std::size_t register_original_class_properties(
    RegistryState& state,TimerClass owner,
    ObservePropertyRegistration&& on_register) {
    std::size_t count=0;
    for(std::size_t index=0;index<kOriginalProperties.size();++index) {
        if(kOriginalProperties[index].owner!=owner)continue;
        if(register_original_once(state,index,on_register))++count;
    }
    return count;
}

// Original offsets returned by the partly overlapping compiler-generated
// resolver+constant helper pairs for secondary TextureName properties.
// Native x86 0x24230A30 and 0x24236070 fall through to a constant getter
// after a NULL input, so NULL returns the complete object offset rather
// than zero. This helper is deliberately DIFFERENT from the previous
// +0xA3/+0xA8 resolver functions, which return zero on NULL.
inline std::uint32_t original_late_resource_resolver(
    std::uint32_t original_pointer,std::uint32_t panel_offset) noexcept {
    return original_pointer!=0U ? original_pointer+panel_offset
                                : panel_offset+0x14U;
}
} // namespace sin_compat::original_client_timer_properties
