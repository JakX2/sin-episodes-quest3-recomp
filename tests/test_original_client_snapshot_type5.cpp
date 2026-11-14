#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include "sin_compat/original_client_snapshot_type5.hpp"

namespace native=sin_compat::original_client_snapshot_type5;
using Bytes=native::Bytes;
struct Sink final : native::ITraceSink {
    std::vector<native::TraceEvent> events;
    void on_event(const native::TraceEvent& event) override {
        events.push_back(event);
    }
};
struct Resolver final : native::ISymbolHost {
    bool found{true};
    bool oversized{false};
    int lookups{};
    int basename_calls{};
    std::int32_t last_value{};
    std::size_t last_capacity{};
    std::optional<std::string> find_name(std::int32_t value) override {
        ++lookups;
        last_value=value;
        if(!found)return std::nullopt;
        return std::string{"materials/tiles/stone.vtf"};
    }
    std::string q_file_base(const std::string& name,
                            std::size_t capacity) override {
        assert(name=="materials/tiles/stone.vtf");
        ++basename_calls;
        last_capacity=capacity;
        return oversized?std::string(512,'x'):"stone";
    }
};
static native::OriginalFields state() {
    return {1u,0u,1u,0x11110000u,0x22220000u};
}
static Bytes words(std::initializer_list<std::uint32_t> values) {
    Bytes data;
    for(auto value:values) {
        for(unsigned bits:{0u,8u,16u,24u})
            data.push_back(static_cast<std::uint8_t>((value>>bits)&0xffu));
    }
    return data;
}
static void original_addresses_and_little_endian() {
    static_assert(native::kCompareVA==0x24197DB0u);
    static_assert(native::kCopyVA==0x24198A10u);
    static_assert(native::kDiagnosticsVA==0x24197B60u);
    static_assert(native::kPreviewVA==0x241983B0u);
    static_assert(native::kCompareCaller==0x241999BCu);
    static_assert(native::kCopyCaller==0x241999CBu);
    static_assert(native::kDiagnosticsCaller==0x241999D6u);
    static_assert(native::kPreviewCaller==0x241999E5u);
    assert(native::read_signed_dword(words({0x80000000u}),0)==
           std::numeric_limits<std::int32_t>::min());
    assert(native::read_signed_dword(words({0xffffffffu}),0)==-1);
    assert(native::read_signed_dword(words({0x7fffffffu}),0)==
           std::numeric_limits<std::int32_t>::max());
}
static void compare_early_log_and_status_guards() {
    auto f=state(); Sink s;
    assert(native::compare_dwords_with_immediate_diagnostic(
        f,words({2u,3u}),0,words({2u,3u}),0,2,s)==1u);
    assert(s.events.empty());
    assert(native::compare_dwords_with_immediate_diagnostic(
        f,{},999,{},999,0,s)==1u);
    assert(native::compare_dwords_with_immediate_diagnostic(
        f,{},999,{},999,-1,s)==1u);
    f.descriptor_flags_13=4u;
    assert(native::compare_dwords_with_immediate_diagnostic(
        f,{},999,{},999,100,s)==1u);
    f.enabled_14=0u;
    assert(native::compare_dwords_with_immediate_diagnostic(
        f,{},999,{},999,100,s)==0u);
    f.enabled_14=1u;f.descriptor_flags_13=0u;
    // First mismatch is at index 1, not at index 0.
    assert(native::compare_dwords_with_immediate_diagnostic(
        f,words({9u,123u}),0,words({9u,126u}),0,2,s)==0u);
    assert(s.events.size()==1u);
    assert(s.events[0].kind==native::TraceKind::IntegerDifference);
    assert(s.events[0].status==0u);
    assert(s.events[0].net==126 && s.events[0].predicted==123);
    assert(s.events[0].difference==-3);
    s.events.clear();
    // Native early mismatch does NOT pre-check unused later words.
    assert(native::compare_dwords_with_immediate_diagnostic(
        f,words({1u}),0,words({2u}),0,100,s)==0u);
    assert(s.events.size()==1u);
    s.events.clear();
    bool refused=false;
    try {native::compare_dwords_with_immediate_diagnostic(
        f,words({1u}),0,words({1u}),0,2,s);}
    catch(const std::out_of_range&) {refused=true;}
    assert(refused);
    s.events.clear();
    assert(native::compare_dwords_with_immediate_diagnostic(
        f,words({0x80000000u}),0,words({1u}),0,1,s)==0u);
    assert(s.events.size()==1u &&
           s.events[0].difference==std::numeric_limits<std::int32_t>::max());
}
static void preview_gate_symbol_fallback_and_independent_enable() {
    auto f=state(); Sink s; Resolver resolver;
    native::preview(f,{},999,s,nullptr);
    assert(s.events.empty());
    f.preview_token_34=f.descriptor_token_18;
    f.enabled_14=0u;
    native::preview(f,words({42u}),0,s,nullptr);
    assert(s.events.size()==1u &&
           s.events[0].kind==native::TraceKind::IntegerNumeric);
    assert(s.events[0].predicted==42);
    s.events.clear();
    f.descriptor_flags_13=8u;
    native::preview(f,words({100u}),0,s,&resolver);
    assert(s.events.size()==1u &&
           s.events[0].kind==native::TraceKind::IntegerSymbol);
    assert(s.events[0].predicted==100 && s.events[0].symbol=="stone");
    assert(resolver.last_value==100 && resolver.last_capacity==512u);
    assert(resolver.lookups==1 && resolver.basename_calls==1);
    s.events.clear();
    resolver.found=false;
    native::preview(f,words({77u}),0,s,&resolver);
    assert(s.events.size()==1u &&
           s.events[0].kind==native::TraceKind::IntegerNumeric);
    assert(resolver.lookups==2 && resolver.basename_calls==1);
    resolver.found=true;resolver.oversized=true;
    bool failed=false;
    try {native::preview(f,words({88u}),0,s,&resolver);}
    catch(const std::length_error&) {failed=true;}
    assert(failed);
    failed=false;
    try {native::preview(f,words({88u}),0,s,nullptr);}
    catch(const std::runtime_error&) {failed=true;}
    assert(failed);
}
static void four_original_direct_calls_preserve_both_mismatch_logs() {
    auto f=state(); Sink s;
    Bytes pred=words({1u,2u});
    const Bytes net=words({1u,3u});
    auto result=native::run_type5_direct_calls(
        f,net,0,pred,0,2u,s,nullptr);
    assert(result==0u && pred==net);
    assert(s.events.size()==3u);
    assert(s.events[0].kind==native::TraceKind::IntegerDifference);
    assert(s.events[0].predicted==2 && s.events[0].net==3);
    assert(s.events[0].difference==-1);
    // This second mismatch event is AFTER the conditional DWORD copy.
    assert(s.events[1].kind==native::TraceKind::IntegerDifference);
    assert(s.events[1].predicted==1 && s.events[1].net==1);
    assert(s.events[1].difference==0);
    assert(s.events[2].kind==native::TraceKind::IntegerNumeric &&
           s.events[2].predicted==1);
    s.events.clear();
    result=native::run_type5_direct_calls(f,net,0,pred,0,2u,s,nullptr);
    assert(result==1u);
    assert(s.events.size()==1u &&
           s.events[0].kind==native::TraceKind::IntegerNumeric);
    s.events.clear();
    f.copy_enabled_2c=0u;
    pred=words({9u,2u});
    result=native::run_type5_direct_calls(f,net,0,pred,0,2u,s,nullptr);
    assert(result==0u && native::read_signed_dword(pred,0)==9);
    assert(s.events.size()==3u);
    assert(s.events[0].difference==8 &&
           s.events[1].difference==8 &&
           s.events[2].predicted==9);
}
static void status_1_skips_copy_and_optional_both_symbol_lookups() {
    auto f=state(); Sink s; Resolver resolver;
    f.descriptor_flags_13=0x0cu; // force match + resolve symbolic names.
    f.preview_token_34=f.descriptor_token_18;
    Bytes pred=words({55u});
    const Bytes net=words({99u});
    const auto result=native::run_type5_direct_calls(
        f,net,0,pred,0,0xffffffffu,s,&resolver);
    assert(result==1u && native::read_signed_dword(pred,0)==55);
    assert(s.events.size()==2u);
    assert(s.events[0].kind==native::TraceKind::IntegerSymbol);
    assert(s.events[1].kind==native::TraceKind::IntegerSymbol);
    assert(resolver.lookups==2 && resolver.basename_calls==2);
    assert(resolver.last_value==55);
}
static void disabled_diagnostic_preview_still_acts_and_host_guard() {
    auto f=state();Sink s;
    f.enabled_14=0u;
    f.copy_enabled_2c=0u;
    f.preview_token_34=f.descriptor_token_18;
    Bytes pred=words({7u});const Bytes net=words({8u});
    assert(native::run_type5_direct_calls(
        f,net,0,pred,0,1u,s,nullptr)==0u);
    assert(s.events.size()==1u &&
           s.events[0].kind==native::TraceKind::IntegerNumeric);
    assert(s.events[0].predicted==7);
    f.preview_token_34=0u;
    f.copy_enabled_2c=1u;
    bool refused=false;
    try {native::run_type5_direct_calls(
        f,net,0,pred,0,0xffffffffu,s,nullptr);}
    catch(const std::overflow_error&) {refused=true;}
    assert(refused);
}
int main() {
    original_addresses_and_little_endian();
    compare_early_log_and_status_guards();
    preview_gate_symbol_fallback_and_independent_enable();
    four_original_direct_calls_preserve_both_mismatch_logs();
    status_1_skips_copy_and_optional_both_symbol_lookups();
    disabled_diagnostic_preview_still_acts_and_host_guard();
}
