// Phase1672: original-free native Win32 x86 tests for the original
// 1036-byte PostNetwork per-node dispatch entry at 0x241996F0.
//
// This executes a REAL compiler-generated NONVIRTUAL MSVC __thiscall
// four-DWORD member call (native RET 0x10), and the previous 2-DWORD
// Phase1671 finalizer (RET 0x08). Original comparison case bodies
// remain injected opaque callbacks, never emulated as equality.
//
// No original retail DLL, Source game object or executable original
// machine code is executed in this native diagnostic regression.
#include "native_original_postnetwork_type_switch.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1672 original typed PostNetwork dispatch requires MSVC x86
#endif
namespace sw=sin_independent::original_postnetwork_type_switch;
namespace final=sin_independent::original_postnetwork_diagnostic_core_finalizer;
namespace {
int fail(int n,const char* why) {
    std::fprintf(stderr,
      "FAIL Phase1672 original-free typed PostNetwork x86 #%d: %s\n",
      n,why);
    return n;
}
struct CaseEvent {
    std::uint8_t case_index=0u;
    std::uint32_t type=0u;
    sw::Record* record=nullptr;
    std::uint32_t left=0u;
    std::uint32_t right=0u;
    std::int16_t flags=0;
};
struct Trace {
    std::array<CaseEvent,64> seen{};
    std::array<std::uint32_t,32> warned_types{};
    unsigned seen_count=0u;
    unsigned warn_count=0u;
    unsigned alias_count=0u;
    unsigned unexpected=0u;
    unsigned nested_calls=0u;
    unsigned nested_records=0u;
    std::uint32_t nested_sequence=0u;
    std::uint32_t expected_cache_during_callback=0u;
    bool assert_cache=false;
    sw::Fixture* current=nullptr;
    sw::Record* alias_other=nullptr;
    void reset() noexcept {
        seen_count=warn_count=alias_count=unexpected=0u;
        nested_calls=0u;nested_records=0u;
        seen.fill(CaseEvent{});
        warned_types.fill(0u);
        assert_cache=false;
    }
};
void on_case(void* user,std::uint8_t case_id,
             std::uint32_t type,sw::Record* record,
             std::uint32_t left,std::uint32_t right,
             std::int16_t flags) noexcept {
    auto* t=static_cast<Trace*>(user);
    if(t->seen_count<t->seen.size())
        t->seen[t->seen_count]=CaseEvent{
            case_id,type,record,left,right,flags};
    else ++t->unexpected;
    ++t->seen_count;
    if(t->assert_cache && t->current &&
       t->current->cached_node_word!=t->expected_cache_during_callback)
        ++t->unexpected;
}
void on_warning(void* user,std::uint32_t type,
                sw::Record* record) noexcept {
    auto* t=static_cast<Trace*>(user);
    if(t->warn_count<t->warned_types.size())
        t->warned_types[t->warn_count]=type;
    else ++t->unexpected;
    ++t->warn_count;
    if(!record)++t->unexpected;
}
void on_alias(void* user,sw::Record* source,
              std::uint32_t raw_alias,
              std::uint32_t sequence) noexcept {
    auto* t=static_cast<Trace*>(user);
    ++t->alias_count;
    sw::Record* destination=nullptr;
    if(raw_alias==static_cast<std::uint32_t>(
           reinterpret_cast<std::uintptr_t>(source)))
        destination=source;
    else if(t->alias_other &&
        raw_alias==static_cast<std::uint32_t>(
           reinterpret_cast<std::uintptr_t>(t->alias_other)))
        destination=t->alias_other;
    if(!destination) {
        ++t->unexpected;
        return;
    }
    destination->write32(sw::kOriginalRecordSequenceOffset,sequence);
}
sw::DispatchOps ops_for(Trace& t) noexcept {
    sw::DispatchOps ops{};
    ops.user=&t;
    ops.update_alias_sequence=&on_alias;
    ops.on_opaque_case=&on_case;
    ops.on_opaque_warning=&on_warning;
    return ops;
}
void prepare(sw::Record& r,std::uint32_t type,
             std::uint16_t flags=0u) noexcept {
    r.raw.fill(0u);
    r.write32(0u,type);
    r.write32(0x08u,0x10000u+type);
    r.write32(0x0Cu,0x20000u+type);
    r.write16(sw::kOriginalRecordFlagsOffset,flags);
    r.write32(sw::kOriginalRecordSequenceOffset,0u);
}
bool call4(sw::Fixture& owner,std::uint32_t sequence,
           sw::Node& node,sw::Record* records,
           std::int32_t count) noexcept {
    std::uintptr_t before=0u,after=0u;
    __asm mov before,esp
    owner.Dispatch(sequence,&node,records,count);
    __asm mov after,esp
    return before==after;
}
struct Nested {
    Trace* trace=nullptr;
    sw::Fixture* dispatcher=nullptr;
    sw::Node* expected_first=nullptr;
    sw::Node* expected_second=nullptr;
};
void invoke_from_finalizer(void* user,std::uint32_t sequence,
                           sw::Node* node,
                           std::uint32_t record_ptr,
                           std::uint32_t raw_count) noexcept {
    auto* n=static_cast<Nested*>(user);
    ++n->trace->nested_calls;
    if(sequence!=n->trace->nested_sequence ||
       (node!=n->expected_first && node!=n->expected_second) ||
       node->word0!=record_ptr || node->word4!=raw_count)
        ++n->trace->unexpected;
    const auto* const records=reinterpret_cast<sw::Record*>(
        static_cast<std::uintptr_t>(record_ptr));
    if(!call4(*n->dispatcher,sequence,*node,
              const_cast<sw::Record*>(records),
              static_cast<std::int32_t>(raw_count)))
        ++n->trace->unexpected;
    n->trace->nested_records+=raw_count;
}
} // namespace
int main() {
    // All 27 ORIGINAL type codes plus type27 out of table range.
    Trace t{};
    for(std::uint32_t type=0u;type<=27u;++type) {
        sw::Fixture owner{};
        t.reset();
        t.current=&owner;
        owner.ops=ops_for(t);
        owner.left_index=0u;
        owner.right_index=1u;
        owner.left_bias=0x11u;
        owner.right_bias=0x22u;
        owner.option=0xABu;
        sw::Record r{};
        prepare(r,type);
        sw::Node node{};
        node.unknown8=0x00F00D44u;
        const auto seq=type+1u;
        if(!call4(owner,seq,node,&r,1))
            return fail(1,"native nonvirtual __thiscall four-arg RET16 ESP");
        if(owner.cached_node_word!=0u ||
           owner.current_node!=static_cast<std::uint32_t>(
               reinterpret_cast<std::uintptr_t>(&node)) ||
           owner.current_record!=static_cast<std::uint32_t>(
               reinterpret_cast<std::uintptr_t>(&r)) ||
           owner.copied_option!=0xABu || owner.active!=1u ||
           owner.events.visited!=1u || t.unexpected)
            return fail(2,"retail per-node original +0x18/+0x1C/+0x20/+0x24/25 fields");
        const auto expected_case=type<27u?
            sw::kOriginalTypeToCase[type]:12u;
        if(expected_case==0u && type<=26u) {
            if(t.seen_count!=0u || t.warn_count!=0u ||
               owner.events.no_op_case0!=1u)
                return fail(3,"source type-code case0 must be silent no-op");
        } else if(expected_case==12u) {
            if(t.seen_count!=0u || t.warn_count!=1u ||
               t.warned_types[0]!=type ||
               owner.events.opaque_warnings!=1u)
                return fail(4,"case12/type>26 original error handoff");
        } else {
            if(t.seen_count!=1u || t.warn_count!=0u ||
               t.seen[0].case_index!=expected_case ||
               t.seen[0].type!=type ||
               t.seen[0].record!=&r ||
               t.seen[0].left!=0x10000u+type+0x11u ||
               t.seen[0].right!=0x20000u+type+0x22u)
                return fail(5,"27->13 mapping or record pointers/biases");
            if(type==10u &&
               owner.events.recursive_case10_not_implemented!=1u)
                return fail(6,"retail recursive case10 not explicitly opaque");
        }
    }

    // Negative and zero signed record count must skip all records
    // but still save node pointer then reset original this+0x1C.
    sw::Fixture owner{};
    owner.ops=ops_for(t);
    t.current=&owner;
    sw::Record r{};
    prepare(r,1u);
    sw::Node node{};
    node.unknown8=0xAA11BB22u;
    owner.cached_node_word=0x77u;
    t.reset();t.current=&owner;
    if(!call4(owner,1u,node,nullptr,-3) ||
       owner.cached_node_word!=0u ||
       owner.events.visited!=0u || t.seen_count!=0u)
        return fail(7,"signed negative count/entry cache cleanup");
    if(!call4(owner,1u,node,nullptr,0) ||
       owner.events.visited!=0u ||
       owner.current_node!=static_cast<std::uint32_t>(
           reinterpret_cast<std::uintptr_t>(&node)))
        return fail(8,"zero count native entry/RET16 semantics");

    // Common bit 0x0200 skips all ordinary types. Type10 bypasses
    // this common gate AND both mode-dependent 0x0100 gates.
    t.reset();
    prepare(r,1u,0x0200u);
    if(!call4(owner,1u,node,&r,1) ||
       owner.events.flag_mode_skips!=1u || t.seen_count!=0u)
        return fail(9,"original common flags high byte bit1 skip");
    t.reset();
    prepare(r,10u,0x0300u);
    if(!call4(owner,2u,node,&r,1) ||
       t.seen_count!=1u || t.seen[0].case_index!=10u ||
       owner.events.recursive_case10_not_implemented!=1u)
        return fail(10,"type10 bypass flag gate and opaque recursive case");
    owner.mode=1u;
    t.reset();
    prepare(r,1u,0x0100u);
    if(!call4(owner,3u,node,&r,1) ||
       t.seen_count!=0u || owner.events.flag_mode_skips!=2u)
        return fail(11,"original mode1 rejects bit0 in flags high byte");
    owner.mode=2u;
    t.reset();
    prepare(r,1u,0x0000u);
    if(!call4(owner,4u,node,&r,1) ||
       t.seen_count!=0u || owner.events.flag_mode_skips!=3u)
        return fail(12,"original mode2 requires bit0 of flags high byte");
    t.reset();
    prepare(r,1u,0x0100u);
    if(!call4(owner,5u,node,&r,1) ||
       t.seen_count!=1u || t.seen[0].flags!=0x0100)
        return fail(13,"mode2 accepts high-byte bit0 when set");
    owner.mode=0u;

    // Original compares record+0x2C to sequence BEFORE type
    // case dispatch. It does not interpret it as a debug count.
    t.reset();
    prepare(r,1u);
    r.write32(sw::kOriginalRecordSequenceOffset,6u);
    if(!call4(owner,6u,node,&r,1) ||
       t.seen_count!=0u ||
       owner.events.repeated_sequence_skips!=1u)
        return fail(14,"source record+0x2C same-sequence suppression");

    // Original if record+0x28 !=null writes through the pointer
    // to [alias+0x2C] and THEN checks [record+0x2C].
    // Self-alias makes it skip; other-alias does not.
    t.reset();
    prepare(r,1u);
    r.write32(sw::kOriginalRecordAliasOffset,
        static_cast<std::uint32_t>(
            reinterpret_cast<std::uintptr_t>(&r)));
    if(!call4(owner,7u,node,&r,1) ||
       r.read32(sw::kOriginalRecordSequenceOffset)!=7u ||
       t.alias_count!=1u || t.seen_count!=0u ||
       owner.events.repeated_sequence_skips!=2u)
        return fail(15,"record+0x28 self alias update must precede skip");
    t.reset();
    sw::Record other{};
    prepare(other,0u);
    t.alias_other=&other;
    prepare(r,2u);
    r.write32(sw::kOriginalRecordAliasOffset,
        static_cast<std::uint32_t>(
            reinterpret_cast<std::uintptr_t>(&other)));
    if(!call4(owner,8u,node,&r,1) ||
       other.read32(sw::kOriginalRecordSequenceOffset)!=8u ||
       r.read32(sw::kOriginalRecordSequenceOffset)!=0u ||
       t.seen_count!=1u || t.seen[0].case_index!=2u)
        return fail(16,"alias-other update should not skip current record");
    t.alias_other=nullptr;

    // Source this+0x1C is NOT replaced from node+8 when already
    // nonzero, but always clears at final RET16. Observe mid-call.
    t.reset();
    prepare(r,3u);
    owner.cached_node_word=0x13572468u;
    t.assert_cache=true;
    t.expected_cache_during_callback=0x13572468u;
    if(!call4(owner,9u,node,&r,1) ||
       t.unexpected!=0u || t.seen_count!=1u ||
       owner.cached_node_word!=0u)
        return fail(17,"nonzero original this+0x1C cache retained until epilogue");

    // Native source record array increments by 0x34 even on a
    // skipped type. There is NO array of pointers between records.
    t.reset();t.current=&owner;
    std::array<sw::Record,3> records{};
    prepare(records[0],1u);
    prepare(records[1],0u);
    prepare(records[2],13u);
    if(reinterpret_cast<std::uintptr_t>(&records[1])-
          reinterpret_cast<std::uintptr_t>(&records[0])!=
              sw::kOriginalRecordStride ||
       !call4(owner,10u,node,records.data(),3) ||
       t.seen_count!=2u ||
       t.seen[0].record!=&records[0] ||
       t.seen[0].case_index!=1u ||
       t.seen[1].record!=&records[2] ||
       t.seen[1].case_index!=11u ||
       owner.events.visited<3u)
        return fail(18,"0x34-byte record walk or type13 mapped case11");

    // Original record can index more than two DWORD source bases.
    // Untrusted original indices/counts are safely bounded in CI.
    t.reset(); owner.left_index=1024u;
    const auto refused_before=owner.events.refused_indices;
    if(!call4(owner,11u,node,records.data(),1) ||
       owner.events.refused_indices!=refused_before+1u ||
       t.seen_count!=0u)
        return fail(19,"malformed base index must be safely refused");
    owner.left_index=0u;
    t.reset();
    const auto null_before=owner.events.missing_inputs;
    if(!call4(owner,12u,node,nullptr,1) ||
       owner.events.missing_inputs!=null_before+1u)
        return fail(20,"null record array must safely refuse original deref");
    const auto max_before=owner.events.refused_count;
    if(!call4(owner,13u,node,records.data(),4097) ||
       owner.events.refused_count!=max_before+1u)
        return fail(21,"oversized synthetic record count must be bounded");
    // Compose with the Phase1671 native finalizer. This calls the
    // exact four-argument synthetic source dispatcher using original
    // finalizer sequence,node,node.word0,node.word4 argument order.
    t.reset();
    sw::Fixture nested_owner{};
    nested_owner.ops=ops_for(t);
    t.current=&nested_owner;
    std::array<sw::Record,2> group_a{};
    std::array<sw::Record,1> group_b{};
    prepare(group_a[0],1u);
    prepare(group_a[1],2u);
    prepare(group_b[0],3u);
    sw::Node n1{},n2{};
    n1.word0=static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(group_a.data()));
    n1.word4=2u;
    n1.unknown8=0xAAAABBBBu;
    n1.next=&n2;
    n2.word0=static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(group_b.data()));
    n2.word4=1u;
    n2.unknown8=0xCCCCDDDDu;
    final::ListFinalizerFixture finalizer{};
    Nested nested{};
    nested.trace=&t;
    nested.dispatcher=&nested_owner;
    nested.expected_first=&n1;
    nested.expected_second=&n2;
    t.nested_sequence=21u;
    final::FinalizerOps fops{};
    fops.user=&nested;
    fops.apply_entry=&invoke_from_finalizer;
    finalizer.set_ops(fops);
    std::uintptr_t before=0u,after=0u;
    __asm mov before,esp
    finalizer.Finalize(21u,&n1);
    __asm mov after,esp
    if(before!=after || t.unexpected!=0u ||
       t.nested_calls!=2u || t.nested_records!=3u ||
       t.seen_count!=3u ||
       t.seen[0].case_index!=1u ||
       t.seen[1].case_index!=2u ||
       t.seen[2].case_index!=3u ||
       nested_owner.cached_node_word!=0u ||
       nested_owner.current_node!=static_cast<std::uint32_t>(
           reinterpret_cast<std::uintptr_t>(&n2)))
        return fail(23,"actual native finalizer RET8 -> dispatch RET16 chain");

    std::puts("PASS Phase1672 original-free MSVC x86: 1036-byte "
              "PostNetwork typed per-node dispatch entry, authentic "
              "four-arg __thiscall RET16/0x34 stride, 27 type aliases "
              "and 13 case targets, case0 no-op/case12 warning/"
              "type10 recursive-opaque, flags and mode gates, "
              "record+0x28 alias-before-+0x2C sequence skip, "
              "this+0x18/+0x1C/+0x20/+0x24/+0x25 layout, "
              "safe count/indices, and nested native finalizer RET8 "
              "forwarding 4 native DWORDs, no retail code");
    return 0;
}
