#pragma once
// Phase1699: original 2006 PE32 20 aux RecvTables include three further
// dynamically constructed type5 child arrays, with 112 child RecvProps.
// Source evidence is SHA-pinned by independent owner-original audit.
// All integer/byte offsets here are original x86 SOURCE METADATA only.
//
// Source 0x24126E90 initializes each CHILD RecvProp+0x2C to zero,
// stride, 2*stride,...; the OUTER type5 property carries object base.
// Source 0x241271B0 animation overlay uses a DIFFERENT 621-byte helper.
// Source 0x24126BD0 type4 viewmodel does NOT create a child RecvTable.
//
// This immutable host-owned C++17 representation CANNOT be published as
// the native engine's ClientClass or mutable RecvTable pointer graph.
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>
#include <vector>

namespace sin_compat::original_client_aux_dynamic_receivers {

inline constexpr std::size_t kGenericNestedArrayCount=3u;
inline constexpr std::size_t kGenericGeneratedElementCount=112u;
inline constexpr std::uint32_t kGenericNestedOriginalFactoryVA=0x24126E90u;
inline constexpr std::uint32_t kOverlayOriginalFactoryVA=0x241271B0u;
inline constexpr std::uint32_t kArray4OriginalFactoryVA=0x24126BD0u;
inline constexpr std::size_t kOriginalRecvPropStride=60u;
inline constexpr std::uint32_t kNativeOuterNestedTableType=5u;
inline constexpr std::uint32_t kNativeElementIntegerType=0u;
inline constexpr std::uint32_t kNativeArrayType=4u;

struct Shape final {
    std::string_view auxTable;
    std::string_view networkProperty;
    std::uint32_t elements;
    std::uint32_t byteStride;
    std::uint32_t originalOuterParentX86Offset;
    std::uint32_t originalElementType;
};

// Source IMM32 arguments from exact original caller windows:
//   0x240C37CE (48 x4, outer 0xC80),
//   0x240D3334 (32 x1, outer +4),
//   0x240D39A1 (32 x4, outer 0xC00).
inline constexpr std::array<Shape,kGenericNestedArrayCount> kSourceShapes{{
    {"DT_BCCLocalPlayerExclusive","m_hMyWeapons",48,4,0xC80u,0},
    {"DT_Local","m_chAreaBits",32,1,4,0},
    {"DT_LocalPlayerExclusive","m_iAmmo",32,4,0xC00u,0},
}};

// DISTINCT from those three type5 nested-table constructors:
// source native 0x24126BD0 emits a type4 2-by-4 descriptor in
// DT_LocalPlayerExclusive.m_hViewModel; no new RecvTable is created.
struct Type4DescriptorFacts final {
    std::string_view auxTable="DT_LocalPlayerExclusive";
    std::string_view networkProperty="m_hViewModel";
    std::uint32_t nativeType=kNativeArrayType;
    std::uint32_t count=2u;
    std::uint32_t byteWidth=4u;
    std::uint32_t sourceDescriptorVA=0x2433A328u;
};

// Source original 0x241271B0 consumes count=15, element stride 0x20,
// outer member offset 0x7A4. It allocates count+1=16 * 60 descriptor
// bytes and uses specialized callbacks 0x24127150/0x24127180.
// Its live proxy/ownership semantics are NOT emulated here.
struct OverlayFacts final {
    std::string_view auxTable="DT_OverlayVars";
    std::string_view networkProperty="m_AnimOverlay";
    std::uint32_t sourceRequestedCount=15u;
    std::uint32_t sourceAllocatedDescriptors=16u;
    std::uint32_t byteStride=32u;
    std::uint32_t originalOuterParentX86Offset=0x7A4u;
    std::uint32_t originalSourceProxyA=0x24127150u;
    std::uint32_t originalSourceProxyB=0x24127180u;
    std::uint32_t nativeOuterType=kNativeOuterNestedTableType;
};
inline constexpr Type4DescriptorFacts kSourceType4ViewModel{};
inline constexpr OverlayFacts kSourceOverlay{};

struct Element final {
    std::uint32_t index=0u;
    // The true native RecvProp DWORD at +44 in the GENERATED child table.
    std::uint32_t originalChildRelativeMemberOffset=0u;
    // DERIVED only for diagnostics, never stored to native child +44.
    std::uint32_t derivedParentObjectX86Offset=0u;
    std::uint32_t originalNativeElementType=0u;
    std::array<char,4> originalName{{'0','0','0','\0'}};
};
struct BuiltMetadata final {
    bool valid=false;
    std::vector<Element> elements;
    bool CanExposeToRetailEngine() const noexcept {return false;}
};

inline const Shape* FindShape(std::string_view table,
                              std::string_view property) noexcept {
    for(const auto& item:kSourceShapes)
        if(item.auxTable==table && item.networkProperty==property)
            return &item;
    return nullptr;
}

inline BuiltMetadata Build(const Shape& input) {
    BuiltMetadata result{};
    const Shape* source=FindShape(input.auxTable,input.networkProperty);
    if(source==nullptr || source->elements!=input.elements ||
       source->byteStride!=input.byteStride ||
       source->originalOuterParentX86Offset!=
           input.originalOuterParentX86Offset ||
       source->originalElementType!=input.originalElementType ||
       input.elements==0u || input.elements>256u ||
       (input.byteStride!=1u && input.byteStride!=4u) ||
       input.originalElementType!=kNativeElementIntegerType)
        return result;
    const auto max=std::numeric_limits<std::uint32_t>::max();
    if(input.elements>max/kOriginalRecvPropStride ||
       (input.elements-1u)>
           (max-input.originalOuterParentX86Offset)/input.byteStride)
        return result;
    result.elements.reserve(input.elements);
    for(std::uint32_t i=0u;i<input.elements;++i) {
        const std::array<char,4> name{{
            static_cast<char>('0'+(i/100u)),
            static_cast<char>('0'+((i/10u)%10u)),
            static_cast<char>('0'+(i%10u)), '\0'
        }};
        result.elements.push_back({
            i,
            i*input.byteStride,
            input.originalOuterParentX86Offset+i*input.byteStride,
            input.originalElementType,
            name
        });
    }
    result.valid=true;
    return result;
}
inline constexpr bool CanExposeToRetailEngine() noexcept {return false;}
static_assert(kSourceShapes.size()==kGenericNestedArrayCount);
static_assert(kSourceShapes[0].elements+kSourceShapes[1].elements+
              kSourceShapes[2].elements==kGenericGeneratedElementCount);
static_assert(kSourceOverlay.sourceAllocatedDescriptors==
              kSourceOverlay.sourceRequestedCount+1u);
static_assert(kSourceType4ViewModel.nativeType!=kNativeOuterNestedTableType);
} // namespace sin_compat::original_client_aux_dynamic_receivers
