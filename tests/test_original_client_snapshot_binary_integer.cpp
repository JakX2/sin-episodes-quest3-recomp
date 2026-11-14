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
#include "sin_compat/original_client_snapshot_binary_integer.hpp"

namespace native=sin_compat::original_client_snapshot_binary_integer;
using Bytes=native::Bytes;

struct Sink final : native::ITraceSink {
    std::vector<native::TraceEvent> events;
    void on_event(const native::TraceEvent& event) override {
        events.push_back(event);
    }
};
struct Resolver final : native::ISymbolHost {
    bool exists{true};
    bool oversized{false};
    int lookups{};
    int bases{};
    std::int32_t last_value{};
    std::size_t last_capacity{};
    std::optional<std::string> find_name(std::int32_t value) override {
        ++lookups;
        last_value=value;
        return exists ? std::optional<std::string>{"folder/item.name"} : std::nullopt;
    }
    std::string q_file_base(const std::string& name,
                            std::size_t capacity) override {
        assert(name=="folder/item.name");
        ++bases;
        last_capacity=capacity;
        return oversized ? std::string(512,'x') : "item";
    }
};
static native::OriginalFields active() {
    return {1u,0u,1u,0x12345678u,0xabcdefu};
}
static void test_original_entrypoints() {
    static_assert(native::kCompareVA==0x24197A90u);
    static_assert(native::kBinaryDiagnosticVA==0x24197AD0u);
    static_assert(native::kIntegerDiagnosticVA==0x24197B60u);
    static_assert(native::kBinaryPreviewVA==0x24198360u);
}
static void test_fixed_byte_compare_and_native_short_circuit() {
    auto fields=active();
    assert(native::compare_bytes(fields,4u,Bytes{1,2,3,4},0u,
                                 Bytes{1,2,3,4},0u)==1u);
    assert(native::compare_bytes(fields,4u,Bytes{1,2,3,4},0u,
                                 Bytes{1,2,0,4},0u)==0u);
    assert(native::compare_bytes(fields,0u,{},999u,{},999u)==1u);
    assert(native::compare_bytes(fields,100u,Bytes{1},0u,Bytes{2},0u)==0u);
    fields.enabled_14=0u;
    assert(native::compare_bytes(fields,100u,{},999u,{},999u)==0u);
    fields.enabled_14=1u;
    fields.descriptor_flags_13=4u;
    assert(native::compare_bytes(fields,100u,{},999u,{},999u)==1u);
    fields.descriptor_flags_13=0u;
    bool refused=false;
    try {native::compare_bytes(fields,3u,Bytes{1,2},0u,Bytes{1,2},0u);}
    catch(const std::out_of_range&) {refused=true;}
    assert(refused);
}
static void test_original_binary_log_order_and_preview_gate() {
    auto fields=active();
    Sink sink;
    native::binary_diagnostics(fields,0u,12u,sink);
    assert(sink.events.size()==2u);
    assert(sink.events[0].kind==native::TraceKind::BinaryDifference);
    assert(sink.events[1].kind==native::TraceKind::BinaryStatus);
    assert(sink.events[0].byte_count==12u && sink.events[1].status==0u);
    sink.events.clear();
    native::binary_diagnostics(fields,1u,8u,sink);
    assert(sink.events.size()==1u &&
           sink.events[0].kind==native::TraceKind::BinaryStatus);
    native::binary_preview(fields,8u,sink);
    assert(sink.events.size()==1u);
    fields.preview_token_34=fields.descriptor_token_18;
    native::binary_preview(fields,8u,sink);
    assert(sink.events.size()==2u &&
           sink.events[1].kind==native::TraceKind::BinaryPreview);
    assert(sink.events[1].byte_count==8u);
    sink.events.clear();
    fields.enabled_14=0u;
    native::binary_diagnostics(fields,0u,100u,sink);
    assert(sink.events.empty());
    native::binary_preview(fields,3u,sink);
    assert(sink.events.size()==1u);
}
static void test_integer_diagnostics_and_original_symbol_branch() {
    auto fields=active();
    Sink sink;
    Resolver resolver;
    native::integer_diagnostics(fields,0u,-7,3,sink,&resolver);
    assert(sink.events.size()==2u);
    assert(sink.events[0].kind==native::TraceKind::IntegerDifference);
    assert(sink.events[0].net==3 && sink.events[0].predicted==-7);
    assert(sink.events[0].difference==-10);
    assert(sink.events[1].kind==native::TraceKind::IntegerNumeric);
    assert(resolver.lookups==0);
    sink.events.clear();
    native::integer_diagnostics(fields,1u,7,8,sink,&resolver);
    assert(sink.events.size()==1u &&
           sink.events[0].kind==native::TraceKind::IntegerNumeric);
    fields.descriptor_flags_13=8u;
    sink.events.clear();
    native::integer_diagnostics(fields,0u,42,40,sink,&resolver);
    assert(sink.events.size()==2u);
    assert(sink.events[0].kind==native::TraceKind::IntegerDifference);
    assert(sink.events[1].kind==native::TraceKind::IntegerSymbol);
    assert(sink.events[1].symbol=="item");
    assert(resolver.lookups==1 && resolver.bases==1);
    assert(resolver.last_value==42 && resolver.last_capacity==512u);
    resolver.exists=false;
    sink.events.clear();
    native::integer_diagnostics(fields,1u,22,22,sink,&resolver);
    assert(sink.events.size()==1u &&
           sink.events[0].kind==native::TraceKind::IntegerNumeric);
    assert(resolver.lookups==2 && resolver.bases==1);
    resolver.exists=true; resolver.oversized=true;
    bool refused=false;
    try {native::integer_diagnostics(fields,1u,1,1,sink,&resolver);}
    catch(const std::length_error&) {refused=true;}
    assert(refused);
    refused=false;
    try {native::integer_diagnostics(fields,1u,1,1,sink,nullptr);}
    catch(const std::runtime_error&) {refused=true;}
    assert(refused);
    fields.enabled_14=0u;
    sink.events.clear();
    native::integer_diagnostics(fields,0u,1,2,sink,nullptr);
    assert(sink.events.empty());
}
static void test_exact_32bit_wrapping_diff_and_signed_bytes() {
    const auto lo=std::numeric_limits<std::int32_t>::min();
    const auto hi=std::numeric_limits<std::int32_t>::max();
    auto fields=active();Sink sink;
    native::integer_diagnostics(fields,0u,lo,1,sink,nullptr);
    assert(sink.events[0].difference==hi);
    assert(native::signed_byte(0x00u)==0);
    assert(native::signed_byte(0x7fu)==127);
    assert(native::signed_byte(0x80u)==-128);
    assert(native::signed_byte(0xffu)==-1);
}
static void test_real_router_case9_order_and_byte_count() {
    auto fields=active();
    Sink sink;
    Bytes predicted{1,2,3,4,5,6,7,8};
    const Bytes net{1,2,3,0,5,6,7,8};
    const auto status=native::run_binary_case9_calls(
        fields,2u,net,0u,predicted,0u,sink);
    assert(status==0u);
    assert(predicted==net);
    assert(sink.events.size()==2u);
    assert(sink.events[0].kind==native::TraceKind::BinaryDifference);
    assert(sink.events[0].byte_count==8u);
    sink.events.clear();
    fields.preview_token_34=fields.descriptor_token_18;
    assert(native::run_binary_case9_calls(fields,2u,net,0u,predicted,0u,sink)==1u);
    assert(sink.events.size()==2u);
    assert(sink.events[0].kind==native::TraceKind::BinaryStatus);
    assert(sink.events[1].kind==native::TraceKind::BinaryPreview);
    bool refused=false;
    try {native::run_binary_case9_calls(fields,0x40000000u,net,0u,
                                        predicted,0u,sink);}
    catch(const std::overflow_error&) {refused=true;}
    assert(refused);
}
static void test_real_router_case8_post_copy_integer_value() {
    auto fields=active();
    Sink sink; Resolver resolver;
    Bytes predicted{0x80u,3u,4u};
    const Bytes net{0xffu,3u,4u};
    auto status=native::run_integer_case8_calls(
        fields,3u,net,0u,predicted,0u,sink,&resolver);
    assert(status==0u && predicted==net);
    assert(sink.events.size()==2u);
    assert(sink.events[0].kind==native::TraceKind::IntegerDifference);
    assert(sink.events[0].predicted==-1 &&
           sink.events[0].net==-1 &&
           sink.events[0].difference==0);
    assert(sink.events[1].kind==native::TraceKind::IntegerNumeric);
    sink.events.clear();
    fields.copy_enabled_2c=0u;
    predicted={0x80u,3u,4u};
    status=native::run_integer_case8_calls(
        fields,3u,net,0u,predicted,0u,sink,&resolver);
    assert(status==0u && predicted[0]==0x80u);
    assert(sink.events[0].predicted==-128 &&
           sink.events[0].net==-1 &&
           sink.events[0].difference==-127);
    fields.descriptor_flags_13=0x08u;
    sink.events.clear();
    native::run_integer_case8_calls(fields,3u,net,0u,predicted,0u,sink,&resolver);
    assert(sink.events[1].kind==native::TraceKind::IntegerSymbol);
    assert(resolver.last_value==-128);
    // Native still reads each first byte after a zero-count compare.
    fields.descriptor_flags_13=0u;
    sink.events.clear();
    status=native::run_integer_case8_calls(
        fields,0u,net,0u,predicted,0u,sink,&resolver);
    assert(status==1u && sink.events.size()==1u);
}
int main() {
    test_original_entrypoints();
    test_fixed_byte_compare_and_native_short_circuit();
    test_original_binary_log_order_and_preview_gate();
    test_integer_diagnostics_and_original_symbol_branch();
    test_exact_32bit_wrapping_diff_and_signed_bytes();
    test_real_router_case9_order_and_byte_count();
    test_real_router_case8_post_copy_integer_value();
}
