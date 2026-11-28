// Phase1677: original-free native Win32 type8/9 reconciliation pipeline.
// Source-attested type8/9 router call order is compare -> conditional
// inline correction -> specialized diagnostics -> selected preview.
// Native RET12/RET16 generated MSVC methods share ONE synthetic owner.
// Diagnostic models are the previously reviewed Phase217 C++ reference;
// no retail game binary, engine or original x86 method is executed.
#include "native_original_postnetwork_case8_case9_bytecmp.h"
#include "sin_compat/original_client_snapshot_binary_integer.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1677 original-free integrated test requires native MSVC x86
#endif
namespace bc=sin_independent::original_postnetwork_case8_case9_bytecmp;
namespace sw=sin_independent::original_postnetwork_type_switch;
namespace fm=sin_independent::original_postnetwork_scalar_formatters;
namespace bi=sin_compat::original_client_snapshot_binary_integer;
static_assert(sw::kOriginalTypeToCase[8]==8u &&
              sw::kOriginalTypeToCase[9]==9u &&
              bi::kCompareVA==bc::kOriginalMethodVa &&
              bi::kBinaryPreviewVA==fm::kOriginalBinaryVa &&
              offsetof(sw::Fixture,current_record)==0x18u &&
              offsetof(sw::Fixture,correction_byte)==0x2Cu &&
              offsetof(sw::Fixture,diagnostic_record)==0x34u,
              "native phase1677 source type8/9 layout drift");

namespace {
std::uint32_t rawptr(const void* ptr) noexcept {
    return static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(ptr));
}
std::int32_t source_word_count(std::uint16_t w) noexcept {
    return w<0x8000u ? static_cast<std::int32_t>(w) :
                       static_cast<std::int32_t>(w)-0x10000;
}
struct Resolver : bi::ISymbolHost {
    bool found=true;
    unsigned looked_up=0u;
    unsigned base_calls=0u;
    std::int32_t last_value=0;
    std::optional<std::string> find_name(std::int32_t value) override {
        ++looked_up;last_value=value;
        return found?std::optional<std::string>{"resources/example.name"}:
                     std::nullopt;
    }
    std::string q_file_base(const std::string& name,
                            std::size_t capacity) override {
        ++base_calls;
        if(name!="resources/example.name" || capacity!=512u)
            return std::string(512u,'X'); // safe host-only refusal
        return "example";
    }
};
struct FormatEvent {
    std::int32_t value=0;
    fm::FormatKind kind=fm::FormatKind::Short;
    std::array<char,32> label{};
};
struct Trace {
    bc::ComparatorFixture* owner=nullptr;
    std::array<sw::Record,2>* records=nullptr;
    std::array<std::array<std::uint8_t,40>,2> predicted{};
    std::array<std::array<std::uint8_t,40>,2> net{};
    std::array<std::uint32_t,2> status{{99u,99u}};
    std::array<bool,2> corrected{{false,false}};
    std::array<std::uint32_t,2> lengths{{0u,0u}};
    std::array<std::vector<bi::TraceEvent>,2> diagnostics{};
    std::array<FormatEvent,4> previews{};
    Resolver resolver{};
    unsigned seen=0u;
    unsigned count=0u;
    unsigned bad=0u;
    void reset() {
        seen=count=bad=0u;
        status.fill(99u);corrected.fill(false);lengths.fill(0u);
        for(auto& d:diagnostics)d.clear();
        previews.fill(FormatEvent{});
    }
    void initialize_data() noexcept {
        for(unsigned i=0u;i<40u;++i) {
            net[0][i]=static_cast<std::uint8_t>(0x20u+i);
            predicted[0][i]=net[0][i];
            net[1][i]=static_cast<std::uint8_t>(0x40u+i);
            predicted[1][i]=net[1][i];
        }
        net[0][0]=0xFFu;predicted[0][0]=0x80u;
        net[1][11]=0xAAu;predicted[1][11]=0x55u;
    }
};
struct Sink final : bi::ITraceSink {
    Trace& trace;
    unsigned index;
    Sink(Trace& t,unsigned n) : trace(t),index(n) {}
    void on_event(const bi::TraceEvent& event) override {
        trace.diagnostics[index].push_back(event);
    }
};
void on_format(void* user,void* raw_owner,fm::FormatKind kind,
               const char* text,std::int32_t value) noexcept {
    auto* trace=static_cast<Trace*>(user);
    if(!trace || raw_owner!=trace->owner ||
       kind!=fm::FormatKind::Binary || !text ||
       std::strcmp(text,"binary (%i bytes)")!=0 ||
       trace->count>=trace->previews.size()) {
        if(trace)++trace->bad;
        return;
    }
    auto& e=trace->previews[trace->count++];
    e.value=value;e.kind=kind;
    std::strncpy(e.label.data(),text,e.label.size()-1u);
}
void setup(sw::Record& r,std::uint32_t type,std::int16_t count,
           Trace& t,unsigned index,std::uint16_t flags=0u) noexcept {
    r.raw.fill(0u);
    r.write32(0u,type);
    r.write32(0x08u,rawptr(t.predicted[index].data()));
    r.write32(0x0Cu,rawptr(t.net[index].data()));
    r.write16(0x10u,static_cast<std::uint16_t>(count));
    r.write16(0x12u,flags);
    r.write32(sw::kOriginalRecordSequenceOffset,0u);
}
void on_case(void* user,std::uint8_t case_index,std::uint32_t type,
             sw::Record* record,std::uint32_t left,std::uint32_t right,
             std::int16_t signed_flags) noexcept {
    auto* t=static_cast<Trace*>(user);
    if(!t || !t->owner || !t->records || !record ||
       (case_index!=8u && case_index!=9u) || type!=case_index ||
       t->owner->current_record!=rawptr(record) ||
       static_cast<std::uint16_t>(signed_flags)!=record->read16(0x12u)) {
        if(t)++t->bad;
        return;
    }
    unsigned index=2u;
    for(unsigned i=0u;i<2u;++i)
        if(record==&(*t->records)[i])index=i;
    if(index>=2u || left!=rawptr(t->predicted[index].data()) ||
       right!=rawptr(t->net[index].data())) {
        ++t->bad;return;
    }
    ++t->seen;
    const auto signed_n=source_word_count(record->read16(0x10u));
    const auto byte_count=static_cast<std::uint32_t>(
        signed_n*(case_index==9u?4:1));
    auto& owner=*t->owner;
    owner.authorize_current_record(record);
    owner.authorize_copy_buffers(
        t->predicted[index].data(),t->net[index].data(),
        t->predicted[index].size());
    std::uintptr_t before=0u,after=0u;
    __asm mov before,esp
    const auto compared=owner.CompareBytes(
        byte_count,t->predicted[index].data(),t->net[index].data());
    __asm mov after,esp
    if(before!=after)++t->bad;
    t->status[index]=compared;
    t->lengths[index]=byte_count;
    t->corrected[index]=owner.MaybeApplyCase8Case9Correction(
        byte_count,compared,t->predicted[index].data(),
        t->net[index].data());

    bi::OriginalFields fields{};
    fields.enabled_14=owner.unknown14;
    fields.descriptor_flags_13=record->raw[0x13u];
    fields.copy_enabled_2c=owner.correction_byte;
    fields.descriptor_token_18=rawptr(record);
    fields.preview_token_34=owner.diagnostic_record;
    Sink sink(*t,index);
    try {
        if(case_index==9u) {
            bi::binary_diagnostics(fields,compared,byte_count,sink);
        } else {
            // Original integer reporter reads FIRST byte after any
            // correction, even though its comparison spans N bytes.
            const auto predicted=bi::signed_byte(t->predicted[index][0]);
            const auto net=bi::signed_byte(t->net[index][0]);
            bi::integer_diagnostics(fields,compared,predicted,net,
                                    sink,&t->resolver);
        }
    } catch(...) {
        ++t->bad;return;
    }
    // Source uses this+0x34 vs this+0x18 gate, unaffected by
    // this+0x14 comparison and status/diagnostic gate.
    __asm mov before,esp
    owner.FormatBinaryIfSelected(0x11u,byte_count,0x33u,0x44u);
    __asm mov after,esp
    if(before!=after)++t->bad;
}
bool dispatch(bc::ComparatorFixture& owner,std::uint32_t seq,
              sw::Node& node,std::array<sw::Record,2>& records) noexcept {
    std::uintptr_t before=0u,after=0u;
    __asm mov before,esp
    owner.Dispatch(seq,&node,records.data(),2);
    __asm mov after,esp
    return before==after;
}
bool equal_prefix(const Trace& t,unsigned record,unsigned n) noexcept {
    for(unsigned i=0u;i<n;++i)
        if(t.predicted[record][i]!=t.net[record][i])return false;
    return true;
}
int fail(unsigned where,const char* why) {
    std::fprintf(stderr,
       "FAIL Phase1677 native type8/9 integration #%u: %s\n",where,why);
    return static_cast<int>(where);
}
} // namespace

int main() {
    bc::ComparatorFixture owner{};
    Trace trace{};
    std::array<sw::Record,2> records{};
    sw::Node node{};
    node.unknown8=0xABC12340u;
    trace.owner=&owner;trace.records=&records;
    owner.ops.user=&trace;
    owner.ops.on_opaque_case=&on_case;
    owner.format_ops.user=&trace;
    owner.format_ops.on_format=&on_format;
    owner.mode=0u;owner.left_index=0u;owner.right_index=1u;
    owner.left_bias=owner.right_bias=0u;
    owner.unknown14=1u;owner.correction_byte=1u;
    trace.initialize_data();
    setup(records[0],8u,5,trace,0u);
    setup(records[1],9u,3,trace,1u);
    owner.diagnostic_record=rawptr(&records[1]);

    // Mismatches: case8 copies N=5, case9 copies 4*N=12.
    // Diagnostic reports post-copy first integer byte (-1), but
    // preserves mismatch status=0. Only selected case9 previews.
    trace.reset();
    if(!dispatch(owner,1u,node,records) || trace.bad ||
       trace.seen!=2u || trace.status[0]!=0u ||
       trace.status[1]!=0u || !trace.corrected[0] ||
       !trace.corrected[1] || !equal_prefix(trace,0u,5u) ||
       !equal_prefix(trace,1u,12u) ||
       trace.lengths[0]!=5u || trace.lengths[1]!=12u ||
       trace.diagnostics[0].size()!=2u ||
       trace.diagnostics[0][0].kind!=bi::TraceKind::IntegerDifference ||
       trace.diagnostics[0][0].predicted!=-1 ||
       trace.diagnostics[0][0].net!=-1 ||
       trace.diagnostics[0][0].difference!=0 ||
       trace.diagnostics[0][1].kind!=bi::TraceKind::IntegerNumeric ||
       trace.diagnostics[1].size()!=2u ||
       trace.diagnostics[1][0].kind!=bi::TraceKind::BinaryDifference ||
       trace.diagnostics[1][0].byte_count!=12u ||
       trace.diagnostics[1][1].kind!=bi::TraceKind::BinaryStatus ||
       trace.count!=1u || trace.previews[0].value!=12 ||
       owner.cached_node_word!=0u)
        return fail(1,"native compare/correction -> source diagnostics/preview");

    // Correction independently disabled: compare still detects
    // mismatches, and integer difference sees the pre-copy byte.
    trace.initialize_data();trace.reset();
    owner.correction_byte=0u;
    owner.diagnostic_record=rawptr(&records[0]);
    if(!dispatch(owner,2u,node,records) || trace.bad ||
       trace.status[0]!=0u || trace.status[1]!=0u ||
       trace.corrected[0] || trace.corrected[1] ||
       trace.predicted[0][0]!=0x80u ||
       trace.diagnostics[0].size()!=2u ||
       trace.diagnostics[0][0].predicted!=-128 ||
       trace.diagnostics[0][0].net!=-1 ||
       trace.diagnostics[0][0].difference!=-127 ||
       trace.count!=1u || trace.previews[0].value!=5)
        return fail(2,"copy-disabled status and pre-copy diagnostic");

    // Descriptor bit 0x04 forces compare success despite mismatch,
    // suppressing correction while selected preview still runs.
    trace.initialize_data();trace.reset();
    owner.correction_byte=1u;
    setup(records[0],8u,5,trace,0u,0x0400u);
    if(!dispatch(owner,3u,node,records) || trace.bad ||
       trace.status[0]!=1u || trace.corrected[0] ||
       trace.predicted[0][0]!=0x80u ||
       trace.diagnostics[0].size()!=1u ||
       trace.diagnostics[0][0].kind!=bi::TraceKind::IntegerNumeric ||
       trace.count!=1u || trace.previews[0].value!=5)
        return fail(3,"forced true descriptor bypasses correction");

    // Disable comparison/logging but NOT correction or the
    // this+0x34 selected preview; both mismatches must be copied.
    trace.initialize_data();trace.reset();
    setup(records[0],8u,5,trace,0u);
    owner.unknown14=0u;
    owner.diagnostic_record=rawptr(&records[1]);
    if(!dispatch(owner,4u,node,records) || trace.bad ||
       trace.status[0]!=0u || trace.status[1]!=0u ||
       !trace.corrected[0] || !trace.corrected[1] ||
       !equal_prefix(trace,0u,5u) || !equal_prefix(trace,1u,12u) ||
       !trace.diagnostics[0].empty() ||
       !trace.diagnostics[1].empty() ||
       trace.count!=1u || trace.previews[0].value!=12)
        return fail(4,"compare-disable independent correction/preview");

    // Original descriptor high-byte bit0x08 chooses symbol
    // resolution in case8; this diagnostic remains a host model.
    trace.initialize_data();trace.reset();
    owner.unknown14=1u;owner.correction_byte=0u;
    setup(records[0],8u,5,trace,0u,0x0800u);
    owner.diagnostic_record=rawptr(&records[0]);
    trace.resolver.looked_up=trace.resolver.base_calls=0u;
    if(!dispatch(owner,5u,node,records) || trace.bad ||
       trace.diagnostics[0].size()!=2u ||
       trace.diagnostics[0][1].kind!=bi::TraceKind::IntegerSymbol ||
       trace.diagnostics[0][1].symbol!="example" ||
       trace.resolver.looked_up!=1u ||
       trace.resolver.base_calls!=1u ||
       trace.resolver.last_value!=-128 ||
       trace.count!=1u || trace.previews[0].value!=5)
        return fail(5,"case8 source symbolic diagnostics");

    // Dispatcher early skip prevents any case8 side effects
    // including preview, even when it is the selected record.
    trace.initialize_data();trace.reset();
    owner.correction_byte=1u;
    setup(records[0],8u,5,trace,0u,0x0200u);
    if(!dispatch(owner,6u,node,records) || trace.bad ||
       trace.seen!=1u || trace.status[0]!=99u ||
       trace.diagnostics[0].size()!=0u ||
       trace.count!=0u || owner.events.flag_mode_skips==0u)
        return fail(6,"dispatcher early flag filter prevents preview");

    // Native zero-length REPZ CMPSB returns equality without
    // dereferencing buffers; binary preview may forward count0.
    trace.initialize_data();trace.reset();
    setup(records[0],8u,5,trace,0u);
    setup(records[1],9u,0,trace,1u);
    owner.diagnostic_record=rawptr(&records[1]);
    if(!dispatch(owner,7u,node,records) || trace.bad ||
       trace.status[1]!=1u || trace.corrected[1] ||
       trace.diagnostics[1].size()!=1u ||
       trace.diagnostics[1][0].kind!=bi::TraceKind::BinaryStatus ||
       trace.count!=1u || trace.previews[0].value!=0)
        return fail(7,"zero-count native compare and preview");

    // Host-only invalid signed negative length: original x86 can
    // access unsafe ranges. This fixture refuses the oversized
    // native compare and copy without any unowned host writes;
    // raw signed byte count is forwarded to selected diagnostics.
    trace.initialize_data();trace.reset();
    setup(records[1],9u,-1,trace,1u);
    const auto refused_cmp=owner.compare_events.refused_unsafe_count_or_buffers;
    const auto refused_copy=owner.correction_events.rejected_unowned_or_oversized_copy;
    if(!dispatch(owner,8u,node,records) || trace.bad ||
       trace.status[1]!=0u || trace.corrected[1] ||
       trace.lengths[1]!=0xFFFFFFFCu ||
       owner.compare_events.refused_unsafe_count_or_buffers!=refused_cmp+1u ||
       owner.correction_events.rejected_unowned_or_oversized_copy!=refused_copy+1u ||
       trace.count!=1u || trace.previews[0].value!=-4)
        return fail(8,"negative count guarded host memory, preserves signed preview");

    std::puts("PASS Phase1677 original-free native MSVC x86: original "
              "type8/9 shared RET12 compare, inline conditional "
              "correction N vs 4*N, Phase217 integer/binary "
              "diagnostics including optional symbol, and native "
              "Phase1675 RET16 selected preview on SAME owner; "
              "flag/selection gates and ESP stable; no retail game");
    return 0;
}
