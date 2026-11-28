// Phase1673: actual MSVC Win32 x86 gated original PostNetwork
// 56-byte REPZ CMPSB contract, one ECX=this and 3 DWORD RET12.
// Also composes typed dispatcher case8/9 RET16 and Phase1671
// linked-list finalizer RET8 through actual native C++ member calls.
//
// All byte buffers, records, callbacks and nodes are host-owned.
// No original commercial DLL code or game engine is executed.
#include "native_original_postnetwork_case8_case9_bytecmp.h"
#include "native_original_postnetwork_diagnostic_core_finalizer.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1673 PostNetwork byte-compare gate needs native MSVC x86
#endif
namespace bc=sin_independent::original_postnetwork_case8_case9_bytecmp;
namespace sw=sin_independent::original_postnetwork_type_switch;
namespace final=sin_independent::original_postnetwork_diagnostic_core_finalizer;
namespace {
int fail(int id,const char* why) {
    std::fprintf(stderr,
      "FAIL Phase1673 original-free PostNetwork native x86 byte gate "
      "#%d: %s\n",id,why);
    return id;
}
struct Trace {
    bc::ComparatorFixture* dispatcher=nullptr;
    std::array<std::uint8_t,32> left{};
    std::array<std::uint8_t,32> right{};
    std::array<std::uint32_t,32> results{};
    std::array<std::uint32_t,32> lengths{};
    std::array<std::uint8_t,32> cases{};
    unsigned calls=0u;
    unsigned warnings=0u;
    unsigned bad=0u;
    unsigned nested_nodes=0u;
    void reset() noexcept {
        results.fill(0u);lengths.fill(0u);cases.fill(0u);
        calls=warnings=bad=nested_nodes=0u;
    }
};
std::uint32_t rawptr(const void* ptr) noexcept {
    return static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(ptr));
}
void prepare(sw::Record& r,std::uint32_t type,
             const Trace& t,std::int16_t signed_len,
             std::uint16_t flags=0u) noexcept {
    r.raw.fill(0u);
    r.write32(0u,type);
    r.write32(0x08u,rawptr(t.left.data()));
    r.write32(0x0Cu,rawptr(t.right.data()));
    r.write16(0x10u,static_cast<std::uint16_t>(signed_len));
    r.write16(0x12u,flags);
    r.write32(sw::kOriginalRecordSequenceOffset,0u);
}
void on_opaque_case(void* user,std::uint8_t case_index,
                    std::uint32_t type,sw::Record* record,
                    std::uint32_t left,std::uint32_t right,
                    std::int16_t flags) noexcept {
    auto* t=static_cast<Trace*>(user);
    if(!t->dispatcher || !record ||
       (case_index!=8u && case_index!=9u) ||
       case_index!=type || left!=rawptr(t->left.data()) ||
       right!=rawptr(t->right.data()) ||
       static_cast<std::uint16_t>(flags)!=record->read16(0x12u)) {
        ++t->bad;
        return;
    }
    const auto signed_elements=
        static_cast<std::int16_t>(record->read16(0x10u));
    const auto signed_bytes=static_cast<std::int32_t>(
        signed_elements)*(case_index==9u?4:1);
    const auto native_count=static_cast<std::uint32_t>(signed_bytes);
    t->dispatcher->authorize_current_record(record);
    std::uintptr_t before=0u,after=0u;
    __asm mov before,esp
    const auto result=t->dispatcher->CompareBytes(
        native_count,t->left.data(),t->right.data());
    __asm mov after,esp
    if(before!=after)++t->bad;
    if(t->calls<t->results.size()) {
        t->results[t->calls]=result;
        t->lengths[t->calls]=native_count;
        t->cases[t->calls]=case_index;
    } else ++t->bad;
    ++t->calls;
}
void on_warning(void* user,std::uint32_t,sw::Record*) noexcept {
    ++static_cast<Trace*>(user)->warnings;
}
sw::DispatchOps ops_for(Trace& t) noexcept {
    sw::DispatchOps ops{};
    ops.user=&t;
    ops.on_opaque_case=&on_opaque_case;
    ops.on_opaque_warning=&on_warning;
    return ops;
}
bool call4(bc::ComparatorFixture& fixture,std::uint32_t seq,
           sw::Node& node,sw::Record* records,
           std::int32_t count) noexcept {
    std::uintptr_t before=0u,after=0u;
    __asm mov before,esp
    fixture.Dispatch(seq,&node,records,count);
    __asm mov after,esp
    return before==after;
}
struct Nested {
    Trace* trace=nullptr;
    bc::ComparatorFixture* owner=nullptr;
    sw::Node* expected1=nullptr;
    sw::Node* expected2=nullptr;
    std::uint32_t seq=0u;
};
void nested_finalizer(void* user,std::uint32_t sequence,
                      sw::Node* node,std::uint32_t raw_records,
                      std::uint32_t count) noexcept {
    auto* n=static_cast<Nested*>(user);
    ++n->trace->nested_nodes;
    if(sequence!=n->seq ||
       (node!=n->expected1 && node!=n->expected2) ||
       raw_records!=node->word0 || count!=node->word4 ||
       !call4(*n->owner,sequence,*node,
         reinterpret_cast<sw::Record*>(
             static_cast<std::uintptr_t>(raw_records)),
         static_cast<std::int32_t>(count)))
        ++n->trace->bad;
}
} // namespace
int main() {
    Trace t{};
    bc::ComparatorFixture fixture{};
    fixture.ops=ops_for(t);
    t.dispatcher=&fixture;
    fixture.unknown14=1u;
    fixture.left_index=0u;
    fixture.right_index=1u;
    fixture.left_bias=0u;
    fixture.right_bias=0u;
    for(unsigned i=0u;i<t.left.size();++i) {
        t.left[i]=static_cast<std::uint8_t>(i*7u+0x19u);
        t.right[i]=t.left[i];
    }
    t.right[7]^=0x5Au; // outside 5 bytes, inside 12 bytes
    sw::Record r8{},r9{};
    prepare(r8,8u,t,5);
    prepare(r9,9u,t,3);
    sw::Node node{};
    node.unknown8=0xA1B2C3D4u;
    if(!call4(fixture,1u,node,&r8,1) ||
       t.calls!=1u || t.cases[0]!=8u ||
       t.lengths[0]!=5u || t.results[0]!=1u ||
       fixture.compare_events.compared!=1u ||
       fixture.compare_events.equal!=1u || t.bad)
        return fail(1,"native case8 raw byte count5 should compare equal");
    t.reset();
    if(!call4(fixture,2u,node,&r9,1) ||
       t.calls!=1u || t.cases[0]!=9u ||
       t.lengths[0]!=12u || t.results[0]!=0u ||
       fixture.compare_events.unequal!=1u || t.bad)
        return fail(2,"case9 four-times signed record count must find mismatch");
    t.right[7]=t.left[7];
    t.reset();
    if(!call4(fixture,3u,node,&r9,1) ||
       t.results[0]!=1u || t.lengths[0]!=12u ||
       fixture.compare_events.equal!=2u || t.bad)
        return fail(3,"native case9 equality after replacing changed byte");
    // Difference after the compared prefix must not matter.
    t.right[18]^=0xFFu;
    t.reset();
    if(!call4(fixture,4u,node,&r9,1) ||
       t.results[0]!=1u || t.bad)
        return fail(4,"REPZ CMPSB byte count must not examine suffix");

    // Source record BYTE +0x13 bit0x04 overrides byte comparison
    // and returns AL1 without reading either input array.
    prepare(r9,9u,t,3,0x0400u);
    t.right[0]^=0x55u;
    t.reset();
    const auto before_forced=fixture.compare_events.forced_true_from_record_flag;
    if(!call4(fixture,5u,node,&r9,1) ||
       t.results[0]!=1u || t.bad ||
       fixture.compare_events.forced_true_from_record_flag!=
         before_forced+1u)
        return fail(5,"native record+0x13 bit2 force-true return");

    // Source this+0x14 flag is checked BEFORE record flag: disabled
    // must return zero, even when force-true bit is set.
    fixture.unknown14=0u;
    t.reset();
    if(!call4(fixture,6u,node,&r9,1) ||
       t.results[0]!=0u ||
       fixture.compare_events.returned_disabled_zero!=1u || t.bad)
        return fail(6,"disabled native this+0x14 must override force-true flag");
    fixture.unknown14=1u;

    // With enabled and zero byteCount, x86 XOR EDX,EDX sets ZF=1,
    // REPZ CMPSB does nothing, and SETE returns 1.
    prepare(r8,8u,t,0);
    t.reset();
    if(!call4(fixture,7u,node,&r8,1) ||
       t.calls!=1u || t.lengths[0]!=0u ||
       t.results[0]!=1u || t.bad)
        return fail(7,"zero-length native byte comparison should equal");
    std::uintptr_t before=0u,after=0u;
    fixture.authorize_current_record(&r8);
    fixture.current_record=rawptr(&r8);
    __asm mov before,esp
    const auto null_zero=fixture.CompareBytes(0u,nullptr,nullptr);
    __asm mov after,esp
    if(before!=after || null_zero!=1u)
        return fail(8,"direct native three-stack-arg RET12/zero-length null");

    // Signed record length -1 maps to UINT32_MAX (case8) or
    // UINT32_MAX-3 (case9), both safely refused by public fixture.
    prepare(r9,9u,t,-1);
    t.reset();
    const auto before_refuse=
        fixture.compare_events.refused_unsafe_count_or_buffers;
    if(!call4(fixture,8u,node,&r9,1) ||
       t.calls!=1u || t.results[0]!=0u ||
       t.lengths[0]!=0xFFFFFFFCu ||
       fixture.compare_events.refused_unsafe_count_or_buffers!=
           before_refuse+1u || t.bad)
        return fail(9,"negative signed case9 record length not safely refused");
    // Directly reject missing trusted record and null positive buffer.
    fixture.authorize_current_record(nullptr);
    fixture.current_record=rawptr(&r9);
    const auto bad_record=fixture.compare_events.refused_invalid_record;
    if(fixture.CompareBytes(1u,t.left.data(),t.right.data())!=0u ||
       fixture.compare_events.refused_invalid_record!=bad_record+1u)
        return fail(10,"raw original current-record pointer not safely owned");
    fixture.authorize_current_record(&r9);
    const auto bad_buffer=
        fixture.compare_events.refused_unsafe_count_or_buffers;
    if(fixture.CompareBytes(1u,nullptr,t.right.data())!=0u ||
       fixture.compare_events.refused_unsafe_count_or_buffers!=
           bad_buffer+1u)
        return fail(11,"native positive-length null byte-buffer not refused");

    // Source dispatcher flag gating happens BEFORE it calls case8/9.
    // 0x0200 flag skips ordinary type8 entirely, no CMPSB call.
    prepare(r8,8u,t,3,0x0200u);
    t.reset();
    const auto compared_before=fixture.compare_events.compared;
    if(!call4(fixture,9u,node,&r8,1) || t.calls!=0u ||
       fixture.compare_events.compared!=compared_before ||
       t.bad)
        return fail(12,"outer original record bit0x0200 must suppress compare");

    // A real native Phase1671 linked finalizer RET8 dispatches
    // two entries to the native typed dispatcher RET16, which
    // invokes this native case8/9 byte helper RET12. Nothing
    // original is executed; only owned fixture objects.
    prepare(r8,8u,t,5);
    prepare(r9,9u,t,3);
    t.right=t.left;
    t.right[7]^=0x01u;
    sw::Node n1{},n2{};
    n1.word0=rawptr(&r8);
    n1.word4=1u;
    n1.unknown8=0xAA11BB22u;
    n1.next=&n2;
    n2.word0=rawptr(&r9);
    n2.word4=1u;
    n2.unknown8=0xCC33DD44u;
    final::ListFinalizerFixture finalizer{};
    Nested nested{};
    nested.trace=&t;
    nested.owner=&fixture;
    nested.expected1=&n1;
    nested.expected2=&n2;
    nested.seq=10u;
    final::FinalizerOps fops{};
    fops.user=&nested;
    fops.apply_entry=&nested_finalizer;
    finalizer.set_ops(fops);
    t.reset();
    __asm mov before,esp
    finalizer.Finalize(10u,&n1);
    __asm mov after,esp
    if(before!=after || t.nested_nodes!=2u ||
       t.calls!=2u || t.cases[0]!=8u || t.cases[1]!=9u ||
       t.lengths[0]!=5u || t.lengths[1]!=12u ||
       t.results[0]!=1u || t.results[1]!=0u ||
       fixture.cached_node_word!=0u ||
       fixture.current_node!=rawptr(&n2) || t.bad)
        return fail(13,"actual three-level x86 RET8 -> RET16 -> RET12 chain");

    std::puts("PASS Phase1673 original-free MSVC x86: original 56B "
              "PostNetwork REPZ CMPSB native three-arg RET12 gate, "
              "this+0x14 enabled, this+0x18 record byte+0x13 bit4 "
              "forced AL1, signed case8 length vs case9 four-times "
              "length, equality/mismatch/suffix/zero-length ZF, "
              "negative length and owned-pointer refusal, "
              "nested real Win32 RET8 finalizer -> RET16 typed "
              "dispatcher -> RET12 byte helper, no original code");
    return 0;
}
