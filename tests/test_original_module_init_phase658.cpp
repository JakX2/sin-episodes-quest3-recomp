#include "sin_compat/original_module_init_phase658.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace n = sin_compat::original_module_init_phase658;
namespace {
constexpr std::uint32_t kEngineFactory=0x11110000u;
constexpr std::uint32_t kFileFactory=0x22220000u;
constexpr std::uint32_t kGlobalVars=0x33330000u;
struct Host final : n::IHost {
  std::map<std::uint32_t,std::uint32_t> words;
  std::vector<std::tuple<std::string,n::Factory,std::uint32_t>> calls;
  std::vector<std::uint32_t> written;
  n::AcquirePrefix* reentrant_model=nullptr;
  n::Code nested_result=n::Code::PrefixAcquired;
  bool reenter=false,throw_on_query=false,fail_sound=false,fail_hardware=false;
  bool fail_query_callback=false,fail_write=false,optional_stub_null=false;
  std::uint32_t reject_write_address=0u;
  std::size_t null_query_index=static_cast<std::size_t>(-1);
  std::size_t reject_query_index=static_cast<std::size_t>(-1);
  unsigned sound_calls=0u,hardware_calls=0u;
  std::uint32_t sound_object=0u,sound_factory=0u,hardware_material=0u;
  std::string hardware_name;

  bool factory_call(n::Factory provenance,std::uint32_t factory,
                    const char* name,std::uint32_t& output)override {
    if(throw_on_query)throw std::runtime_error("original engine factory throws");
    if(reenter&&reentrant_model){
      reenter=false;
      nested_result=reentrant_model->run(n::Role::Server,
             kEngineFactory,kFileFactory,kGlobalVars).code;
    }
    const std::size_t index=calls.size();
    calls.emplace_back(name,provenance,factory);
    if(fail_query_callback||index==reject_query_index)return false;
    output=static_cast<std::uint32_t>(0x40000000u+index*0x100u);
    if(index==null_query_index ||
       (optional_stub_null&&std::string(name)=="VMaterialSystemStub001"))
      output=0u;
    return true;
  }
  bool write_u32(std::uint32_t va,std::uint32_t value)override {
    if(fail_write||va==reject_write_address)return false;
    words[va]=value;written.push_back(va);
    return true;
  }
  bool sound_emitter_slot_zero(std::uint32_t emitter,
       std::uint32_t engine_factory,bool& al_success)override {
    ++sound_calls;sound_object=emitter;sound_factory=engine_factory;
    al_success=!fail_sound;
    return true;
  }
  bool material_hardware_slot_eight(std::uint32_t material,const char* name,
       std::uint32_t& result)override {
    ++hardware_calls;hardware_material=material;hardware_name=name;
    result=fail_hardware?0u:0x77770000u;
    return true;
  }
};
void assert_manifest(){
  const auto server=n::original_manifest(n::Role::Server);
  const auto client=n::original_manifest(n::Role::Client);
  assert(server.count==16u&&client.count==21u);
  assert(std::string(server.queries[0].name)=="VEngineServer021");
  assert(std::string(client.queries[0].name)=="VEngineClient012");
  assert(server.queries[10].factory==n::Factory::FileSystem);
  assert(std::string(server.queries[10].name)=="VFileSystem012");
  assert(client.queries[16].factory==n::Factory::Engine);
  assert(std::string(client.queries[16].name)=="VFileSystem012");
  assert(server.queries[15].target_global_va==0x225721d4u);
  assert(client.queries[20].target_global_va==0x2433f6e4u);
  for(auto role:{n::Role::Server,n::Role::Client}) {
    const auto m=n::original_manifest(role);
    for(std::size_t i=0;i<m.count;++i) {
      const auto& q=m.queries[i];
      assert(q.name!=nullptr&&q.original_site_va!=0u);
      assert(q.original_name_va!=0u&&q.target_global_va!=0u);
      if(i>0u)assert(q.original_site_va>m.queries[i-1u].original_site_va);
      for(std::size_t j=0u;j<i;++j)
        assert(q.target_global_va!=m.queries[j].target_global_va);
    }
  }
}
void test_server_success(){
  Host h;n::AcquirePrefix model(h);
  const auto result=model.run(n::Role::Server,kEngineFactory,kFileFactory,kGlobalVars);
  assert(result.code==n::Code::PrefixAcquired);
  assert(result.original_init_tail_not_executed);
  assert(result.required_queries_completed==16u&&result.writes_committed==17u);
  assert(result.emitter_connected&&h.sound_calls==1u);
  assert(h.sound_object==h.words.at(0x225721d0u));
  assert(h.sound_factory==kEngineFactory);
  assert(h.hardware_calls==0u);
  assert(h.words.at(n::kServerGlobalsVA)==kGlobalVars);
  assert(h.written.back()==n::kServerGlobalsVA);
  assert(h.calls.size()==16u);
  for(std::size_t i=0u;i<h.calls.size();++i) {
    const auto& q=n::original_manifest(n::Role::Server).queries[i];
    assert(std::get<0>(h.calls[i])==q.name);
    assert(std::get<1>(h.calls[i])==q.factory);
    assert(std::get<2>(h.calls[i])==
      (q.factory==n::Factory::FileSystem?kFileFactory:kEngineFactory));
    assert(h.words.at(q.target_global_va)==0x40000000u+i*0x100u);
  }
  assert(h.words.count(0x225721ccu)==0u); // optional debug overlay is later.
}
void test_client_success(bool optional_missing){
  Host h;h.optional_stub_null=optional_missing;
  n::AcquirePrefix model(h);
  const auto result=model.run(n::Role::Client,kEngineFactory,0u,kGlobalVars);
  assert(result.code==n::Code::PrefixAcquired);
  assert(result.original_init_tail_not_executed);
  assert(result.required_queries_completed==21u&&result.writes_committed==24u);
  assert(result.emitter_connected&&h.sound_calls==1u);
  assert(h.sound_object==h.words.at(0x2433f6e4u));
  assert(h.hardware_calls==1u);
  assert(h.hardware_material==h.words.at(0x2433f69cu));
  assert(h.hardware_name=="MaterialSystemHardwareConfig012");
  assert(result.hardware_config_object==0x77770000u);
  assert(h.words.at(n::kClientGlobalsVA)==kGlobalVars);
  assert(h.written.front()==n::kClientGlobalsVA);
  assert(h.calls.size()==22u);
  assert(std::get<0>(h.calls.back())=="VMaterialSystemStub001");
  assert(std::get<2>(h.calls.back())==kEngineFactory);
  assert(h.words.at(0x2433f6a0u)==(optional_missing?0u:0x40001500u));
  for(std::size_t i=0u;i<21u;++i){
    const auto& q=n::original_manifest(n::Role::Client).queries[i];
    assert(std::get<0>(h.calls[i])==q.name);
    assert(std::get<2>(h.calls[i])==kEngineFactory);
    assert(h.words.at(q.target_global_va)==0x40000000u+i*0x100u);
  }
}
void test_fail_at_every_required_query(n::Role role){
  const auto m=n::original_manifest(role);
  for(std::size_t i=0u;i<m.count;++i) {
    Host h;h.null_query_index=i;
    n::AcquirePrefix model(h);
    const auto r=model.run(role,kEngineFactory,kFileFactory,kGlobalVars);
    assert(r.code==n::Code::MissingRequiredInterface);
    assert(r.stage==n::Stage::StoreQueryResult);
    assert(r.required_queries_completed==i+1u);
    assert(r.failed_site_va==m.queries[i].original_site_va);
    assert(r.failed_storage_va==m.queries[i].target_global_va);
    assert(std::string(r.failed_interface)==m.queries[i].name);
    assert(h.calls.size()==i+1u);
    assert(h.words.at(m.queries[i].target_global_va)==0u);
    assert(h.sound_calls==0u);
    assert(r.writes_committed==i+1u+(role==n::Role::Client?1u:0u));
    if(role==n::Role::Server)assert(h.words.count(n::kServerGlobalsVA)==0u);
    else assert(h.words.at(n::kClientGlobalsVA)==kGlobalVars);
  }
}
void test_post_query_failure_gates(){
  Host a;a.fail_sound=true;n::AcquirePrefix server(a);
  const auto blocked=server.run(n::Role::Server,kEngineFactory,kFileFactory,kGlobalVars);
  assert(blocked.code==n::Code::MissingSoundEmitter);
  assert(blocked.required_queries_completed==16u);
  assert(blocked.writes_committed==16u&&!blocked.emitter_connected);
  assert(a.words.count(n::kServerGlobalsVA)==0u);
  Host b;b.fail_sound=true;n::AcquirePrefix client(b);
  const auto c=client.run(n::Role::Client,kEngineFactory,0u,kGlobalVars);
  assert(c.code==n::Code::MissingSoundEmitter);
  assert(c.required_queries_completed==21u);
  assert(c.writes_committed==22u&&!c.emitter_connected);
  assert(b.calls.size()==21u&&b.hardware_calls==0u);
  Host d;d.fail_hardware=true;n::AcquirePrefix config(d);
  const auto result=config.run(n::Role::Client,kEngineFactory,0u,kGlobalVars);
  assert(result.code==n::Code::MissingHardwareConfig);
  assert(result.emitter_connected);
  assert(result.writes_committed==24u&&result.hardware_config_object==0u);
  assert(d.words.at(0x2433f6a4u)==0u);
}
void test_partial_state_and_host_faults(){
  Host h;n::AcquirePrefix model(h);
  assert(model.run(n::Role::Server,0u,kFileFactory,kGlobalVars).code==
    n::Code::InvalidArgument);
  assert(model.run(n::Role::Server,kEngineFactory,0u,kGlobalVars).code==
    n::Code::InvalidArgument);
  assert(model.run(n::Role::Client,kEngineFactory,0u,0u).code==
    n::Code::InvalidArgument);
  assert(h.words.empty()&&h.calls.empty());
  h.reject_write_address=0x225721a4u;
  const auto refused=model.run(n::Role::Server,kEngineFactory,kFileFactory,kGlobalVars);
  assert(refused.code==n::Code::HostRejected);
  assert(refused.stage==n::Stage::StoreQueryResult);
  assert(refused.required_queries_completed==10u);
  assert(h.calls.size()==11u);
  assert(h.words.count(0x225721a4u)==0u);
  h.reject_write_address=0u;
  h.reenter=true;h.reentrant_model=&model;
  assert(model.run(n::Role::Server,kEngineFactory,kFileFactory,kGlobalVars).code==
    n::Code::PrefixAcquired);
  assert(h.nested_result==n::Code::Reentrant);
  Host broken;broken.throw_on_query=true;n::AcquirePrefix rejecting(broken);
  assert(rejecting.run(n::Role::Client,kEngineFactory,0u,kGlobalVars).code==
    n::Code::HostRejected);
  assert(broken.words.at(n::kClientGlobalsVA)==kGlobalVars);
  Host missing;missing.reject_query_index=2u;n::AcquirePrefix rejection(missing);
  const auto r=rejection.run(n::Role::Server,kEngineFactory,kFileFactory,kGlobalVars);
  assert(r.code==n::Code::HostRejected&&r.required_queries_completed==2u);
  assert(missing.words.count(0x225721a0u)==0u);
}
}
int main() {
  assert_manifest();
  test_server_success();test_client_success(false);test_client_success(true);
  test_fail_at_every_required_query(n::Role::Server);
  test_fail_at_every_required_query(n::Role::Client);
  test_post_query_failure_gates();test_partial_state_and_host_faults();
}
