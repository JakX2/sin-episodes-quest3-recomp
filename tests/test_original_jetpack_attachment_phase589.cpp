#include "sin_compat/original_jetpack_attachment_phase589.hpp"
#include <cassert>
#include <string>
#include <vector>
using namespace sin_compat::jetpack_attachment_phase589;

struct MockHost final : Host {
  FlyingEntity* found=nullptr;
  int finishing_value=67;
  std::vector<std::string> called;
  FlyingEntity* setup_entity=nullptr;
  FlyingEntity* lookup_flying(std::string_view key,Owner&) override {
    assert(key=="ai_base_physics_flying");called.emplace_back("find:ai_base_physics_flying");return found;
  }
  void call_native_setup(FlyingEntity* ent,int a,int b,int c) override {
    assert(a==0 && b==0 && c==0);setup_entity=ent;called.emplace_back("setup:0,0,0");
  }
  void before_owner_link(FlyingEntity& entity,Owner&) override {
    assert(&entity==found);called.emplace_back("link");
  }
  int call_native_finalize(FlyingEntity& ent) override {
    assert(&ent==found);called.emplace_back("finalize:+0x64");return finishing_value;
  }
};
static void test_native_initial_state() {
 Owner o{};o.raw_fe0=900;o.raw_fe4=900;o.raw_fe8=900;
 o.raw_ff4=900;o.raw_1000=900;o.raw_1008=900;
 o.flying_1024=900;o.raw_1028=900;
 initialize_original_jetpack_fields(o);
 assert(o.raw_fe0==0&&o.raw_1008==0);
 assert(o.raw_fe4==kInvalid&&o.raw_fe8==kInvalid);
 assert(o.raw_ff4==kInvalid&&o.raw_1000==kInvalid);
 assert(o.flying_1024==kInvalid&&o.raw_1028==kInvalid);
}
static void test_native_slot_and_serial() {
 FlyingEntity a,b; a.encoded_handle=HandleTable::encode(2,7);
 HandleTable table;table.slots.resize(3);
 table.slots[2]={&a,7};
 assert(original_handle_setter(&a)==a.encoded_handle);
 assert(original_handle_setter(nullptr)==kInvalid);
 assert(original_handle_resolver(table,kInvalid)==nullptr);
 assert(original_handle_resolver(table,a.encoded_handle)==&a);
 assert(original_handle_resolver(table,HandleTable::encode(2,8))==nullptr);
 assert(original_handle_resolver(table,HandleTable::encode(2,6))==nullptr);
 assert(original_handle_resolver(table,HandleTable::encode(5,7))==nullptr);
 table.slots[2]={&b,7};assert(original_handle_resolver(table,a.encoded_handle)==&b);
 table.slots[2]={&b,8};assert(original_handle_resolver(table,a.encoded_handle)==nullptr);
 assert(HandleTable::encode(2,7)==((7u<<12)|2u));
}
static void test_successful_flying_attachment_and_vector_provenance() {
 Owner grunt; initialize_original_jetpack_fields(grunt);
 grunt.encoded_handle=HandleTable::encode(1,12);
 grunt.position_36c={11.0f,22.0f,33.0f};
 FlyingEntity flight;flight.encoded_handle=HandleTable::encode(3,9);
 flight.vector_c18={0.0f,30.0f,40.0f};
 HandleTable table;table.slots.resize(4);table.slots[3]={&flight,9};
 MockHost host;host.found=&flight;
 auto res=acquire_original_flying_entity(grunt,table,host);
 assert(res.status==Status::Attached && res.final_callback_result==67);
 assert(grunt.flying_1024==flight.encoded_handle);
 assert(flight.owner_handle_c50==grunt.encoded_handle);
 assert(flight.origin_c24.x==11.0f&&flight.origin_c24.y==22.0f&&flight.origin_c24.z==33.0f);
 assert(flight.vector_c18.y==30.0f&&flight.vector_c18.z==40.0f); // not mutated
 assert(host.setup_entity==&flight);
 assert((host.called==std::vector<std::string>{"find:ai_base_physics_flying","setup:0,0,0","link","finalize:+0x64"}));
}
static void test_missing_original_attachment() {
 Owner o;initialize_original_jetpack_fields(o);o.encoded_handle=HandleTable::encode(0,1);
 HandleTable table;MockHost host;
 auto res=acquire_original_flying_entity(o,table,host);
 assert(res.status==Status::MissingOriginalFlyingEntity);
 assert(o.flying_1024==kInvalid&&host.setup_entity==nullptr);
 assert((host.called==std::vector<std::string>{"find:ai_base_physics_flying","setup:0,0,0"}));
}
static void test_stale_attachment() {
 Owner o; o.encoded_handle=HandleTable::encode(1,2);
 FlyingEntity f;f.encoded_handle=HandleTable::encode(0,7);
 HandleTable t;t.slots={{&f,8}};MockHost host;host.found=&f;
 auto res=acquire_original_flying_entity(o,t,host);
 assert(res.status==Status::UnresolvedNativeHandle);
 assert(o.flying_1024==f.encoded_handle&&host.setup_entity==nullptr);
 assert(f.owner_handle_c50==kInvalid);
 assert(host.called.size()==2);
}
static void test_missing_owner_guard() {
 Owner o;initialize_original_jetpack_fields(o);
 FlyingEntity f;f.encoded_handle=HandleTable::encode(0,6);
 HandleTable t;t.slots={{&f,6}};MockHost host;host.found=&f;
 auto res=acquire_original_flying_entity(o,t,host);
 assert(res.status==Status::MissingOwnerHandle);
 assert(host.setup_entity==&f);
 assert(host.called.size()==2);
}
int main(){
 test_native_initial_state();test_native_slot_and_serial();
 test_successful_flying_attachment_and_vector_provenance();
 test_missing_original_attachment();test_stale_attachment();test_missing_owner_guard();
}