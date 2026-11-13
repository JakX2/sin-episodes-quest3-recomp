#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
#include "sin_compat/original_client_snapshot_type2.hpp"

namespace type2 = sin_compat::original_client_snapshot_type2;
using Bytes = type2::Bytes;
struct RecordingSink final : type2::ITraceSink {
    std::vector<type2::TraceEvent> events;
    void on_event(const type2::TraceEvent& event) override {
        events.push_back(event);
    }
};
static Bytes bytes(const std::string& str, std::size_t capacity = 0u) {
    Bytes value(str.begin(), str.end());
    value.push_back(0);
    value.resize(capacity > value.size() ? capacity : value.size(), 0xe5u);
    return value;
}
static type2::OriginalFields active() {
    return {1u, 0u, 1u, 0x100u, 0x200u};
}

static void verify_three_native_entrypoints() {
    static_assert(type2::kNativeCompareVA == 0x24198B80u);
    static_assert(type2::kNativeDiagnosticsVA == 0x24197D00u);
    static_assert(type2::kNativePreviewVA == 0x241984A0u);
    static_assert(type2::kCompareCaller == 0x24199883u);
    static_assert(type2::kDiagnosticsCaller == 0x24199899u);
    static_assert(type2::kPreviewCaller == 0x241998A3u);
}
static void check_original_equality_and_gate_order() {
    auto a = active();
    assert(type2::compare_status(a,bytes("Same"),0,bytes("Same"),0) == 1);
    assert(type2::compare_status(a,bytes("Same"),0,bytes("same"),0) == 0);
    assert(type2::compare_status(a,bytes("abc"),0,bytes("abcd"),0) == 0);
    assert(type2::compare_status(a,bytes(""),0,bytes(""),0) == 1);
    assert(type2::compare_status(a,bytes("a\0unreachable",13),0,
                                 bytes("a"),0) == 1);
    a.byte_14 = 0;
    assert(type2::compare_status(a,{},888,{},888) == 0);
    a.byte_14 = 1; a.descriptor_byte_13 = 4;
    assert(type2::compare_status(a,{},888,{},888) == 1);
    a.descriptor_byte_13 = 0;
    bool threw = false;
    try { type2::compare_status(a,Bytes{'a'},0,bytes("a"),0); }
    catch (const std::runtime_error&) { threw = true; }
    assert(threw);
}
static void check_original_diagnostic_branches() {
    auto a=active();
    RecordingSink sink;
    type2::diagnostics(a,0u,bytes("old"),0u,bytes("new"),0u,sink);
    assert(sink.events.size()==2);
    assert(sink.events[0].kind==type2::TraceKind::Difference);
    assert(sink.events[0].net=="new" && sink.events[0].predicted=="old");
    assert(sink.events[1].kind==type2::TraceKind::Status);
    assert(sink.events[1].predicted=="old");
    assert(sink.events[1].operation==0u);
    sink.events.clear();
    type2::diagnostics(a,1u,bytes("same"),0u,bytes("same"),0u,sink);
    assert(sink.events.size()==1);
    assert(sink.events[0].kind==type2::TraceKind::Status);
    assert(sink.events[0].operation==1u);
    sink.events.clear();
    a.byte_14=0;
    type2::diagnostics(a,0u,{},99u,{},99u,sink);
    assert(sink.events.empty());
}
static void check_preview_only_opaque_DWORD_comparison() {
    auto a=active();
    RecordingSink sink;
    type2::preview(a,{},999u,sink);
    assert(sink.events.empty());
    a.dword_34 = a.dword_18;
    type2::preview(a,bytes("visible"),0u,sink);
    assert(sink.events.size()==1);
    assert(sink.events[0].kind==type2::TraceKind::Preview);
    assert(sink.events[0].predicted=="visible");
    a.byte_14=0; // Unlike diagnostics, native preview ignores +0x14.
    type2::preview(a,bytes("still visible"),0u,sink);
    assert(sink.events.size()==2);
}
static void check_phase212_to_phase215_four_direct_calls() {
    auto a=active();
    RecordingSink sink;
    Bytes destination=bytes("old",16);
    const Bytes net=bytes("fresh");
    const auto status=type2::run_direct_type2_calls(a,net,0,destination,0,sink);
    assert(status==0);
    assert(type2::safe_c_string(destination,0)=="fresh");
    // The native diagnostic CALL is AFTER the Phase213 copy CALL.
    assert(sink.events.size()==2);
    assert(sink.events[0].net=="fresh" &&
           sink.events[0].predicted=="fresh");
    assert(sink.events[1].predicted=="fresh");
    sink.events.clear();
    destination=bytes("already",16);
    auto skipped=type2::run_direct_type2_calls(a,bytes("already"),0,destination,0,sink);
    assert(skipped==1u);
    assert(type2::safe_c_string(destination,0)=="already");
    assert(sink.events.size()==1 && sink.events[0].operation==1u);

    sink.events.clear();
    a.byte_2c=0u; // Original separate copy gate.
    destination=bytes("old",16);
    auto no_write=type2::run_direct_type2_calls(a,net,0,destination,0,sink);
    assert(no_write==0u && type2::safe_c_string(destination,0)=="old");
    assert(sink.events[0].net=="fresh" && sink.events[0].predicted=="old");

    sink.events.clear();
    a.byte_14=0; a.dword_18=a.dword_34;
    destination=bytes("old",16);
    const auto inactive=type2::run_direct_type2_calls(a,net,0,destination,0,sink);
    assert(inactive==0u && type2::safe_c_string(destination,0)=="old");
    assert(sink.events.size()==1 &&
           sink.events[0].kind==type2::TraceKind::Preview);
}
static void check_original_free_host_refusal() {
    bool threw=false;
    try {type2::safe_c_string(Bytes{'a'},0);}
    catch (const std::runtime_error&) {threw=true;}
    assert(threw);
    threw=false;
    try {type2::safe_c_string(Bytes{},0);}
    catch (const std::out_of_range&) {threw=true;}
    assert(threw);
}
int main() {
    verify_three_native_entrypoints();
    check_original_equality_and_gate_order();
    check_original_diagnostic_branches();
    check_preview_only_opaque_DWORD_comparison();
    check_phase212_to_phase215_four_direct_calls();
    check_original_free_host_refusal();
}
