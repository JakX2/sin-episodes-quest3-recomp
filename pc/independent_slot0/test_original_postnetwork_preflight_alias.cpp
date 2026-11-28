// Phase1680: native Win32 x86 original-free CDECL preflight RET0
// (real original 0x24197F50, 121B whole SHA) feeding recovered
// Phase1679 recursive record-name lookup 0x24197ED0.
//
// The original writes node+0x10 visited and record+0x28 alias;
// Phase1672 dispatcher then reads record+0x28 to propagate the
// sequence to its alias record's +0x2C. All records/nodes are
// synthetic, authorized and never access retail game memory.
#include "native_original_postnetwork_preflight_alias.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1680 requires native MSVC Windows x86
#endif
namespace pf=sin_independent::original_postnetwork_preflight_alias;
namespace names=sin_independent::original_postnetwork_record_name_lookup;
namespace sw=sin_independent::original_postnetwork_type_switch;
namespace {
int fail(int code,const char* why) {
    std::fprintf(stderr,
      "FAIL Phase1680 native preflight/alias x86 #%d: %s\n",
      code,why);
    return code;
}
constexpr char kType0Name[]="alpha";
constexpr char kMainAliasName[]="Alpha";
constexpr char kGroupName[]="group";
constexpr char kMissingName[]="Missing";
constexpr char kNestedAliasName[]="Nested";
constexpr char kSideAlphaName[]="ALPHA";
constexpr char kSideNestedName[]="nested";
constexpr char kSideOtherName[]="other";
unsigned lower_ascii(unsigned c) noexcept {
    return c>='A'&&c<='Z' ? c+32u:c;
}
int compare_ascii(void* user,const char* lhs,const char* rhs) noexcept {
    auto* n=static_cast<unsigned*>(user);
    if(!n || !lhs || !rhs)return 1;
    ++*n;
    for(unsigned i=0u;i<128u;++i) {
        const auto a=lower_ascii(static_cast<unsigned char>(lhs[i]));
        const auto b=lower_ascii(static_cast<unsigned char>(rhs[i]));
        if(a!=b)return a<b?-1:1;
        if(a==0u)return 0;
    }
    return 1;
}
struct Data {
    std::array<sw::Record,4> main{};
    std::array<sw::Record,1> child{};
    std::array<sw::Record,3> next{};
    pf::NodeBlock nmain{};
    pf::NodeBlock nchild{};
    pf::NodeBlock nnext{};
};
void record(sw::Record& r,std::uint32_t type,const char* name,
            std::uint8_t flags=0u) noexcept {
    r.raw.fill(0u);
    r.write32(0u,type);
    r.write32(4u,names::rawptr(name));
    r.raw[0x12u]=flags;
    r.write32(sw::kOriginalRecordSequenceOffset,0u);
}
void setup(Data& d,pf::Context& pre,names::Context& finder) noexcept {
    record(d.main[0],0u,kType0Name,0x80u);
    record(d.main[1],8u,kMainAliasName,0x80u);
    record(d.main[2],10u,kGroupName,0x80u);
    record(d.main[3],9u,kMissingName,0x80u);
    record(d.child[0],8u,kNestedAliasName,0x80u);
    record(d.next[0],9u,kSideAlphaName);
    record(d.next[1],8u,kSideNestedName);
    record(d.next[2],7u,kSideOtherName);
    d.main[2].write32(
        pf::kOriginalRecordNestedOffset,
        names::rawptr(&d.nchild.base));
    d.main[0].write32(pf::kOriginalRecordAliasOffset,0x11223344u);
    d.main[2].write32(pf::kOriginalRecordAliasOffset,0x55667788u);
    d.main[3].write32(pf::kOriginalRecordAliasOffset,0xDEADBEEFu);
    d.nmain.base.word0=names::rawptr(d.main.data());
    d.nmain.base.word4=4u;
    d.nmain.base.unknown8=0x13572468u;
    d.nmain.base.next=&d.nnext.base;
    d.nchild.base.word0=names::rawptr(d.child.data());
    d.nchild.base.word4=1u;
    d.nchild.base.unknown8=0x24681357u;
    d.nchild.base.next=&d.nnext.base;
    d.nnext.base.word0=names::rawptr(d.next.data());
    d.nnext.base.word4=3u;
    d.nnext.base.unknown8=0xAABBCCDDu;
    d.nnext.base.next=nullptr;
    pre.authorize(&d.nmain,d.main.data(),4u);
    pre.authorize(&d.nchild,d.child.data(),1u);
    pre.authorize(&d.nnext,d.next.data(),3u);
    finder.authorize_node(&d.nmain.base,d.main.data(),4u);
    finder.authorize_node(&d.nchild.base,d.child.data(),1u);
    finder.authorize_node(&d.nnext.base,d.next.data(),3u);
    finder.authorize_name(kType0Name);
    finder.authorize_name(kMainAliasName);
    finder.authorize_name(kGroupName);
    finder.authorize_name(kMissingName);
    finder.authorize_name(kNestedAliasName);
    finder.authorize_name(kSideAlphaName);
    finder.authorize_name(kSideNestedName);
    finder.authorize_name(kSideOtherName);
}
bool preflight(pf::NodeBlock* node) noexcept {
    std::uintptr_t before=0u,after=0u;
    __asm mov before,esp
    pf::NativePreflight::Run(node);
    __asm mov after,esp
    return before==after;
}
struct DispatchTrace {
    Data* data=nullptr;
    unsigned aliased=0u;
    unsigned bad=0u;
    unsigned calls=0u;
};
void update_alias(void* raw,sw::Record* current,
                  std::uint32_t alias,std::uint32_t sequence) noexcept {
    auto* t=static_cast<DispatchTrace*>(raw);
    if(!t || !t->data || current!=&t->data->main[1] ||
       alias!=names::rawptr(&t->data->next[0])) {
        if(t)++t->bad;
        return;
    }
    ++t->aliased;
    t->data->next[0].write32(
        sw::kOriginalRecordSequenceOffset,sequence);
}
void on_case(void* raw,std::uint8_t,std::uint32_t,
             sw::Record*,std::uint32_t,std::uint32_t,
             std::int16_t) noexcept {
    auto* t=static_cast<DispatchTrace*>(raw);
    if(t)++t->calls;
}
bool dispatch(sw::Fixture& owner,std::uint32_t sequence,
              sw::Node* node,sw::Record* records,
              std::int32_t count) noexcept {
    std::uintptr_t before=0u,after=0u;
    __asm mov before,esp
    owner.Dispatch(sequence,node,records,count);
    __asm mov after,esp
    return before==after;
}
} // namespace
int main() {
    Data d{};
    pf::Context pre{};
    names::Context finder{};
    unsigned comparisons=0u;
    finder.comparator_user=&comparisons;
    finder.compare=&compare_ascii;
    pre.finder=&finder;
    setup(d,pre,finder);
    names::ScopedContext names_scope(finder);
    pf::ScopedContext pre_scope(pre);

    // Native source: mark current +0x10 first, then scan records.
    // Two aliases are found in next node, missing alias retains
    // old value, type0 and type10 never run lookup directly.
    if(!preflight(&d.nmain) ||
       d.nmain.visited!=1u || d.nchild.visited!=1u ||
       d.nnext.visited!=1u ||
       pre.processed!=3u || pre.type0_skips!=1u ||
       pre.recursive_type10!=1u ||
       pre.alias_queries!=3u || pre.alias_writes!=2u ||
       pre.alias_misses!=1u ||
       d.main[1].read32(0x28u)!=names::rawptr(&d.next[0]) ||
       d.child[0].read32(0x28u)!=names::rawptr(&d.next[1]) ||
       d.main[3].read32(0x28u)!=0xDEADBEEFu ||
       d.main[0].read32(0x28u)!=0x11223344u ||
       d.main[2].read32(0x28u)!=0x55667788u ||
       pre.already_visited_next_stops==0u)
        return fail(1,"source visited->nested->name lookup->alias fill");

    // Re-running on already visited FIRST node is allowed by
    // source; an already visited NEXT node stops traversal.
    const auto processed_before=pre.processed;
    const auto visited_stops_before=pre.already_visited_next_stops;
    if(!preflight(&d.nmain) ||
       pre.processed!=processed_before+2u ||
       pre.already_visited_next_stops<=visited_stops_before)
        return fail(2,"already-visited first still processes, next skips");

    // Without flag0x80 the original doesn't touch alias even when
    // the same name exists downstream. Other records retain behavior.
    const auto alias_before=d.main[1].read32(0x28u);
    d.main[1].raw[0x12u]=0u;
    const auto lookups_before=pre.alias_queries;
    if(!preflight(&d.nmain) ||
       d.main[1].read32(0x28u)!=alias_before ||
       pre.alias_queries!=lookups_before+2u)
        return fail(3,"source record+0x12 flag0x80 independently gated");
    d.main[1].raw[0x12u]=0x80u;

    // Signed node+4 count -1 still sets visited byte and follows
    // next without dereferencing a forged record-array pointer.
    pf::NodeBlock negative{};
    negative.base.word0=0xDEADBEEFu;
    negative.base.word4=0xFFFFFFFFu;
    negative.base.next=&d.nnext.base;
    d.nnext.visited=0u;
    if(!pre.authorize(&negative,nullptr,0u) ||
       !finder.authorize_node(&negative.base,nullptr,0u) ||
       !preflight(&negative) ||
       negative.visited!=1u || d.nnext.visited!=1u)
        return fail(4,"negative count visited and next following");
    negative.visited=0u;
    negative.base.word4=0u;
    d.nnext.visited=0u;
    if(!preflight(&negative) ||
       negative.visited!=1u || d.nnext.visited!=1u)
        return fail(5,"zero count visited and next following");

    // Unknown node/array pointers must not be dereferenced in
    // synthetic CI; retail x86 would have undefined behavior.
    const auto failed_before=pre.refused_unowned;
    const auto previous_word0=d.nmain.base.word0;
    d.nmain.base.word0=0xBAD00BADu;
    if(!preflight(&d.nmain) ||
       pre.refused_unowned!=failed_before+1u)
        return fail(6,"forged record buffer not safely refused");
    d.nmain.base.word0=previous_word0;
    const auto refused_next=pre.refused_unowned;
    pf::NodeBlock unknown{};
    if(!preflight(&unknown) ||
       pre.refused_unowned!=refused_next+1u)
        return fail(7,"unknown first node not rejected");

    // Connect actual record+0x28 alias to the Phase1672 native
    // dispatcher callback which changes the matched record's
    // record+0x2C sequence. The later selected record must skip
    // comparison on the same sequence.
    d.main[0].write32(0x28u,0u);
    d.main[2].write32(0x28u,0u);
    d.main[3].write32(0x28u,0u);
    d.child[0].write32(0x28u,0u);
    sw::Fixture owner{};
    DispatchTrace t{};
    t.data=&d;
    owner.ops.user=&t;
    owner.ops.update_alias_sequence=&update_alias;
    owner.ops.on_opaque_case=&on_case;
    owner.left_index=0u; owner.right_index=1u;
    owner.mode=0u;
    if(!dispatch(owner,42u,&d.nmain.base,d.main.data(),4) ||
       t.aliased!=1u || t.bad ||
       d.next[0].read32(sw::kOriginalRecordSequenceOffset)!=42u ||
       owner.events.alias_updates!=1u)
        return fail(8,"preflight alias pointer -> native typed sequence write");
    const auto prev_skips=owner.events.repeated_sequence_skips;
    if(!dispatch(owner,42u,&d.nnext.base,d.next.data(),3) ||
       owner.events.repeated_sequence_skips!=prev_skips+1u ||
       t.bad)
        return fail(9,"alias-target record skipped on matching sequence");

    // Original next-node visited flag acts as a stop signal.
    // Root itself may still be processed; no second node is read.
    d.nnext.visited=1u;
    d.nmain.visited=0u;
    d.nchild.visited=0u;
    const auto pre_count=pre.processed;
    if(!preflight(&d.nmain) || pre.processed!=pre_count+2u)
        return fail(10,"already visited next must stop without processing");

    // Safety divergence for source's unbounded type10 recursion:
    // point record+0x20 back to own node; explicit depth limit.
    const auto original_child=d.main[2].read32(0x20u);
    d.main[2].write32(0x20u,names::rawptr(&d.nmain.base));
    const auto old_budget=pre.refused_budget;
    if(!preflight(&d.nmain) ||
       pre.refused_budget<=old_budget)
        return fail(11,"original type10 recursive cycle must be bounded");
    d.main[2].write32(0x20u,original_child);

    std::puts("PASS Phase1680 original-free MSVC x86: 121B source "
              "preflight cdecl RET0 node+0x10 visited flag, "
              "0x34 record stride/type0 skip/type10 recursion, "
              "record flag0x80 + node next name lookup -> "
              "conditional record+0x28 alias write, safe cycles, "
              "and native typed dispatcher alias sequence propagation; "
              "zero retail native game execution");
    return 0;
}
