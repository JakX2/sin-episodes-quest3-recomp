// Phase1678: real MSVC x86 ORIGINAL-FREE CDECL 3-arg formatter,
// nested under native Phase1675 four-arg __thiscall RET16 gates.
// 93B retail formatter SHA/audit established from unchanged DLL.
// Exact Q_vsnprintf and tier0 Msg calls are represented by bounded
// synthetic host-owned printf/Msg callback; no original DLL runs.
#include "native_original_postnetwork_cdecl_msg_formatter.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1678 native cdecl formatter ABI test requires MSVC x86
#endif
namespace fx=sin_independent::original_postnetwork_cdecl_msg_formatter;
namespace sw=sin_independent::original_postnetwork_type_switch;
namespace fm=sin_independent::original_postnetwork_scalar_formatters;

namespace {
int fail(int id,const char* why) {
    std::fprintf(stderr,
      "FAIL Phase1678 native original-free cdecl formatter #%d: %s\n",
      id,why);
    return id;
}
struct Trace {
    fx::Fixture* owner=nullptr;
    unsigned seen=0u;
    unsigned bad=0u;
    std::array<std::array<char,160>,12> messages{};
    bool is(unsigned idx,const char* expected) const noexcept {
        return idx<seen &&
               std::strcmp(messages[idx].data(),expected)==0;
    }
};
void on_msg(void* user,const char* text) noexcept {
    auto* t=static_cast<Trace*>(user);
    if(!t || !text || t->seen>=t->messages.size()) {
        if(t)++t->bad;
        return;
    }
    auto& line=t->messages[t->seen++];
    std::strncpy(line.data(),text,line.size()-1u);
    if(std::strlen(text)>=line.size())++t->bad;
}
void on_format(void* user,void* raw_owner,
               fm::FormatKind kind,const char* format,
               std::int32_t value) noexcept {
    auto* t=static_cast<Trace*>(user);
    if(!t || !raw_owner || raw_owner!=t->owner ||
       (kind!=fm::FormatKind::Binary &&
        kind!=fm::FormatKind::Short)) {
        if(t)++t->bad;
        return;
    }
    // The true native original 0x24197FE0 uses CDECL RET0,
    // called by two native RET16 wrappers which clean 12 bytes.
    // The compiler must keep ESP unchanged across nested calls.
    std::uintptr_t before=0u,after=0u;
    __asm mov before,esp
    fx::Fixture::FormatScalar(
        static_cast<fx::Fixture*>(raw_owner),format,value);
    __asm mov after,esp
    if(before!=after)++t->bad;
}
}
int main() {
    fx::Fixture owner{};
    Trace t{};
    t.owner=&owner;
    sw::Record record{};
    record.raw.fill(0u);
    const char* const name="sim_weapon";
    const char* const label="net";
    record.write32(fx::kOriginalRecordNameOffset,fx::rawptr(name));
    owner.current_record=fx::rawptr(&record);
    owner.diagnostic_record=owner.current_record;
    owner.diagnostic_label_word=fx::rawptr(label);
    owner.authorize(&record,name,label,17);
    fm::FormatOps formatter{};
    formatter.user=&t;
    formatter.on_format=&on_format;
    owner.format_ops=formatter;
    fx::OutputOps output{};
    output.user=&t;
    output.on_msg=&on_msg;
    owner.output_ops=output;
    const auto base=reinterpret_cast<std::uintptr_t>(&owner);
    if(reinterpret_cast<std::uintptr_t>(
            &owner.diagnostic_label_word)-base!=0x38u ||
       reinterpret_cast<std::uintptr_t>(&owner.current_record)-base!=0x18u ||
       reinterpret_cast<std::uintptr_t>(
            &owner.diagnostic_record)-base!=0x34u)
        return fail(1,"original this+0x18/0x34/0x38 fields drifted");

    std::uintptr_t before=0u,after=0u;
    __asm mov before,esp
    owner.FormatBinaryIfSelected(0x11u,5u,0x33u,0x44u);
    __asm mov after,esp
    if(before!=after || t.bad || t.seen!=1u ||
       !t.is(0u,"17 net sim_weapon : binary (5 bytes)\n") ||
       owner.output_events.emitted!=1u)
        return fail(2,"RET16 -> CDECL RET0 -> bounded 4096B -> Msg");

    std::int16_t short_value=-32768;
    owner.authorize_short(&short_value);
    __asm mov before,esp
    owner.FormatShortIfSelected(
        0x21u,fx::rawptr(&short_value),0x23u,0x24u);
    __asm mov after,esp
    if(before!=after || t.bad || t.seen!=2u ||
       !t.is(1u,"17 net sim_weapon : short (-32768)\n"))
        return fail(3,"native signed WORD MOVSX through nested cdecl");

    // Plain CDECL direct call, with three stack DWORD arguments
    // and NO callee RET immediate: caller must restore its ESP.
    __asm mov before,esp
    fx::Fixture::FormatScalar(
        &owner,fm::kOriginalBinaryFormat,-2);
    __asm mov after,esp
    if(before!=after || t.bad || t.seen!=3u ||
       !t.is(2u,"17 net sim_weapon : binary (-2 bytes)\n"))
        return fail(4,"direct original three-arg CDECL RET0 stack contract");

    // Selected record mismatch suppresses the call BEFORE the
    // formatter, so no client/global pointers are read downstream.
    owner.diagnostic_record=owner.current_record+4u;
    const auto attempted_before=owner.output_events.candidate;
    __asm mov before,esp
    owner.FormatBinaryIfSelected(0u,12u,0u,0u);
    __asm mov after,esp
    if(before!=after || t.seen!=3u ||
       owner.output_events.candidate!=attempted_before)
        return fail(5,"selected record mismatch bypasses formatter");

    // Wrong record+0x04 identity is refused, rather than
    // dereferencing arbitrary native game memory.
    owner.diagnostic_record=owner.current_record;
    record.write32(fx::kOriginalRecordNameOffset,0xDEADBEEFu);
    owner.FormatBinaryIfSelected(0u,7u,0u,0u);
    if(t.seen!=3u || owner.output_events.refused_unowned_record!=1u)
        return fail(6,"forged record name pointer not refused");
    record.write32(fx::kOriginalRecordNameOffset,fx::rawptr(name));

    // Wrong this+0x38 string-token is independently rejected.
    owner.diagnostic_label_word=0xBAD00BADu;
    owner.FormatBinaryIfSelected(0u,8u,0u,0u);
    if(t.seen!=3u || owner.output_events.refused_unowned_label!=1u)
        return fail(7,"forged this+0x38 label not refused");
    owner.diagnostic_label_word=fx::rawptr(label);

    // This safe fixture accepts only two source-pinned 1-int
    // format strings; unknown format pointers are never dereferenced.
    const auto bad_before=owner.output_events.refused_bad_format;
    fx::Fixture::FormatScalar(
        &owner,reinterpret_cast<const char*>(0x101u),9);
    if(t.seen!=3u ||
       owner.output_events.refused_bad_format!=bad_before+1u)
        return fail(8,"unknown format must not be interpreted");

    owner.output_ops.on_msg=nullptr;
    owner.FormatBinaryIfSelected(0u,9u,0u,0u);
    if(t.seen!=3u ||
       owner.output_events.refused_missing_output!=1u)
        return fail(9,"absent invented tier0 Msg callback not refused");
    owner.output_ops=output;

    // Distinct original field categories stay distinct:
    // global context+0x18, this+0x38, record+0x04, formatted body.
    const char* const label2="prediction";
    const char* const name2="record7";
    record.write32(fx::kOriginalRecordNameOffset,fx::rawptr(name2));
    owner.diagnostic_label_word=fx::rawptr(label2);
    owner.authorize(&record,name2,label2,-9);
    short_value=32767;
    owner.FormatShortIfSelected(
        0u,fx::rawptr(&short_value),0u,0u);
    if(t.seen!=4u || t.bad ||
       !t.is(3u,"-9 prediction record7 : short (32767)\n"))
        return fail(10,"original context/label/record order and signed bounds");

    // No original DLL/process/engine module was executed.
    std::puts("PASS Phase1678 original-free MSVC x86: 93B original "
              "CDECL RET0 formatter reconstruction, 4096B bounded "
              "Q_vsnprintf analogue, exact tier0 Msg field order and "
              "literal, native RET16 -> CDECL stack invariants, "
              "signed/negative values, authorization refusals, "
              "original this+0x38 and record+0x04; zero retail calls");
    return 0;
}
