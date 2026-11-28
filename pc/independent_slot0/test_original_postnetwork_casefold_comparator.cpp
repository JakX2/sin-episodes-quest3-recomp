// Phase1681: original-free native Win32 MSVC x86 cdecl name
// comparison integrates Phase1679 recursive finder and Phase1680
// alias preflight without a generic strcmp stand-in.
//
// Retail exact whole SHA original 67B 0x2425F81B and 78B ASCII
// fast path 0x24261C60. Source locale nonzero calls a separate
// 0x2424C373 mapping routine that is NOT reproduced or loaded.
// This test injects a synthetic locale mapping, strictly distinct
// from retail code, and grants only invented safe string pointers.
#include "native_original_postnetwork_casefold_comparator.h"
#include "native_original_postnetwork_preflight_alias.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1681 requires native MSVC Win32 x86
#endif
namespace cmp=sin_independent::original_postnetwork_casefold_comparator;
namespace names=sin_independent::original_postnetwork_record_name_lookup;
namespace pf=sin_independent::original_postnetwork_preflight_alias;
namespace sw=sin_independent::original_postnetwork_type_switch;
namespace {
int fail(int number,const char* why) {
    std::fprintf(stderr,
      "FAIL Phase1681 native name comparator x86 #%d: %s\n",
      number,why);
    return number;
}
struct LocaleTrace {
    std::array<std::uint8_t,32> bytes{};
    unsigned count=0u;
};
std::int32_t map_invented_locale(void* user,
                                 std::uint8_t c) noexcept {
    auto* trace=static_cast<LocaleTrace*>(user);
    if(trace && trace->count<trace->bytes.size())
        trace->bytes[trace->count++]=c;
    // Invented non-retail mapping used ONLY to prove the
    // original comparator's separately injected locale path.
    if(c==0xC4u)return 0xE4;
    return static_cast<std::int32_t>(cmp::ascii_fold(c));
}
constexpr char kUpper[]="ALPHA";
constexpr char kLower[]="alpha";
constexpr char kA[]="A";
constexpr char kB[]="B";
constexpr char kEmpty[]="";
constexpr char kUtfUpper[]="\xC4";
constexpr char kUtfLower[]="\xE4";
constexpr char kGroup[]="Group";
constexpr char kNested[]="Nested";
constexpr char kNestedKey[]="nEsTeD";
constexpr char kBinary[]="BINARY";
constexpr char kBinaryPeer[]="binary";
constexpr char kBinaryKey[]="bInArY";
constexpr char kMissing[]="missing";
constexpr char kNetworkLabel[]="network";

struct FixtureData {
    std::array<sw::Record,2> root{};
    std::array<sw::Record,1> nested{};
    std::array<sw::Record,1> next{};
    pf::NodeBlock nroot{};
    pf::NodeBlock nnested{};
    pf::NodeBlock nnext{};
};
void record(sw::Record& r,unsigned type,const char* name,
            std::uint8_t flags=0u) noexcept {
    r.raw.fill(0u);
    r.write32(0u,type);
    r.write32(4u,names::rawptr(name));
    r.raw[0x12u]=flags;
    r.write32(0x28u,0u);
    r.write32(sw::kOriginalRecordSequenceOffset,0u);
}
bool build(FixtureData& d,names::Context& n,pf::Context& pre) noexcept {
    record(d.root[0],10u,kGroup);
    record(d.root[1],9u,kBinary,0x80u);
    record(d.nested[0],7u,kNested);
    record(d.next[0],8u,kBinaryPeer);
    d.nroot.base.word0=names::rawptr(d.root.data());
    d.nroot.base.word4=2u;
    d.nroot.base.next=&d.nnext.base;
    d.nnested.base.word0=names::rawptr(d.nested.data());
    d.nnested.base.word4=1u;
    d.nnested.base.next=nullptr;
    d.nnext.base.word0=names::rawptr(d.next.data());
    d.nnext.base.word4=1u;
    d.nnext.base.next=nullptr;
    d.root[0].write32(
        pf::kOriginalRecordNestedOffset,
        names::rawptr(&d.nnested.base));
    return n.authorize_node(&d.nroot.base,d.root.data(),2u) &&
           n.authorize_node(&d.nnested.base,d.nested.data(),1u) &&
           n.authorize_node(&d.nnext.base,d.next.data(),1u) &&
           n.authorize_name(kGroup) &&
           n.authorize_name(kNested) &&
           n.authorize_name(kBinary) &&
           n.authorize_name(kBinaryPeer) &&
           pre.authorize(&d.nroot,d.root.data(),2u) &&
           pre.authorize(&d.nnested,d.nested.data(),1u) &&
           pre.authorize(&d.nnext,d.next.data(),1u);
}
bool native_compare(const char* a,const char* b,
                    int expected) noexcept {
    std::uintptr_t before=0u,after=0u;
    __asm mov before,esp
    const auto value=cmp::NativeCompare::Compare(a,b);
    __asm mov after,esp
    return before==after && value==expected;
}
bool native_find(names::Context& n,const char* key,
                 sw::Node* start,sw::Record* expected) noexcept {
    n.authorized_key=key;
    std::uintptr_t before=0u,after=0u;
    __asm mov before,esp
    sw::Record* result=names::NativeLookup::Find(key,start);
    __asm mov after,esp
    return before==after && result==expected;
}
bool native_preflight(pf::NodeBlock* start) noexcept {
    std::uintptr_t before=0u,after=0u;
    __asm mov before,esp
    pf::NativePreflight::Run(start);
    __asm mov after,esp
    return before==after;
}
} // namespace
int main() {
    cmp::Context c{};
    const char* const approved[]={
        kUpper,kLower,kA,kB,kEmpty,kUtfUpper,kUtfLower,
        kGroup,kNested,kNestedKey,kBinary,kBinaryPeer,
        kBinaryKey,kMissing,
    };
    for(const char* s:approved)
        if(!c.authorize(s))return fail(1,"host string grants absent");
    cmp::ScopedContext scope(c);
    // Retail fast branch: source ASCII casefold; return -1/0/1
    // and no custom locale callback invocations.
    if(!native_compare(kUpper,kLower,0) ||
       !native_compare(kEmpty,kEmpty,0) ||
       !native_compare(kA,kB,-1) ||
       !native_compare(kB,kA,1) ||
       !native_compare(kUtfUpper,kUtfLower,-1) ||
       c.ascii_calls!=5u || c.locale_calls!=0u)
        return fail(2,"source no-locale fast path/ASCII sign/bytes");

    // With original global !=0, actual native method invokes
    // 0x2424C373 twice per byte, SECOND string first. An
    // invented mapping deliberately demonstrates this distinction.
    LocaleTrace lt{};
    c.locale_flag=true;
    c.locale_user=&lt;
    c.lower_in_locale=&map_invented_locale;
    if(!native_compare(kA,kB,1) ||
       lt.count!=2u || lt.bytes[0]!='B' || lt.bytes[1]!='A' ||
       c.locale_calls!=1u || c.locale_char_map_calls!=2u)
        return fail(3,"source locale mode order and second-minus-first sign");
    lt={};
    if(!native_compare(kUtfUpper,kUtfLower,0) ||
       c.locale_calls!=2u ||
       lt.count!=4u || lt.bytes[0]==0u)
        return fail(4,"injected locale mode differs from ASCII fast path");
    c.lower_in_locale=nullptr;
    const auto prev_missing=c.refused_missing_locale;
    if(!native_compare(kUpper,kLower,1) ||
       c.refused_missing_locale!=prev_missing+1u)
        return fail(5,"unknown original CRT locale helper refused");
    c.locale_flag=false;

    // Neither branch may dereference unowned names, even if a
    // forged pointer would crash the 2006 original game.
    const auto old_refused=c.refused_unowned;
    if(!native_compare(
           reinterpret_cast<const char*>(0xBAADF00Du),kUpper,1) ||
       c.refused_unowned!=old_refused+1u)
        return fail(6,"forged original pointer was accessed");
    std::array<char,128> unterm{};
    unterm.fill('X');
    if(!c.authorize(unterm.data()))return fail(7,"long host name grant");
    const auto before_unterm=c.refused_unterminated;
    if(!native_compare(unterm.data(),kLower,1) ||
       c.refused_unterminated!=before_unterm+1u)
        return fail(8,"bounded host-only NUL terminator check");

    // Swap Phase1679 injected generic ASCII comparator for the
    // just-built actual native __cdecl two-argument path.
    FixtureData d{};
    names::Context n{};
    pf::Context pre{};
    pre.finder=&n;
    if(!build(d,n,pre))
        return fail(9,"synthetic original 0x34 record/nodes not granted");
    n.compare=&cmp::CompareForFinder;
    n.comparator_user=nullptr;
    names::ScopedContext name_scope(n);
    pf::ScopedContext pre_scope(pre);

    // Direct native finder calls now actually execute the
    // Phase1681 native comparator CDECL method in its callback.
    if(!native_find(n,kNestedKey,&d.nroot.base,&d.nested[0]) ||
       !native_find(n,kBinaryKey,&d.nroot.base,&d.root[1]) ||
       !native_find(n,kMissing,&d.nroot.base,nullptr) ||
       n.nested_calls==0u || c.ascii_calls<=5u)
        return fail(10,"whole recursive name finder uses new cdecl comparer");

    // Source preflight alias path uses the SAME original name
    // comparator while resolving root record+0x28 from next.
    if(!native_preflight(&d.nroot) ||
       d.root[1].read32(0x28u)!=names::rawptr(&d.next[0]) ||
       pre.alias_writes!=1u ||
       d.nroot.visited!=1u ||
       d.nnested.visited!=1u ||
       d.nnext.visited!=1u)
        return fail(11,"native cdecl preflight -> finder -> comparer -> alias");

    // Phase1679 selection still writes the exact selected record
    // address into source this+0x34, but now uses original fast
    // comparator rather than a made-up generic ASCII callback.
    names::IntegratedFixture owner{};
    owner.expected_index=7;
    owner.configured_key=kNestedKey;
    n.authorized_key=kNestedKey;
    std::uintptr_t before=0u,after=0u;
    __asm mov before,esp
    owner.SelectDiagnostic(kNetworkLabel,7u,&d.nroot.base);
    __asm mov after,esp
    if(before!=after ||
       owner.diagnostic_record!=names::rawptr(&d.nested[0]) ||
       owner.diagnostic_label_word!=names::rawptr(kNetworkLabel))
        return fail(12,"selected this+0x34 via audited comparator chain");

    // Nonzero locale enables a distinct, injected CRT-helper
    // route during BOTH finder and preflight; original code's
    // external locale tables remain unmodeled, not invented.
    c.locale_flag=true;
    c.lower_in_locale=&map_invented_locale;
    lt={};
    n.authorized_key=kBinaryKey;
    if(!native_find(n,kBinaryKey,&d.nroot.base,&d.root[1]) ||
       c.locale_calls<3u ||
       c.locale_char_map_calls<=2u)
        return fail(13,"finder selectable original locale conversion path");
    c.locale_flag=false;

    std::puts("PASS Phase1681 original-free native Win32 x86: "
              "whole-SHA original 67B CDECL locale-gated record "
              "comparator, 78B ASCII fast path, true compiler RET0 "
              "stack semantics, unknown CRT char-map isolated, "
              "fast vs injected locale/equality/nonzero-sign, "
              "pointer grants, recursive original name finder, "
              "preflight 0x80 alias match and native selected-record "
              "this+0x34 chain, zero original game code execution");
    return 0;
}
