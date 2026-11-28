#pragma once
// Phase1700: all 211 original 2006 PE32 static RecvTable property counts,
// all 1110 original static RecvProp type DWORDs and all 207 native type5
// constructor routes. THIS IS SOURCE METADATA, NOT ORIGINAL PE BYTES.
//
// Table order: 191 exact original VClient011.slot5 class-list nodes,
// then 20 original source-VA sorted auxiliary RecvTables.
// Counts are 211 bytes hex; type values 0..5 are 1110 single hex nibbles.
// Type5 links: 192 original static child RecvTable edges (source manifest),
// 14 dynamic generic nested table allocation paths, and one 621-byte
// specialized animation overlay path (not runnable/implemented).
//
// 211 native count bytes SHA256:
// ee7010aab35e8e31381069f56e19113de7e8bad292c902137ef999b17390a204
// 1110 unpacked native type bytes SHA256:
// 4db1788356cca573eb6a21250cedfe785b700be738434dd08adf50f0acad2ec0
// 555 packed nibble bytes SHA256:
// b0feb09024519c08d0c6d95c3e0ff73ed9bad4bc821798241b3a93f6bd1dec4b
//
// Never interpret source-VA labels as real Android/Windows pointers.
#include <array>
#include <cstddef>
#include <cstdint>
#include "sin_compat/original_client_static_recvtable_links.hpp"

namespace sin_compat::original_client_native32_recvtable_manifest {
namespace links=sin_compat::original_client_static_recvtable_links;

inline constexpr std::size_t kStaticTables=211u;
inline constexpr std::size_t kStaticPropertySlots=1110u;
inline constexpr std::size_t kStaticClassTables=191u;
inline constexpr std::size_t kStaticAuxiliaryTables=20u;
inline constexpr std::size_t kGeneratedGenericSubtables=14u;
inline constexpr std::size_t kGeneratedGenericSubtableProps=691u;
inline constexpr std::size_t kNativeTypeFiveProps=207u;
inline constexpr std::size_t kOriginalRecvPropStride=60u;
inline constexpr std::size_t kOriginalRecvTableBytes=20u;
inline constexpr std::size_t kOriginalClientClassPrefixBytes=20u;

// SHA-pinned original source table property counts.
inline constexpr char kOriginalTableCountsHex[]=
"010101010101010101020101010101010101010101010101010101010101010202030206051102010303052303030103"
"050405030302060601010101020102030201030a0b050b0001050209030702020305020403041101030301020a070801"
"0202040b010c030102040b020a0505060802040908040f0718020302031202071a100108070603080504050205040804"
"05030205060f060509030209060707040c0709030403030303050f01020d121303080610130d0502010305050204000b"
"04050201050102010201311b010814030f000d";

// Numeric original native RecvProp DWORD at descriptor byte +4.
inline constexpr char kOriginalPropTypesHex[]=
"555555555525555555555555555555555050500505000025210052200110001111410515503520501015222222222222"
"222222222222222222222150150255035001151125000050155050510110550111555551553522555500511111121050"
"011111100500225242401011155222011512201111511511000050555330030450020051152010000000001001120005"
"015015505110011000555555551110000550515333000000022015500311111105500551511200110000005515001122"
"005511105110051110053414100515050511111110500101115111555000001000043550211150220000000110000005"
"050051550555005002000151500000055050000000000404011111111100012020500000100020000145511002015550"
"000021110520522010005201050025201052522105200521112005111522105005052200520110522221110000000522"
"000500005010020002205552000011152200052201015220101520052111220001052000002200000000245211500522"
"500500500220000111010000005225111111100000511111101102212212512211111111100010150151211110511111"
"512211111110010151221111111110001015040010000002511102255025221150000555000000011000111101000000"
"010111051500005001000011112220001002000200110020011122222222000511115000000111204021111101022000"
"122111111220110000000011111121111122222101110000000110";

// Exactly fourteen actual native type5 array-wrapper source properties.
// Triples [parent static table ordinal, property slot, shape ordinal]:
// shapes 0..10 are original 11 class arrays; shapes 11..13 are three
// source auxiliary arrays (weapons, area-bits, ammo).
inline constexpr char kGenericNestedOuterHex[]=
"5d00005d01015d02025d03035d04045d05055d06066c09077701087d08097d0a0a"
"c6010bca000ccb050d";

constexpr std::uint8_t HexNibble(char c) noexcept {
    if(c>='0'&&c<='9')return static_cast<std::uint8_t>(c-'0');
    if(c>='a'&&c<='f')return static_cast<std::uint8_t>(c-'a'+10);
    return 0xFFu;
}
constexpr std::uint8_t HexByte(char a,char b) noexcept {
    return static_cast<std::uint8_t>(HexNibble(a)*16u+HexNibble(b));
}
template<std::size_t N>
constexpr bool AllHex(const char (&s)[N]) noexcept {
    for(std::size_t i=0;i<N-1;++i)
        if(HexNibble(s[i])>15u)return false;
    return true;
}
constexpr std::array<std::uint8_t,kStaticTables> DecodeCounts() {
    std::array<std::uint8_t,kStaticTables> result{};
    for(std::size_t i=0;i<result.size();++i)
        result[i]=HexByte(kOriginalTableCountsHex[i*2u],
                          kOriginalTableCountsHex[i*2u+1u]);
    return result;
}
constexpr std::array<std::uint8_t,kStaticPropertySlots> DecodeTypes() {
    std::array<std::uint8_t,kStaticPropertySlots> result{};
    for(std::size_t i=0;i<result.size();++i)
        result[i]=HexNibble(kOriginalPropTypesHex[i]);
    return result;
}
struct GeneratedOuter final {
    std::uint8_t parentTableOrdinal;
    std::uint8_t parentSlot;
    std::uint8_t sourceShapeIndex;
};
constexpr std::array<GeneratedOuter,kGeneratedGenericSubtables>
DecodeGenerated() {
    std::array<GeneratedOuter,kGeneratedGenericSubtables> result{};
    for(std::size_t i=0;i<result.size();++i) {
        const auto at=i*6u;
        result[i]={HexByte(kGenericNestedOuterHex[at],
                           kGenericNestedOuterHex[at+1]),
                   HexByte(kGenericNestedOuterHex[at+2],
                           kGenericNestedOuterHex[at+3]),
                   HexByte(kGenericNestedOuterHex[at+4],
                           kGenericNestedOuterHex[at+5])};
    }
    return result;
}
inline constexpr auto kCounts=DecodeCounts();
inline constexpr auto kTypes=DecodeTypes();
inline constexpr auto kGeneratedOuters=DecodeGenerated();

constexpr std::array<std::size_t,kStaticTables+1u> Prefixes() {
    std::array<std::size_t,kStaticTables+1u> out{};
    for(std::size_t i=0;i<kStaticTables;++i)
        out[i+1]=out[i]+kCounts[i];
    return out;
}
inline constexpr auto kTypePrefixes=Prefixes();
constexpr bool EveryNativeTypeFiveRouteSourceClassified() noexcept {
    if(kTypePrefixes[kStaticTables]!=kStaticPropertySlots)
        return false;
    std::array<std::uint8_t,kStaticPropertySlots> allocated{};
    std::size_t marked=0;
    for(const auto& edge:links::kOriginalSourceEdges) {
        if(edge.parentTableOrdinal>=kStaticTables ||
           edge.parentPropertySlot>=kCounts[edge.parentTableOrdinal])
            return false;
        const auto idx=kTypePrefixes[edge.parentTableOrdinal]+
                       edge.parentPropertySlot;
        if(kTypes[idx]!=5u || allocated[idx]++)return false;
        ++marked;
    }
    for(const auto& edge:kGeneratedOuters) {
        if(edge.parentTableOrdinal>=kStaticTables ||
           edge.parentSlot>=kCounts[edge.parentTableOrdinal] ||
           edge.sourceShapeIndex>=kGeneratedGenericSubtables)
            return false;
        const auto idx=kTypePrefixes[edge.parentTableOrdinal]+edge.parentSlot;
        if(kTypes[idx]!=5u || allocated[idx]++)return false;
        ++marked;
    }
    // Actual source DT_OverlayVars.m_AnimOverlay: table ordinal
    // 191 + auxiliary VA-sorted position6 = 197; slot0, native type5.
    constexpr std::size_t overlayTable=197u;
    constexpr std::size_t overlayPropSlot=0u;
    const auto overlayIdx=kTypePrefixes[overlayTable]+overlayPropSlot;
    if(kCounts[overlayTable]==0u || kTypes[overlayIdx]!=5u ||
       allocated[overlayIdx]++)return false;
    ++marked;
    if(marked!=kNativeTypeFiveProps)return false;
    for(std::size_t i=0;i<kTypes.size();++i)
        if((kTypes[i]==5u)!=(allocated[i]!=0u))return false;
    return true;
}
constexpr bool SourceCardinality() noexcept {
    if(kTypePrefixes[kStaticTables]!=kStaticPropertySlots)return false;
    std::array<std::size_t,6> counts{};
    for(const auto type:kTypes) {
        if(type>=6)return false;
        ++counts[type];
    }
    return counts[0]==414u && counts[1]==300u &&
           counts[2]==164u && counts[3]==12u &&
           counts[4]==13u && counts[5]==207u;
}
inline constexpr bool CanExposeToRetailEngine() noexcept {return false;}
static_assert(sizeof(kOriginalTableCountsHex)-1u==kStaticTables*2u);
static_assert(sizeof(kOriginalPropTypesHex)-1u==kStaticPropertySlots);
static_assert(sizeof(kGenericNestedOuterHex)-1u==kGeneratedGenericSubtables*6u);
static_assert(AllHex(kOriginalTableCountsHex));
static_assert(AllHex(kOriginalPropTypesHex));
static_assert(AllHex(kGenericNestedOuterHex));
static_assert(SourceCardinality());
static_assert(EveryNativeTypeFiveRouteSourceClassified());
} // namespace sin_compat::original_client_native32_recvtable_manifest
