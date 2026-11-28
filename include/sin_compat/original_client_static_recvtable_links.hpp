#pragma once
// Phase1699: source-authenticated ORIGINAL 2006 PE32 192 static nested
// RecvTable links, across all 211 unique statically constructed tables.
// These metadata triples are NOT live Source SDK pointers or recv proxies.
//
// Ordinals 0..190: exact real VClient011.slot5 class-list head-to-tail
// order from original_client_class_catalog_phase1696.tsv.
// Ordinals 191..210: original 20 auxiliary RecvTables sorted by their
// native source VA in original_client_aux_recvtable_metadata.hpp.
//
// Each 3-byte record is [PARENT TABLE ORDINAL, PARENT PROPERTY SLOT,
// CHILD TABLE ORDINAL], stored as six human-inspectable hex digits.
// All 191 class-attached ordinary type5 links, plus the single genuine
// DT_LocalPlayerExclusive.m_Local auxiliary type5 ordinary link.
// Explicitly EXCLUDES 14 dynamic arrays and one specialized overlay,
// which create their child RecvTables at runtime in original code.
//
// SHA256 of exactly 576 raw triple bytes (whole original client
// source registry and nested target audit, not proprietary PE bytes):
// 1057160b9ff24500569300bbee8589c0cccb5328f7984399b44ca5fcf82958e7
#include <array>
#include <cstddef>
#include <cstdint>

namespace sin_compat::original_client_static_recvtable_links {
inline constexpr std::size_t kOriginalClassTableCount=191u;
inline constexpr std::size_t kOriginalAuxTableCount=20u;
inline constexpr std::size_t kOriginalStaticTableCount=211u;
inline constexpr std::size_t kOriginalStaticNestedEdges=192u;
inline constexpr std::size_t kOriginalOrdinaryClassEdges=191u;
inline constexpr std::size_t kOriginalOrdinaryAuxEdges=1u;
inline constexpr std::size_t kOriginalDynamicGenericNestedWrappers=14u;
inline constexpr std::size_t kOriginalSpecialOverlayNestedWrapper=1u;
inline constexpr std::size_t kOriginalNativeTypeFiveProps=207u;
inline constexpr std::size_t kOriginalStaticGraphMaxDepth=9u;

struct Edge final {
    std::uint8_t parentTableOrdinal;
    std::uint8_t parentPropertySlot;
    std::uint8_t childTableOrdinal;
};

// ORIGINAL 192 source edge triples, source-authenticated ordinal ordering.
inline constexpr char kSourceEdgesHex[]=
"00003901004102003a03004104004105004106004107003a08004109007a0a00390b00410c00410d00410e003a0f0041"
"10004111004112004113003b14001515004116003a1700841800411900411a00411b00411c00411d00841e007d1f00b5"
"20006021007d22007d2300b524008f25007826007827007d28004d2900782a007f2b007f2c007f2d007f2e002d2f004d"
"30007831007832007833007834007d3401d035007d3600783700763701d23800783900413a003b3b00413c007f3d0082"
"3e00823f008f4000474001d141008442007843004444007845004646007d4800784900784b00784c00784d00784e00b5"
"4f00784f01cc5000785200aa5400785500785800605900615a005b5b00755c00785e00785f007d600075610078620078"
"640078650082650bcd66007867007868007869008f6a0acd6b00786c00786d00786e00786f007870007871007d720082"
"7202cf73007874007875007d76007a7601cb7602c977007b7800c77813cd7815c87900787a00777a01c67b007d7b01c5"
"7c00787d00787d11c37e00787f007a8019c282007883007a84007d8401c18402c08600be8700be8800be89008f8a00be"
"8b008f8c00be8d00be8e00be8f00be9000be91008f9200be9300be9400be9500be9600be9700be98008f9a00be9a01ce"
"9b00be9c00be9d00be9e00be9f00bea000bea1008fa400aaa500aaa600aaa700aaa800aaa900aaab00acad0082ae0082"
"af0082b00082b10082b20082b30082b40082b5007db60082b80082b9008fba0078bb0078bc0078bc01bfbd0078cb00ca";

constexpr std::uint8_t HalfByte(char v) noexcept {
    if(v>='0'&&v<='9')return static_cast<std::uint8_t>(v-'0');
    if(v>='a'&&v<='f')return static_cast<std::uint8_t>(v-'a'+10);
    return 0xFFu;
}
constexpr std::uint8_t ByteAt(std::size_t at) noexcept {
    return static_cast<std::uint8_t>(HalfByte(kSourceEdgesHex[at])*16u+
                                     HalfByte(kSourceEdgesHex[at+1]));
}
constexpr bool EncodedSourceWellFormed() noexcept {
    if(sizeof(kSourceEdgesHex)-1u!=kOriginalStaticNestedEdges*6u)
        return false;
    for(std::size_t i=0u;i<sizeof(kSourceEdgesHex)-1u;++i)
        if(HalfByte(kSourceEdgesHex[i])>15u)return false;
    return true;
}
constexpr std::array<Edge,kOriginalStaticNestedEdges> Decode() noexcept {
    std::array<Edge,kOriginalStaticNestedEdges> out{};
    for(std::size_t i=0;i<out.size();++i) {
        const auto pos=i*6u;
        out[i]={ByteAt(pos),ByteAt(pos+2u),ByteAt(pos+4u)};
    }
    return out;
}
inline constexpr auto kOriginalSourceEdges=Decode();

constexpr bool SourceCardinalityValid() noexcept {
    if(kOriginalSourceEdges.size()!=kOriginalStaticNestedEdges)
        return false;
    std::size_t ordinaryClass=0,ordinaryAux=0;
    for(const auto& edge:kOriginalSourceEdges) {
        if(edge.parentTableOrdinal>=kOriginalStaticTableCount ||
           edge.childTableOrdinal>=kOriginalStaticTableCount)
            return false;
        if(edge.parentTableOrdinal<kOriginalClassTableCount)
            ++ordinaryClass;
        else
            ++ordinaryAux;
    }
    return ordinaryClass==kOriginalOrdinaryClassEdges &&
           ordinaryAux==kOriginalOrdinaryAuxEdges;
}
inline constexpr bool CanExposeToRetailEngine() noexcept {return false;}
static_assert(EncodedSourceWellFormed(),"original edge triples malformed");
static_assert(SourceCardinalityValid(),"original 192 static source edges drifted");
static_assert(kOriginalSourceEdges[0].parentTableOrdinal==0u);
static_assert(kOriginalSourceEdges[0].childTableOrdinal==57u);
static_assert(kOriginalSourceEdges[191].parentTableOrdinal==203u);
static_assert(kOriginalSourceEdges[191].childTableOrdinal==202u);
static_assert(kOriginalStaticNestedEdges+kOriginalDynamicGenericNestedWrappers+
              kOriginalSpecialOverlayNestedWrapper==kOriginalNativeTypeFiveProps);
} // namespace sin_compat::original_client_static_recvtable_links
