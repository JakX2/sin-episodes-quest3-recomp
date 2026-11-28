// Phase1674: original-free native Win32 x86 tests of the two
// source-authenticated PostNetwork conditional copy regions inside
// typed dispatcher cases8 and9.
//
// This does NOT claim two new original functions: the 41/43-byte
// segments are INLINE blocks in the original 1036-byte dispatcher.
// It checks their exact observable gate:
//   BYTE this+0x2C !=0 AND compare(EAX)!=1 -> copy RIGHT to LEFT
// with original case8 byte count N and case9 byte count 4*N.
//
// Actual Windows MSVC calls retain Phase1671 finalizer RET8,
// Phase1672 dispatcher RET16 and Phase1673 compare gate RET12.
// All pointers and buffers are invented and independently owned.
#include "native_original_postnetwork_case8_case9_bytecmp.h"
#include "native_original_postnetwork_diagnostic_core_finalizer.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1674 conditional typed byte copy test requires MSVC x86
#endif
namespace bc=sin_independent::original_postnetwork_case8_case9_bytecmp;
namespace sw=sin_independent::original_postnetwork_type_switch;
namespace final=sin_independent::original_postnetwork_diagnostic_core_finalizer;
namespace {
int fail(int id,const char* why) {
    std::fprintf(stderr,
      "FAIL Phase1674 original-free native typed correction x86 "
      "#%d: %s\n",id,why);
    return id;
}
std::uint32_t rawptr(const void* ptr) noexcept {
    return static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(ptr));
}
struct Trace {
    bc::ComparatorFixture* fixture=nullptr;
    std::array<std::uint8_t,40> left{};
    std::array<std::uint8_t,40> right{};
    std::array<std::uint32_t,16> cmp_result{};
    std::array<bool,16> copied{};
    std::array<std::uint32_t,16> byte_counts{};
    std::array<std::uint8_t,16> case_codes{};
    unsigned count=0u;
    unsigned bad=0u;
    unsigned nested_calls=0u;
    void reset() noexcept {
        count=bad=nested_calls=0u;
        cmp_result.fill(0u);
        copied.fill(false);
        byte_counts.fill(0u);
        case_codes.fill(0u);
    }
    void fill_separate() noexcept {
        for(unsigned i=0u;i<left.size();++i) {
            left[i]=static_cast<std::uint8_t>(0x12u+i*3u);
            right[i]=left[i];
        }
    }
};
void setup(sw::Record& r,std::uint32_t type,Trace& t,
           std::int16_t count,std::uint16_t flags=0u) noexcept {
    r.raw.fill(0u);
    r.write32(0u,type);
    r.write32(0x08u,rawptr(t.left.data()));
    r.write32(0x0Cu,rawptr(t.right.data()));
    r.write16(0x10u,static_cast<std::uint16_t>(count));
    r.write16(0x12u,flags);
    r.write32(sw::kOriginalRecordSequenceOffset,0u);
}
void on_case(void* user,std::uint8_t case_index,
             std::uint32_t type,sw::Record* record,
             std::uint32_t raw_left,std::uint32_t raw_right,
             std::int16_t flags) noexcept {
    auto* t=static_cast<Trace*>(user);
    if(!t->fixture || !record ||
       (case_index!=8u && case_index!=9u) ||
       case_index!=type ||
       raw_left!=rawptr(t->left.data()) ||
       raw_right!=rawptr(t->right.data()) ||
       static_cast<std::uint16_t>(flags)!=record->read16(0x12u)) {
        ++t->bad;
        return;
    }
    const auto signed_elements=
        static_cast<std::int16_t>(record->read16(0x10u));
    const auto signed_bytes=static_cast<std::int32_t>(
        signed_elements)*(case_index==9u?4:1);
    const auto n=static_cast<std::uint32_t>(signed_bytes);
    t->fixture->authorize_current_record(record);
    std::uintptr_t before=0u,after=0u;
    __asm mov before,esp
    const auto result=t->fixture->CompareBytes(
        n,t->left.data(),t->right.data());
    __asm mov after,esp
    if(before!=after)++t->bad;
    const auto wrote=t->fixture->MaybeApplyCase8Case9Correction(
        n,result,t->left.data(),t->right.data());
    if(t->count<t->case_codes.size()) {
        t->case_codes[t->count]=case_index;
        t->cmp_result[t->count]=result;
        t->copied[t->count]=wrote;
        t->byte_counts[t->count]=n;
    } else ++t->bad;
    ++t->count;
}
sw::DispatchOps ops_for(Trace& t) noexcept {
    sw::DispatchOps out{};
    out.user=&t;
    out.on_opaque_case=&on_case;
    return out;
}
bool dispatch(bc::ComparatorFixture& fixture,std::uint32_t seq,
              sw::Node& node,sw::Record* array,
              std::int32_t n) noexcept {
    std::uintptr_t before=0u,after=0u;
    __asm mov before,esp
    fixture.Dispatch(seq,&node,array,n);
    __asm mov after,esp
    return before==after;
}
struct Nested {
    Trace* trace=nullptr;
    bc::ComparatorFixture* fixture=nullptr;
    sw::Node* first=nullptr;
    sw::Node* second=nullptr;
    std::uint32_t sequence=0u;
};
void nested_finalizer(void* user,std::uint32_t seq,
                      sw::Node* node,std::uint32_t record_ptr,
                      std::uint32_t count) noexcept {
    auto* n=static_cast<Nested*>(user);
    ++n->trace->nested_calls;
    if(seq!=n->sequence ||
       (node!=n->first && node!=n->second) ||
       record_ptr!=node->word0 || count!=node->word4 ||
       !dispatch(*n->fixture,seq,*node,
          reinterpret_cast<sw::Record*>(
              static_cast<std::uintptr_t>(record_ptr)),
          static_cast<std::int32_t>(count)))
        ++n->trace->bad;
}
} // namespace
int main() {
    Trace t{};
    bc::ComparatorFixture fixture{};
    fixture.ops=ops_for(t);
    t.fixture=&fixture;
    fixture.unknown14=1u;
    fixture.left_index=0u;
    fixture.right_index=1u;
    fixture.left_bias=0u;
    fixture.right_bias=0u;
    fixture.authorize_copy_buffers(t.left.data(),t.right.data(),32u);
    sw::Node node{};
    node.unknown8=0xABC12340u;
    sw::Record r8{},r9{};
    t.fill_separate();
    setup(r8,8u,t,5);
    t.right[2]^=0x50u;

    // Source compares mismatched bytes, but a ZERO correction flag
    // must not write even one byte back into the left buffer.
    fixture.correction_byte=0u;
    const auto left_initial=t.left;
    t.reset();
    if(!dispatch(fixture,1u,node,&r8,1) ||
       t.count!=1u || t.cmp_result[0]!=0u ||
       t.copied[0] || t.left!=left_initial ||
       fixture.correction_events.skipped_correction_disabled!=1u ||
       t.bad)
        return fail(1,"this+0x2C disabled must suppress all native copy");
    // Enabling correction copies EXACTLY the mismatched 5-byte prefix
    // from right EBX into left EDI. Bytes after index4 are untouched.
    fixture.correction_byte=1u;
    t.reset();
    const auto before_copy=left_initial;
    if(!dispatch(fixture,2u,node,&r8,1) ||
       t.count!=1u || t.cmp_result[0]!=0u ||
       !t.copied[0] || t.byte_counts[0]!=5u ||
       fixture.correction_events.correction_copy_paths!=1u ||
       fixture.correction_events.corrected_bytes!=5u ||
       t.bad)
        return fail(2,"case8 mismatched correction must copy exactly N bytes");
    for(unsigned i=0u;i<t.left.size();++i) {
        const auto expected=i<5u?t.right[i]:before_copy[i];
        if(t.left[i]!=expected)
            return fail(3,"source EBX->dest EDI byte-prefix copy direction");
    }

    // Equality returns EAX=1 and takes source JE branch, even
    // when the correction-enable byte remains nonzero.
    t.reset();
    const auto equal_before=t.left;
    const auto skip_before=
        fixture.correction_events.skipped_comparison_equal;
    if(!dispatch(fixture,3u,node,&r8,1) ||
       t.cmp_result[0]!=1u || t.copied[0] ||
       t.left!=equal_before ||
       fixture.correction_events.skipped_comparison_equal!=
           skip_before+1u || t.bad)
        return fail(4,"compare EAX==1 must skip correction branch");
    // Source force-true record byte+0x13 bit0x04 also returns
    // EAX=1; even with different bytes, no correction may happen.
    setup(r8,8u,t,5,0x0400u);
    t.left[0]^=0x22u;
    t.reset();
    const auto force_before=t.left;
    if(!dispatch(fixture,4u,node,&r8,1) ||
       t.cmp_result[0]!=1u || t.copied[0] ||
       t.left!=force_before || t.bad)
        return fail(5,"record force-true gate must suppress source copy");

    // If comparison is disabled at dispatcher this+0x14,
    // the original helper returns zero. This+0x2C correction
    // remains an INDEPENDENT gate: when enabled, it still copies.
    setup(r8,8u,t,5);
    fixture.unknown14=0u;
    t.reset();
    if(!dispatch(fixture,5u,node,&r8,1) ||
       t.cmp_result[0]!=0u || !t.copied[0] ||
       t.left[0]!=t.right[0] || t.bad)
        return fail(6,"disabled compare + enabled correction still copies");
    fixture.unknown14=1u;

    // Case9 uses 4*N bytes from source. For N=3, the source
    // path must copy 12 bytes and leave byte12+ unchanged.
    t.fill_separate();
    setup(r9,9u,t,3);
    t.right[3]^=0x11u;
    t.right[11]^=0x42u;
    t.right[15]^=0x77u;
    const auto before9=t.left;
    t.reset();
    if(!dispatch(fixture,6u,node,&r9,1) ||
       t.cmp_result[0]!=0u || !t.copied[0] ||
       t.byte_counts[0]!=12u || t.bad)
        return fail(7,"case9 must multiply signed length by four");
    for(unsigned i=0u;i<t.left.size();++i) {
        const auto expected=i<12u?t.right[i]:before9[i];
        if(t.left[i]!=expected)
            return fail(8,"case9 native 12-byte prefix copy wrote suffix");
    }

    // One-byte case8 copies only the MOVSB remainder; one-element
    // case9 copies one full MOVSD word. Exact final bytes match.
    t.fill_separate(); t.right[0]^=0x44u; t.right[3]^=0x77u;
    setup(r8,8u,t,1);
    t.reset();
    if(!dispatch(fixture,7u,node,&r8,1) ||
       !t.copied[0] || t.left[0]!=t.right[0] ||
       t.left[3]==t.right[3] || t.bad)
        return fail(9,"case8 remainder-only native MOVSB path");
    t.fill_separate();t.right[3]^=0x44u;
    setup(r9,9u,t,1);
    t.reset();
    if(!dispatch(fixture,8u,node,&r9,1) ||
       !t.copied[0] || t.byte_counts[0]!=4u ||
       t.left[3]!=t.right[3] || t.bad)
        return fail(10,"case9 4*N single-DWORD MOVSD path");

    // Oversized/negative original length is safely refused even
    // when comparison result is zero and correction enabled.
    t.fill_separate();
    setup(r9,9u,t,-1);
    t.reset();
    const auto refusal_before=
        fixture.correction_events.rejected_unowned_or_oversized_copy;
    if(!dispatch(fixture,9u,node,&r9,1) ||
       t.cmp_result[0]!=0u || t.copied[0] ||
       fixture.correction_events.rejected_unowned_or_oversized_copy!=
          refusal_before+1u || t.bad)
        return fail(11,"original unbounded length safely refused");
    // Compare 20 bytes (<=4096) but only 16 of 32 source bytes
    // authorized: source result may be 0, copy must be refused.
    fixture.authorize_copy_buffers(t.left.data(),t.right.data(),16u);
    t.fill_separate();t.right[4]^=0x44u;
    setup(r8,8u,t,20);
    t.reset();
    if(!dispatch(fixture,10u,node,&r8,1) ||
       t.cmp_result[0]!=0u || t.copied[0] ||
       t.left[4]==t.right[4] || t.bad)
        return fail(12,"out-of-grant copy size cannot write host bytes");
    fixture.authorize_copy_buffers(t.left.data(),t.right.data(),32u);

    // Native three-level finalizer RET8 -> dispatcher RET16 ->
    // compare helper RET12, followed by authenticated source
    // correction of both entries. Case8 first copies bytes0..4,
    // case9 second detects mismatch at byte7 and copies 0..11.
    t.fill_separate();
    t.right[2]^=0x21u;
    t.right[7]^=0x41u;
    setup(r8,8u,t,5);
    setup(r9,9u,t,3);
    sw::Node n1{},n2{};
    n1.word0=rawptr(&r8); n1.word4=1u;
    n1.unknown8=0xA1B2C3D4u;n1.next=&n2;
    n2.word0=rawptr(&r9); n2.word4=1u;
    n2.unknown8=0x11223344u;
    final::ListFinalizerFixture finalizer{};
    Nested nested{};
    nested.trace=&t;
    nested.fixture=&fixture;
    nested.first=&n1;
    nested.second=&n2;
    nested.sequence=11u;
    final::FinalizerOps fops{};
    fops.user=&nested;
    fops.apply_entry=&nested_finalizer;
    finalizer.set_ops(fops);
    t.reset();
    std::uintptr_t before=0u,after=0u;
    __asm mov before,esp
    finalizer.Finalize(11u,&n1);
    __asm mov after,esp
    if(before!=after || t.nested_calls!=2u ||
       t.count!=2u || t.case_codes[0]!=8u ||
       t.case_codes[1]!=9u || t.cmp_result[0]!=0u ||
       t.cmp_result[1]!=0u ||
       !t.copied[0] || !t.copied[1] ||
       t.byte_counts[0]!=5u || t.byte_counts[1]!=12u ||
       fixture.cached_node_word!=0u ||
       fixture.current_node!=rawptr(&n2) || t.bad)
        return fail(13,"RET8 -> RET16 -> RET12 nested corrected comparison");
    for(unsigned i=0u;i<t.left.size();++i) {
        if(i<12u && t.left[i]!=t.right[i])
            return fail(14,"nested 12 corrected bytes did not match right");
        if(i>=12u && t.left[i]!=static_cast<std::uint8_t>(
               0x12u+i*3u))
            return fail(15,"nested copy unexpectedly mutated suffix");
    }
    std::puts("PASS Phase1674 original-free MSVC x86: source "
              "PostNetwork case8/case9 inline 41B/43B fixup paths "
              "with native dispatcher this+0x2C correction byte, "
              "compare EAX==1 skip, mismatch right EBX -> left EDI "
              "REP MOVSD/MOVSB byte counts N vs 4*N, force-true "
              "and disabled-compare precedence, N%4 tails and "
              "safe owner memory grants, actual three-level x86 "
              "RET8 linked finalizer/RET16 typed dispatcher/RET12 "
              "gated byte comparison, zero retail methods");
    return 0;
}
