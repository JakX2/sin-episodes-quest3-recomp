#pragma once
// Phase1703: exact original 2006 PE32 native type-4 array RecvProp data.
// Original client.dll SHA256 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.
// Original helper 0x24126BD0 is 102 executable bytes (SHA256
// dbfd5065ab7af1f331c11bd90a892adac57943b63a8af72b06e29da18400d8cd).
// Constructor writes offset +44 ZERO, +48 stride, +52 element COUNT.
// It stores original native array callback at +28. The Team callback
// at ORIGINAL PE32 VA 0x240F76B0 is NOT executable in a recompilation.
// Do not confuse type4 arrays with type5 generated nested RecvTables.
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include "sin_compat/original_client_native32_recvtable_manifest.hpp"
namespace sin_compat::original_client_native_type4_array_fields {
namespace source=sin_compat::original_client_native32_recvtable_manifest;
inline constexpr std::uint32_t kOriginalArrayConstructorVA=0x24126BD0u;
inline constexpr std::uint32_t kOriginalSpecialTeamCallbackVA=0x240F76B0u;
inline constexpr std::size_t kSourceType4ArrayDescriptors=13u;
inline constexpr std::uint32_t kNativeArrayElementStrideOffset=48u;
inline constexpr std::uint32_t kNativeArrayElementCountOffset=52u;
inline constexpr std::uint32_t kNativeArrayCallbackOffset=28u;
struct ArrayDescriptor final {
    std::uint8_t tableOrdinal;
    std::uint8_t propertySlot;
    std::uint16_t elementCount;
    std::uint16_t elementStride;
    std::string_view originalPropertyName;
    bool nativeCallbackNotReconstructed;
};
// All 13 ACTUAL original source CALLs to 0x24126BD0, ordered by
// callsite, NOT fabricated table enumeration or class-list order.
inline constexpr std::array<ArrayDescriptor,kSourceType4ArrayDescriptors>
 kSourceArrays{{
    {181u,2u,3u,4u,"m_hPassengers",false},
    {163u,2u,16u,12u,"m_vecPoints",false},
    {129u,15u,24u,4u,"m_flPoseParameter",false},
    {128u,4u,10u,4u,"m_hAttachEntity",false},
    {128u,6u,10u,4u,"m_nAttachIndex",false},
    {118u,13u,2u,4u,"m_hObserverViewModel",false},
    {203u,17u,2u,4u,"m_hViewModel",false},
    {112u,2u,10u,255u,"m_iszOverlayNames",false},
    {112u,4u,10u,4u,"m_flOverlayTimes",false},
    {81u,4u,32u,0u,"\"player_array\"",true},
    {70u,2u,24u,12u,"m_ragAngles",false},
    {70u,4u,24u,12u,"m_ragPos",false},
    {37u,14u,3u,4u,"m_flFadeColor",false},
}};
constexpr bool SourceRoutesExactlyMatch() noexcept {
    std::size_t found=0u;
    std::size_t special=0u;
    for(const auto& array:kSourceArrays) {
        if(array.tableOrdinal>=source::kStaticTables ||
           array.propertySlot>=source::kCounts[array.tableOrdinal] ||
           array.elementCount==0u || array.elementCount>256u ||
           array.originalPropertyName.empty())return false;
        const auto index=source::kTypePrefixes[array.tableOrdinal]+array.propertySlot;
        if(source::kTypes[index]!=4u)return false;
        if(array.nativeCallbackNotReconstructed) {
            ++special;
            if(array.tableOrdinal!=81u || array.propertySlot!=4u ||
               array.elementStride!=0u)return false;
        }else if(array.elementStride==0u)return false;
        for(const auto& other:kSourceArrays)
            if(&array!=&other && array.tableOrdinal==other.tableOrdinal &&
               array.propertySlot==other.propertySlot)return false;
        ++found;
    }
    if(found!=kSourceType4ArrayDescriptors || special!=1u)return false;
    for(std::size_t t=0u;t<source::kStaticTables;++t)
        for(std::size_t p=0u;p<source::kCounts[t];++p)
            if(source::kTypes[source::kTypePrefixes[t]+p]==4u) {
                unsigned matches=0u;
                for(const auto& array:kSourceArrays)
                    if(array.tableOrdinal==t && array.propertySlot==p)++matches;
                if(matches!=1u)return false;
            }
    return true;
}
inline constexpr bool CanExposeToRetailEngine() noexcept {return false;}
static_assert(kSourceArrays.size()==13u);
static_assert(SourceRoutesExactlyMatch());
} // namespace sin_compat::original_client_native_type4_array_fields
