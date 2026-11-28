#pragma once
// Phase1697: original 2006 PE32 0x24126E90 builds an OUTER RecvProp of
// type 5 (nested table) from a source element RecvProp template.
// It dynamically allocates count * 60-byte elements, copies a template
// to each, updates member offset at each element +0x2C by byte stride,
// and allocates an original 20-byte child RecvTable.
//
// This C++17 implementation ONLY constructs safe HOST METADATA, never
// an original Windows RecvProp, RecvTable, factory pointer or callback.
// The 11 entry shapes were authenticated from original CALL operands.
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>
#include <vector>

namespace sin_compat::original_client_nested_array_table {

inline constexpr std::size_t kOriginalElementDescriptorBytes=60u;
inline constexpr std::size_t kOriginalNestedRecvTableBytes=20u;
inline constexpr std::size_t kSourceArrayShapeCount=11u;
inline constexpr std::size_t kOriginalTotalGeneratedElements=579u;
inline constexpr std::uint32_t kOriginalOuterNativeType=5u;
inline constexpr std::uint32_t kOriginalIntegerElementType=0u;
inline constexpr std::uint32_t kOriginalFloatElementType=1u;
inline constexpr std::size_t kSourceElementOffsetField=44u;
inline constexpr std::uint32_t kOriginalElementNamePointerTableVA=0x242fb110u;
inline constexpr std::size_t kOriginalElementNamePointerCount=256u;
// Original complete 1024-byte PE32 name-pointer array SHA256:
// 7f419caa89c32a637e4f7b7b81f247e232fcc55d1673721e6a3c9dd46d040506
// Each original pointer dereferences to a name from "000" to "255".

struct Shape final {
    std::string_view clientClass;
    std::string_view networkProperty;
    std::uint32_t elements;
    std::uint32_t byteStride;
    std::uint32_t firstX86Offset;
    std::uint32_t elementNativeType;
};

// Original instructions (not SDK-derived array bounds):
// source client VA 0x240EB857..0x240EBA1C, 0x240DD5DA,
// 0x240CE418, 0x240B2354 and 0x240B23B8.
// Real original owner DLL SHA256 must be validated by Python auditor.
inline constexpr std::array<Shape,kSourceArrayShapeCount> kOriginalShapes{{
    {"CPlayerResource","m_iPing",33,4,2212,0},
    {"CPlayerResource","m_iScore",33,4,2344,0},
    {"CPlayerResource","m_iDeaths",33,4,2476,0},
    {"CPlayerResource","m_bConnected",33,1,2608,0},
    {"CPlayerResource","m_iTeam",33,4,2644,0},
    {"CPlayerResource","m_bAlive",33,1,2776,0},
    {"CPlayerResource","m_iHealth",33,4,2812,0},
    {"CBreakableSurface","m_RawPanelBitVec",256,1,1236,0},
    {"CBaseFlex","m_flexWeight",64,4,2144,1},
    {"CBaseAnimating","m_flPoseParameter",24,4,1352,1},
    {"CBaseAnimating","m_flEncodedController",4,4,1644,1},
}};

struct Element final {
    std::uint32_t elementIndex=0;
    std::uint32_t originalX86MemberOffset=0;
    std::uint32_t nativeType=0;
    // Source-authenticated 3-digit original indexed RecvProp name,
    // always NUL terminated. This is NOT a native pointer.
    std::array<char,4> originalElementName{{'0','0','0','\0'}};
};

struct BuiltMetadata final {
    bool valid=false;
    std::uint32_t outerNativeType=kOriginalOuterNativeType;
    std::vector<Element> elements;
    bool CanExposeToRetailEngine() const noexcept {return false;}
};

inline const Shape* FindShape(std::string_view owner,
                              std::string_view name) noexcept {
    for(const auto& item:kOriginalShapes)
        if(item.clientClass==owner && item.networkProperty==name)
            return &item;
    return nullptr;
}

// Accept only an EXACT source-observed shape, not an arbitrary inferred
// std::vector length or a guessed pointer. Bounds protect native offsets,
// count * 60 allocation and pointer lifetime on Android/desktop alike.
inline BuiltMetadata Build(const Shape& input) {
    BuiltMetadata out{};
    const Shape* source=FindShape(input.clientClass,input.networkProperty);
    if(source==nullptr ||
       source->elements!=input.elements ||
       source->byteStride!=input.byteStride ||
       source->firstX86Offset!=input.firstX86Offset ||
       source->elementNativeType!=input.elementNativeType ||
       input.elements==0 || input.elements>256u ||
       (input.byteStride!=1u && input.byteStride!=4u) ||
       (input.elementNativeType!=0u && input.elementNativeType!=1u))
        return out;
    const auto maximum=std::numeric_limits<std::uint32_t>::max();
    if(input.elements>maximum/kOriginalElementDescriptorBytes)
        return out;
    if(input.elements>1u &&
       (input.elements-1u)>(maximum-input.firstX86Offset)/input.byteStride)
        return out;
    out.elements.reserve(input.elements);
    for(std::uint32_t i=0u;i<input.elements;++i) {
        const std::array<char,4> originalName{{
            static_cast<char>('0'+(i/100u)),
            static_cast<char>('0'+((i/10u)%10u)),
            static_cast<char>('0'+(i%10u)),
            '\0'
        }};
        out.elements.push_back({
            i,
            input.firstX86Offset+i*input.byteStride,
            input.elementNativeType,
            originalName
        });
    }
    out.valid=true;
    return out;
}

} // namespace sin_compat::original_client_nested_array_table
