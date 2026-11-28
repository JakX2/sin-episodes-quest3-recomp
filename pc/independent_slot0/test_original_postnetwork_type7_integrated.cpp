// Phase1676: original-free native Win32 composition of retail type-7
// short-record CALL order with the Phase1672 dispatcher and Phase1675
// selected-record signed-short preview. Original Phase218 independently
// authenticated compare/copy/diagnostics; these are C++ host models,
// NOT an original game module, nor original specialist machine code.
#include "native_original_postnetwork_scalar_formatters.h"
#include "sin_compat/original_client_snapshot_short.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1676 native composition requires MSVC x86
#endif

namespace sw=sin_independent::original_postnetwork_type_switch;
namespace fmt=sin_independent::original_postnetwork_scalar_formatters;
namespace short7=sin_compat::original_client_snapshot_short;

static_assert(sw::kOriginalTypeToCase[7]==7u &&
              fmt::kOriginalShortVa==short7::kPreviewVA &&
              offsetof(sw::Fixture,current_record)==0x18u &&
              offsetof(sw::Fixture,correction_byte)==0x2Cu &&
              offsetof(sw::Fixture,diagnostic_record)==0x34u,
              "original type7 dispatch and diagnostic owner layout drift");

namespace {
std::uint32_t rawptr(const void* ptr) noexcept {
    return static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(ptr));
}
struct Context {
    fmt::Fixture* owner=nullptr;
    std::array<sw::Record,2>* records=nullptr;
    std::array<short7::Bytes,2> net{};
    std::array<short7::Bytes,2> predicted{};
    std::array<std::uint32_t,2> statuses{{99u,99u}};
    std::array<std::array<short7::TraceEvent,4>,2> events{};
    std::array<unsigned,2> event_counts{};
    std::int16_t authorized_preview=0;
    unsigned seen=0u;
    unsigned bad=0u;
    unsigned previews=0u;
    std::int32_t preview_value=0;
    void reset() noexcept {
        statuses.fill(99u);
        event_counts.fill(0u);
        seen=bad=previews=0u;
        preview_value=0;
    }
};
struct Sink final : short7::ITraceSink {
    Context& ctx;
    unsigned index;
    Sink(Context& c,unsigned i) : ctx(c), index(i) {}
    void on_event(const short7::TraceEvent& event) override {
        const auto n=ctx.event_counts[index]++;
        if(n<ctx.events[index].size()) ctx.events[index][n]=event;
        else ++ctx.bad;
    }
};
void on_format(void* user,void* raw_owner,fmt::FormatKind kind,
               const char* text,std::int32_t value) noexcept {
    auto* ctx=static_cast<Context*>(user);
    if(raw_owner!=ctx->owner || kind!=fmt::FormatKind::Short ||
       !text || std::strcmp(text,"short (%i)")!=0) {
        ++ctx->bad;
        return;
    }
    ++ctx->previews;
    ctx->preview_value=value;
}
std::int32_t signed_count(std::uint16_t w) noexcept {
    return w<0x8000u ? static_cast<std::int32_t>(w) :
                       static_cast<std::int32_t>(w)-0x10000;
}
void on_type7(void* user,std::uint8_t case_index,std::uint32_t type,
              sw::Record* record,std::uint32_t,std::uint32_t,
              std::int16_t) noexcept {
    auto* ctx=static_cast<Context*>(user);
    if(!ctx->owner || !ctx->records || !record ||
       case_index!=7u || type!=7u ||
       ctx->owner->current_record!=rawptr(record)) {
        ++ctx->bad;
        return;
    }
    unsigned index=2u;
    for(unsigned i=0u;i<ctx->records->size();++i)
        if(record==&(*ctx->records)[i]) index=i;
    if(index>=ctx->records->size()) {
        ++ctx->bad;
        return;
    }
    ++ctx->seen;
    short7::OriginalFields fields{};
    fields.enabled_14=ctx->owner->unknown14;
    fields.descriptor_flags_13=record->raw[0x13u];
    fields.copy_enabled_2c=ctx->owner->correction_byte;
    fields.descriptor_token_18=rawptr(record);
    // The preview is called below through the actual native
    // four-arg RET16 fixture, not duplicated in the portable model.
    fields.preview_token_34=fields.descriptor_token_18^0x80000000u;
    Sink sink(*ctx,index);
    try {
        ctx->statuses[index]=short7::run_type7_direct_calls(
            fields,ctx->net[index],0u,ctx->predicted[index],0u,
            signed_count(record->read16(0x10u)),sink);
        const auto value=short7::read_signed_short(
            ctx->predicted[index],0u);
        ctx->authorized_preview=static_cast<std::int16_t>(value);
        ctx->owner->authorize_short(&ctx->authorized_preview);
        ctx->owner->FormatShortIfSelected(
            0u,rawptr(&ctx->authorized_preview),0u,0u);
    } catch(...) {
        // Host-only refusal; original unchecked x86 pointer faults
        // are deliberately not modeled by the safe fixture.
        ++ctx->bad;
    }
}
int fail(unsigned where,const char* why) {
    std::fprintf(stderr,
      "FAIL Phase1676 native source type7 integration #%u: %s\n",
      where,why);
    return static_cast<int>(where);
}
void setup(sw::Record& r,std::uint32_t sequence) noexcept {
    r.raw.fill(0u);
    r.write32(0u,7u);
    r.write32(0x08u,0x100u);
    r.write32(0x0Cu,0x200u);
    r.write16(0x10u,2u);
    r.write32(sw::kOriginalRecordSequenceOffset,sequence);
}
} // namespace

int main() {
    fmt::Fixture owner{};
    std::array<sw::Record,2> records{};
    setup(records[0],0u);
    setup(records[1],0u);
    Context ctx{};
    ctx.owner=&owner;
    ctx.records=&records;
    owner.ops.user=&ctx;
    owner.ops.on_opaque_case=&on_type7;
    owner.format_ops.user=&ctx;
    owner.format_ops.on_format=&on_format;
    owner.unknown14=1u;
    owner.correction_byte=1u;
    owner.mode=0u;
    owner.left_index=0u;
    owner.right_index=1u;
    sw::Node node{};
    node.unknown8=0xABCDEF01u;
    std::uintptr_t before=0u,after=0u;

    // First record differs and needs two WORD corrections. Second
    // record is already equal; only it is selected for preview.
    ctx.net[0]={0xFF,0xFF,0xFE,0xFF};
    ctx.predicted[0]={0x34,0x12,0xFE,0xFF};
    ctx.net[1]={0x00,0x80,0x33,0x00};
    ctx.predicted[1]=ctx.net[1];
    owner.diagnostic_record=rawptr(&records[1]);
    __asm mov before,esp
    owner.Dispatch(1u,&node,records.data(),2);
    __asm mov after,esp
    if(before!=after || ctx.bad || ctx.seen!=2u ||
       ctx.statuses[0]!=0u || ctx.statuses[1]!=1u ||
       ctx.predicted[0]!=ctx.net[0] ||
       ctx.event_counts[0]!=2u || ctx.event_counts[1]!=1u ||
       ctx.events[0][0].kind!=short7::TraceKind::Difference ||
       ctx.events[0][1].kind!=short7::TraceKind::Status ||
       ctx.events[1][0].kind!=short7::TraceKind::Status ||
       ctx.previews!=1u || ctx.preview_value!=-32768 ||
       owner.cached_node_word!=0u)
        return fail(1,"compare -> word-copy -> diagnostic -> selected preview");

    // Copy disabled: diagnostics must observe the still-uncorrected
    // short word, while the independent selection gate still works.
    ctx.reset();
    owner.correction_byte=0u;
    ctx.net[0]={0x22,0x00,0x00,0x00};
    ctx.predicted[0]={0x11,0x00,0x00,0x00};
    owner.diagnostic_record=rawptr(&records[0]);
    __asm mov before,esp
    owner.Dispatch(2u,&node,records.data(),2);
    __asm mov after,esp
    if(before!=after || ctx.bad || ctx.statuses[0]!=0u ||
       ctx.predicted[0][0]!=0x11u ||
       ctx.event_counts[0]!=2u ||
       ctx.events[0][0].predicted!=17 ||
       ctx.events[0][0].net!=34 ||
       ctx.events[0][0].predicted_minus_net!=-17 ||
       ctx.previews!=1u || ctx.preview_value!=17)
        return fail(2,"copy-disabled mismatch and original diagnostic ordering");

    // Disabled comparison returns status0, but enabled correction
    // still copies. Diagnostics are disabled; preview is separately
    // gated by this+0x34 == this+0x18.
    ctx.reset();
    owner.unknown14=0u;
    owner.correction_byte=1u;
    ctx.net[0]={0xFF,0xFF,0x55,0x00};
    ctx.predicted[0]={0x34,0x12,0x99,0x00};
    __asm mov before,esp
    owner.Dispatch(3u,&node,records.data(),2);
    __asm mov after,esp
    if(before!=after || ctx.bad || ctx.statuses[0]!=0u ||
       ctx.predicted[0]!=ctx.net[0] ||
       ctx.event_counts[0]!=0u ||
       ctx.previews!=1u || ctx.preview_value!=-1)
        return fail(3,"disabled compare independent of correction and preview");

    // Original descriptor +0x13 bit0x04 forces equality, so even a
    // mismatch is not corrected. The selected short remains raw.
    ctx.reset();
    owner.unknown14=1u;
    records[0].raw[0x13u]=0x04u;
    ctx.net[0]={0xFF,0xFF,0x66,0x00};
    ctx.predicted[0]={0x34,0x12,0x77,0x00};
    __asm mov before,esp
    owner.Dispatch(4u,&node,records.data(),2);
    __asm mov after,esp
    if(before!=after || ctx.bad || ctx.statuses[0]!=1u ||
       ctx.predicted[0][0]!=0x34u ||
       ctx.event_counts[0]!=1u ||
       ctx.events[0][0].kind!=short7::TraceKind::Status ||
       ctx.previews!=1u || ctx.preview_value!=0x1234)
        return fail(4,"descriptor force-match must suppress correction");

    // Flag 0x0200 filters the selected first record before type-7
    // native callback; selected-record preview must not leak.
    ctx.reset();
    records[0].raw[0x13u]=0x02u;
    owner.diagnostic_record=rawptr(&records[0]);
    __asm mov before,esp
    owner.Dispatch(5u,&node,records.data(),2);
    __asm mov after,esp
    if(before!=after || ctx.bad || ctx.seen!=1u ||
       ctx.statuses[0]!=99u || ctx.previews!=0u ||
       owner.events.flag_mode_skips==0u)
        return fail(5,"flag-skipped selected record must not preview");

    std::puts("PASS Phase1676 native MSVC x86 original-free: source-attested "
              "type7 compare/copy/diagnostic/preview order composed with "
              "Phase1672 RET16 typed dispatcher and Phase1675 RET16 "
              "signed-short selected preview; copy and flag gates, "
              "signed -32768 and -1, ESP stable; no retail game executed");
    return 0;
}
