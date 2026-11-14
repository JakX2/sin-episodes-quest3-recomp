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
#include "sin_compat/original_client_snapshot_dword.hpp"
namespace d=sin_compat::original_client_snapshot_dword;
namespace i=sin_compat::original_client_snapshot_binary_integer;
using Bytes=d::Bytes;
struct Collector: d::IMismatchSink,i::ITraceSink,d::IPreviewSink {
 std::vector<d::FirstMismatch> pre_copy;
 std::vector<i::TraceEvent> post_copy;
 std::vector<d::PreviewEvent> preview_events;
 void first_mismatch(const d::FirstMismatch& x) override {pre_copy.push_back(x);}
 void on_event(const i::TraceEvent& x) override {post_copy.push_back(x);}
 void on_preview(const d::PreviewEvent& x) override {preview_events.push_back(x);}
};
struct Resolver:i::ISymbolHost {
 bool hit=true;int query_count=0;int basename_count=0;
 std::optional<std::string> find_name(std::int32_t value) override {
  ++query_count; assert(value==42); return hit ? std::optional<std::string>{"sub/hello.ext"} : std::nullopt;
 }
 std::string q_file_base(const std::string& name,std::size_t cap) override {
  ++basename_count;assert(name=="sub/hello.ext" && cap==512u);return "hello";
 }
};
static Bytes dwords(std::initializer_list<std::uint32_t> vals) {
 Bytes out;
 for(auto v:vals)for(unsigned shift:{0u,8u,16u,24u})
  out.push_back(static_cast<std::uint8_t>(v>>shift));
 return out;
}
static d::OriginalFields fields() {
 return {1u,0u,1u,0x1234u,0xabceu};
}
static void addresses() {
 static_assert(d::kCompareVA==0x24197DB0u);
 static_assert(d::kCopyVA==0x24198A10u);
 static_assert(d::kDiagnosticVA==0x24197B60u);
 static_assert(d::kPreviewVA==0x241983B0u);
 static_assert(d::kCompareCaller==0x241999BCu);
 static_assert(d::kCopyCaller==0x241999CBu);
 static_assert(d::kDiagnosticCaller==0x241999D6u);
 static_assert(d::kPreviewCaller==0x241999E5u);
}
static void test_compare_and_first_mismatch() {
 auto f=fields();Collector c;
 assert(d::compare(f,dwords({1,2}),0,dwords({1,2}),0,2,c)==1);
 assert(c.pre_copy.empty());
 assert(d::compare(f,dwords({1,2,3}),0,dwords({1,4,5}),0,3,c)==0);
 assert(c.pre_copy.size()==1);
 assert(c.pre_copy[0].index==1 && c.pre_copy[0].predicted==2 &&
        c.pre_copy[0].net==4 && c.pre_copy[0].diff==-2);
 c.pre_copy.clear();
 assert(d::compare(f,dwords({1}),0,dwords({2}),0,99,c)==0);
 assert(c.pre_copy.size()==1); // Early mismatch; no demand for 99 entries.
 c.pre_copy.clear();
 assert(d::compare(f,{},999,{},999,0,c)==1);
 assert(d::compare(f,{},999,{},999,-1,c)==1);
 f.descriptor_flags_13=4u;
 assert(d::compare(f,{},999,{},999,100,c)==1);
 f.enabled_14=0u;
 assert(d::compare(f,{},999,{},999,100,c)==0);
 assert(c.pre_copy.empty());
 f.enabled_14=1u;f.descriptor_flags_13=0u;
 bool bounds=false;
 try {d::compare(f,dwords({1}),0,dwords({1}),0,2,c);}
 catch(const std::out_of_range&){bounds=true;}
 assert(bounds);
}
static void test_wrapping_native_difference() {
 auto f=fields();Collector c;
 assert(d::compare(f,dwords({0x80000000u}),0,dwords({1}),0,1,c)==0);
 assert(c.pre_copy[0].diff==std::numeric_limits<std::int32_t>::max());
 assert(c.pre_copy[0].predicted==std::numeric_limits<std::int32_t>::min());
}
static void test_preview_resolution_and_gates() {
 auto f=fields();Collector c;Resolver r;
 d::preview(f,{},999,c,nullptr);
 assert(c.preview_events.empty());
 f.preview_token_34=f.descriptor_token_18;
 f.enabled_14=0u;
 d::preview(f,dwords({42}),0,c,nullptr);
 assert(c.preview_events.size()==1 &&
        c.preview_events[0].kind==d::PreviewKind::Numeric);
 f.descriptor_flags_13=8u;
 d::preview(f,dwords({42}),0,c,&r);
 assert(c.preview_events.size()==2 &&
        c.preview_events[1].kind==d::PreviewKind::Symbol &&
        c.preview_events[1].symbol=="hello");
 assert(r.query_count==1 && r.basename_count==1);
 r.hit=false;
 d::preview(f,dwords({42}),0,c,&r);
 assert(c.preview_events.back().kind==d::PreviewKind::Numeric);
 assert(r.query_count==2 && r.basename_count==1);
 bool threw=false;
 try {d::preview(f,dwords({42}),0,c,nullptr);}
 catch(const std::runtime_error&){threw=true;}
 assert(threw);
}
static void test_order_pre_compare_copy_then_integer_diagnostic() {
 auto f=fields();Collector c;Resolver r;
 Bytes pred=dwords({7,50});const Bytes net=dwords({42,60});
 const auto operation=d::run_type5_direct_calls(
  f,net,0,pred,0,2,c,c,c,&r);
 assert(operation==0 && pred==net);
 assert(c.pre_copy.size()==1 && c.pre_copy[0].index==0);
 assert(c.pre_copy[0].predicted==7 && c.pre_copy[0].net==42);
 assert(c.pre_copy[0].diff==-35); // Comparator logs BEFORE copy.
 assert(c.post_copy.size()==2 && c.post_copy[0].predicted==42 &&
        c.post_copy[0].net==42 && c.post_copy[0].difference==0);
 assert(c.preview_events.empty());
 c.pre_copy.clear();c.post_copy.clear();
 const auto equal=d::run_type5_direct_calls(f,net,0,pred,0,2,c,c,c,&r);
 assert(equal==1 && c.pre_copy.empty() && c.post_copy.size()==1);
 f.copy_enabled_2c=0u;c.post_copy.clear();
 pred=dwords({7,50});
 const auto skipped=d::run_type5_direct_calls(
  f,net,0,pred,0,2,c,c,c,&r);
 assert(skipped==0 && d::read_u32(pred,0)==7u);
 assert(c.post_copy[0].predicted==7 && c.post_copy[0].net==42);
}
static void test_full_symbol_chain_and_skip() {
 auto f=fields();f.descriptor_flags_13=8u;
 f.preview_token_34=f.descriptor_token_18;
 Collector c;Resolver r;
 Bytes pred=dwords({42});const Bytes net=dwords({42});
 assert(d::run_type5_direct_calls(f,net,0,pred,0,1,c,c,c,&r)==1);
 assert(c.pre_copy.empty() && c.post_copy.size()==1);
 assert(c.post_copy[0].kind==i::TraceKind::IntegerSymbol);
 assert(c.post_copy[0].symbol=="hello");
 assert(c.preview_events.size()==1 &&
        c.preview_events[0].kind==d::PreviewKind::Symbol);
 assert(r.query_count==2 && r.basename_count==2);
}
static void test_disabled_and_negative_copy_refusal() {
 auto f=fields();Collector c;
 Bytes pred=dwords({1});const Bytes net=dwords({1});
 assert(d::run_type5_direct_calls(f,net,0,pred,0,-2,c,c,c,nullptr)==1);
 assert(c.post_copy.size()==1);
 f.enabled_14=0u;bool refused=false;
 try {d::run_type5_direct_calls(f,net,0,pred,0,-2,c,c,c,nullptr);}
 catch(const std::overflow_error&){refused=true;}
 assert(refused);
 f.copy_enabled_2c=0u;
 assert(d::run_type5_direct_calls(f,net,0,pred,0,-2,c,c,c,nullptr)==0);
}
int main(){
 addresses();test_compare_and_first_mismatch();test_wrapping_native_difference();
 test_preview_resolution_and_gates();test_order_pre_compare_copy_then_integer_diagnostic();
 test_full_symbol_chain_and_skip();test_disabled_and_negative_copy_refusal();
}
