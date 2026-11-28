// Phase1675: ORIGINAL-FREE MSVC x86 diagnostics: two original
// __thiscall FOUR-DWORD RET16 methods 0x24198360 (binary value raw)
// and 0x24198380 (short sign-extended *arg2 pointer).
// Enforces source record condition this+0x34 == this+0x18,
// original labels and source-exact forwarded values.
// Composes with Phase1672 original-free typed case callbacks.
// No proprietary original DLL/machine code/engine function is run.
#include "native_original_postnetwork_scalar_formatters.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1675 scalar original formatter requires native MSVC x86
#endif
namespace fmt=sin_independent::original_postnetwork_scalar_formatters;
namespace sw=sin_independent::original_postnetwork_type_switch;
namespace {
int fail(int id,const char* why) {
    std::fprintf(stderr,
      "FAIL Phase1675 original-free native scalar formatters x86 "
      "#%d: %s\n",id,why);
    return id;
}
std::uint32_t rawptr(const void* ptr) noexcept {
    return static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(ptr));
}
struct Event {
    fmt::FormatKind kind=fmt::FormatKind::Binary;
    std::array<char,32> label{};
    std::int32_t value=0;
};
struct Trace {
    fmt::Fixture* owner=nullptr;
    std::array<Event,24> emitted{};
    unsigned count=0u;
    unsigned bad=0u;
    std::int16_t short_value=-32768;
    void reset() noexcept {
        emitted.fill(Event{});
        count=0u;bad=0u;
    }
};
void on_format(void* user,void* raw_owner,
               fmt::FormatKind kind,const char* label,
               std::int32_t value) noexcept {
    auto* t=static_cast<Trace*>(user);
    if(raw_owner!=t->owner || !label || t->count>=t->emitted.size()) {
        ++t->bad;return;
    }
    Event& e=t->emitted[t->count++];
    e.kind=kind;
    std::strncpy(e.label.data(),label,e.label.size()-1u);
    e.value=value;
}
bool is(const Trace& t,unsigned index,fmt::FormatKind kind,
        const char* text,std::int32_t value) noexcept {
    return index<t.count &&
        t.emitted[index].kind==kind &&
        std::strcmp(t.emitted[index].label.data(),text)==0 &&
        t.emitted[index].value==value;
}
void setup(sw::Record& r,std::uint32_t type,
           std::int16_t length=0) noexcept {
    r.raw.fill(0u);
    r.write32(0u,type);
    r.write32(0x08u,0x100u+type);
    r.write32(0x0Cu,0x200u+type);
    r.write16(0x10u,static_cast<std::uint16_t>(length));
    r.write32(sw::kOriginalRecordSequenceOffset,0u);
}
void on_opaque_case(void* user,std::uint8_t case_index,
             std::uint32_t type,sw::Record* record,
             std::uint32_t,std::uint32_t,
             std::int16_t) noexcept {
    auto* t=static_cast<Trace*>(user);
    if(!t->owner || !record || type!=case_index ||
       (case_index!=7u && case_index!=8u && case_index!=9u)) {
        ++t->bad;return;
    }
    // Case specialists are still OPAQUE. This callback is a
    // DIAGNOSTIC, not a reconstruction of their case algorithms.
    // It chooses a synthetic second argument and demonstrates
    // the real native 4-stack-DWORD helper ABI and selected-record
    // condition while the Phase1672 shell owns current_record.
    if(case_index==7u) {
        t->owner->authorize_short(&t->short_value);
        t->owner->FormatShortIfSelected(
            0x55u,rawptr(&t->short_value),0x66u,0x77u);
    } else {
        const auto synthetic_value=
            case_index==8u?0x000005u:0x00000Cu;
        t->owner->FormatBinaryIfSelected(
            0x55u,synthetic_value,0x66u,0x77u);
    }
}
} // namespace
int main() {
    fmt::Fixture owner{};
    Trace t{};
    t.owner=&owner;
    fmt::FormatOps ops{};
    ops.user=&t;
    ops.on_format=&on_format;
    owner.format_ops=ops;
    const auto base=reinterpret_cast<std::uintptr_t>(&owner);
    if(reinterpret_cast<std::uintptr_t>(&owner.diagnostic_record)-base!=
       fmt::kSourceSelectedRecordOffset ||
       reinterpret_cast<std::uintptr_t>(&owner.current_record)-base!=
       fmt::kSourceCurrentRecordOffset)
        return fail(1,"original this+0x18/+0x34 DWORD offset drift");

    std::uintptr_t before=0u,after=0u;
    owner.current_record=0x11223344u;
    owner.diagnostic_record=0x55667788u;
    __asm mov before,esp
    owner.FormatBinaryIfSelected(
        0x111u,0xFFFFFFFEu,0x333u,0x444u);
    __asm mov after,esp
    if(before!=after || t.count!=0u ||
       owner.format_events.record_mismatch_skips!=1u)
        return fail(2,"mismatched selected/current record must bypass binary");

    owner.diagnostic_record=owner.current_record;
    __asm mov before,esp
    owner.FormatBinaryIfSelected(
        0x111u,0xFFFFFFFEu,0x333u,0x444u);
    __asm mov after,esp
    if(before!=after || t.count!=1u || t.bad ||
       !is(t,0u,fmt::FormatKind::Binary,
           "binary (%i bytes)",-2))
        return fail(3,"four-native-DWORD binary RET16 raw arg2/label ABI");

    // Input should preserve all original 32 bits. No truncation,
    // implicit multiplier, or source-code guessed format result.
    __asm mov before,esp
    owner.FormatBinaryIfSelected(1u,0x7FFFFFFFu,2u,3u);
    __asm mov after,esp
    if(before!=after || t.count!=2u ||
       !is(t,1u,fmt::FormatKind::Binary,
           "binary (%i bytes)",2147483647))
        return fail(4,"binary arg2 full native DWORD not forwarded unchanged");

    owner.authorize_short(&t.short_value);
    __asm mov before,esp
    owner.FormatShortIfSelected(
        0x100u,rawptr(&t.short_value),0x200u,0x300u);
    __asm mov after,esp
    if(before!=after || t.count!=3u ||
       !is(t,2u,fmt::FormatKind::Short,
           "short (%i)",-32768))
        return fail(5,"signed native MOVSX WORD PTR [arg2] and RET16");

    t.short_value=32767;
    __asm mov before,esp
    owner.FormatShortIfSelected(
        0u,rawptr(&t.short_value),0u,0u);
    __asm mov after,esp
    if(before!=after || t.count!=4u ||
       !is(t,3u,fmt::FormatKind::Short,
           "short (%i)",32767))
        return fail(6,"positive max signed 16-bit value not preserved");
    t.short_value=-1;
    owner.FormatShortIfSelected(0u,rawptr(&t.short_value),0u,0u);
    if(t.count!=5u ||
       !is(t,4u,fmt::FormatKind::Short,"short (%i)",-1))
        return fail(7,"signed -1 original word expanded to -1 DWORD");

    // Exact equality only: matching fake pointer+4 is not enough.
    owner.diagnostic_record=owner.current_record+4u;
    owner.FormatShortIfSelected(0u,rawptr(&t.short_value),0u,0u);
    if(t.count!=5u ||
       owner.format_events.record_mismatch_skips!=2u)
        return fail(8,"short method selected record gate not exact DWORD equality");
    owner.diagnostic_record=owner.current_record;

    const auto unowned_before=owner.format_events.unowned_short_refusals;
    owner.FormatShortIfSelected(
        0u,rawptr(&t.short_value)+2u,0u,0u);
    if(t.count!=5u ||
       owner.format_events.unowned_short_refusals!=unowned_before+1u)
        return fail(9,"unknown short pointer must not dereference retail address");
    owner.authorize_short(nullptr);
    owner.FormatShortIfSelected(
        0u,rawptr(&t.short_value),0u,0u);
    if(t.count!=5u ||
       owner.format_events.unowned_short_refusals!=unowned_before+2u)
        return fail(10,"revoked invented short-pointer authority not enforced");

    owner.format_ops.on_format=nullptr;
    const auto miss_before=owner.format_events.missing_callback_refusals;
    owner.FormatBinaryIfSelected(0u,7u,0u,0u);
    if(t.count!=5u ||
       owner.format_events.missing_callback_refusals!=miss_before+1u)
        return fail(11,"original opaque formatter missing backend safely refused");
    owner.format_ops=ops;

    // Real Phase1672 native typed shell passes each selected record
    // through an invented explicit callback. The callback then
    // exercises the REAL Phase1675 4-arg RET16 methods with selection
    // this+0x18/current versus this+0x34/debug target.
    owner.ops.user=&t;
    owner.ops.on_opaque_case=&on_opaque_case;
    owner.mode=0u;
    owner.left_index=0u;
    owner.right_index=1u;
    owner.left_bias=owner.right_bias=0u;
    sw::Node node{};
    node.unknown8=0xDEADBEEFu;
    std::array<sw::Record,3> records{};
    setup(records[0],8u,5);
    setup(records[1],9u,3);
    setup(records[2],7u,-123);
    // Select ONLY record0 as originally this+0x34. All three are
    // visited, but diagnostic gate triggers only for record0.
    owner.diagnostic_record=rawptr(&records[0]);
    t.reset();t.short_value=-123;
    __asm mov before,esp
    owner.Dispatch(1u,&node,records.data(),3);
    __asm mov after,esp
    if(before!=after || t.bad || t.count!=1u ||
       !is(t,0u,fmt::FormatKind::Binary,
           "binary (%i bytes)",5) ||
       owner.current_record!=rawptr(&records[2]) ||
       owner.cached_node_word!=0u)
        return fail(12,"nested source typed case8 selection and RET16/RET16");
    // Select last record (type7) with signed 16-bit -123.
    owner.diagnostic_record=rawptr(&records[2]);
    t.reset();
    __asm mov before,esp
    owner.Dispatch(2u,&node,records.data(),3);
    __asm mov after,esp
    if(before!=after || t.bad || t.count!=1u ||
       !is(t,0u,fmt::FormatKind::Short,
           "short (%i)",-123))
        return fail(13,"nested source type7 short diagnostic and selected record");
    // Select middle record (type9), here synthetic binary byte
    // count is known 12. Do NOT infer unknown game-case format.
    owner.diagnostic_record=rawptr(&records[1]);
    t.reset();
    __asm mov before,esp
    owner.Dispatch(3u,&node,records.data(),3);
    __asm mov after,esp
    if(before!=after || t.bad || t.count!=1u ||
       !is(t,0u,fmt::FormatKind::Binary,
           "binary (%i bytes)",12))
        return fail(14,"nested source type9 selected record diagnostic");

    std::puts("PASS Phase1675 original-free native MSVC x86: "
              "two source-authentic original 30B binary and 33B "
              "signed-short formatter gates, real four-DWORD "
              "__thiscall RET16, this+0x34 selected record versus "
              "this+0x18, exact original binary (%i bytes) and "
              "short (%i) literals, unmodified binary argument2, "
              "signed MOVSX short pointer, missing/revoked backend "
              "refusal, and nested genuine Phase1672 typed dispatcher "
              "RET16 -> scalar helper RET16 source call boundary; "
              "zero retail native method calls");
    return 0;
}
