#include "sin_compat/original_client_appsystem_phase659.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

namespace n=sin_compat::original_client_appsystem_phase659;
namespace {
constexpr std::uint32_t kFirst=0xa000u,kSecond=0xa100u,kThird=0xa200u;
constexpr std::uint32_t kFirstSlot=0xb000u,kSecondSlot=0xb100u,kThirdSlot=0xb200u;
constexpr std::uint32_t kGetFirst=0xc000u,kGetSecond=0xc100u,kGetThird=0xc200u;
constexpr std::uint32_t kFreeFirst=0xd000u,kFreeSecond=0xd100u,kFreeThird=0xd200u;
struct Host final:n::IHost {
 std::map<std::uint32_t,std::uint32_t> words;
 std::map<std::uint32_t,std::uint32_t> resolver_returns;
 std::vector<std::uint32_t> write_order;
 std::vector<std::uint32_t> resolved;
 std::vector<std::pair<std::uint32_t,std::uint32_t>> released;
 bool reject_write=false,reject_resolver=false,reject_release=false;
 bool throw_resolver=false,reenter=false;
 std::uint32_t reject_write_va=0u;
 n::AppSystemChain* model=nullptr;
 n::Code nested=n::Code::Completed;
 Host(){words[n::kHeadVA]=0u;}
 bool read_u32(std::uint32_t a,std::uint32_t& out)override {
  const auto i=words.find(a);if(i==words.end())return false;
  out=i->second;return true;
 }
 bool write_u32(std::uint32_t a,std::uint32_t value)override{
  if(reject_write||a==reject_write_va)return false;
  words[a]=value;write_order.push_back(a);return true;
 }
 bool resolve_original_callback(std::uint32_t callback,std::uint32_t& object)override{
  resolved.push_back(callback);
  if(throw_resolver)throw std::runtime_error("host callback");
  if(reenter&&model){reenter=false;nested=model->connect_all().code;}
  if(reject_resolver)return false;
  auto i=resolver_returns.find(callback);
  object=i==resolver_returns.end()?0u:i->second;
  return true;
 }
 bool release_original_callback(std::uint32_t callback,std::uint32_t object)override{
  released.emplace_back(callback,object);
  return !reject_release;
 }
 void register_three(n::AppSystemChain& chain){
  const auto r1=chain.register_entry(kFirst,kFirstSlot,kGetFirst,kFreeFirst);
  const auto r2=chain.register_entry(kSecond,kSecondSlot,kGetSecond,kFreeSecond);
  const auto r3=chain.register_entry(kThird,kThirdSlot,kGetThird,kFreeThird);
  assert(r1.code==n::Code::Completed&&r1.writes_committed==5u);
  assert(r2.code==n::Code::Completed&&r3.code==n::Code::Completed);
  assert(words.at(n::kHeadVA)==kThird);
  assert(words.at(kThird+12u)==kSecond&&words.at(kSecond+12u)==kFirst);
  assert(words.at(kFirst+12u)==0u);
  assert(words.at(kFirst)==kFirstSlot&&words.at(kFirst+4u)==kGetFirst);
  assert(words.at(kFirst+8u)==kFreeFirst);
  assert(words.at(kSecond)==kSecondSlot&&words.at(kThird)==kThirdSlot);
  assert(write_order.size()==15u);
  assert(write_order[0]==kFirst&&write_order[1]==kFirst+4u);
  assert(write_order[2]==kFirst+8u&&write_order[3]==kFirst+12u);
  assert(write_order[4]==n::kHeadVA);
 }
};
void success_and_disconnect(){
 Host h;n::AppSystemChain chain(h);
 h.register_three(chain);
 h.words[kFirstSlot]=h.words[kSecondSlot]=h.words[kThirdSlot]=0u;
 h.resolver_returns[kGetFirst]=0xe000u;
 h.resolver_returns[kGetSecond]=0xe100u;
 h.resolver_returns[kGetThird]=0xe200u;
 const auto r=chain.connect_all();
 assert(r.code==n::Code::Completed);
 assert(r.entries_visited==3u&&r.resolver_calls==3u);
 assert(r.writes_committed==3u&&!r.cleanup_invoked);
 assert((h.resolved==std::vector<std::uint32_t>{kGetThird,kGetSecond,kGetFirst}));
 assert(h.words.at(kFirstSlot)==0xe000u);
 assert(h.words.at(kSecondSlot)==0xe100u);
 assert(h.words.at(kThirdSlot)==0xe200u);
 const auto cleanup=chain.disconnect_all();
 assert(cleanup.code==n::Code::Completed);
 assert(cleanup.entries_visited==3u&&cleanup.release_calls==3u);
 assert(cleanup.writes_committed==3u);
 assert((h.released==std::vector<std::pair<std::uint32_t,std::uint32_t>>{
  {kFreeThird,0xe200u},{kFreeSecond,0xe100u},{kFreeFirst,0xe000u}}));
 assert(h.words.at(kFirstSlot)==0u&&h.words.at(kSecondSlot)==0u);
 assert(h.words.at(kThirdSlot)==0u);
 assert(h.words.at(n::kHeadVA)==kThird); // original retains registration list.
}
void partial_resolution_triggers_full_cleanup(){
 Host h;n::AppSystemChain chain(h);h.register_three(chain);
 h.words[kFirstSlot]=0xf100u; // even untouched entries are disconnected.
 h.words[kSecondSlot]=0u;
 h.words[kThirdSlot]=0u;
 h.resolver_returns[kGetThird]=0xf300u;
 h.resolver_returns[kGetSecond]=0u; // required entry failed.
 h.resolver_returns[kGetFirst]=0xf100u;
 const auto r=chain.connect_all();
 assert(r.code==n::Code::MissingRequiredAppSystem);
 assert(r.entries_visited==2u&&r.resolver_calls==2u);
 assert(r.cleanup_invoked&&r.release_calls==3u);
 assert(r.writes_committed==4u); // one connect write, three cleanup zeros.
 assert(h.words.at(kFirstSlot)==0u&&h.words.at(kSecondSlot)==0u);
 assert(h.words.at(kThirdSlot)==0u);
 assert((h.resolved==std::vector<std::uint32_t>{kGetThird,kGetSecond}));
 assert((h.released==std::vector<std::pair<std::uint32_t,std::uint32_t>>{
   {kFreeThird,0xf300u},{kFreeSecond,0u},{kFreeFirst,0xf100u}}));
}
void optional_undestined_entry(){
 Host h;n::AppSystemChain chain(h);
 assert(chain.register_entry(kFirst,0u,kGetFirst,kFreeFirst).code==n::Code::Completed);
 assert(chain.register_entry(kSecond,kSecondSlot,kGetSecond,kFreeSecond).code==n::Code::Completed);
 h.words[kSecondSlot]=0u;h.resolver_returns[kGetFirst]=0u;
 h.resolver_returns[kGetSecond]=0xcafeu;
 const auto r=chain.connect_all();
 assert(r.code==n::Code::Completed&&r.resolver_calls==2u);
 assert(h.words.at(kSecondSlot)==0xcafeu);
 const auto disconnect=chain.disconnect_all();
 assert(disconnect.code==n::Code::Completed&&disconnect.release_calls==1u);
 assert((h.released==std::vector<std::pair<std::uint32_t,std::uint32_t>>{
   {kFreeSecond,0xcafeu}}));
 h.resolver_returns[kGetFirst]=0x1234u;
 const auto bad=chain.connect_all();
 assert(bad.code==n::Code::NullDestinationHazard);
}
void partial_failure_and_reentry(){
 Host h;n::AppSystemChain chain(h);h.register_three(chain);
 h.words[kFirstSlot]=h.words[kSecondSlot]=h.words[kThirdSlot]=0u;
 h.resolver_returns[kGetThird]=0xe000u;
 h.resolver_returns[kGetSecond]=0xe100u;
 h.resolver_returns[kGetFirst]=0xe200u;
 h.reenter=true;h.model=&chain;
 const auto r=chain.connect_all();
 assert(r.code==n::Code::Completed&&h.nested==n::Code::Reentrant);
 Host bad; n::AppSystemChain failing(bad);bad.register_three(failing);
 bad.words[kFirstSlot]=bad.words[kSecondSlot]=bad.words[kThirdSlot]=0u;
 bad.throw_resolver=true;
 assert(failing.connect_all().code==n::Code::HostRejected);
 bad.throw_resolver=false;bad.reject_resolver=true;
 assert(failing.connect_all().code==n::Code::HostRejected);
 bad.reject_resolver=false;bad.resolver_returns[kGetThird]=0xee00u;
 bad.resolver_returns[kGetSecond]=0u;
 bad.reject_release=true;
 const auto partially=failing.connect_all();
 assert(partially.code==n::Code::HostRejected&&partially.cleanup_invoked);
 assert(partially.release_calls==1u);
 assert(bad.words[kThirdSlot]==0xee00u); // failed release: no clearing after callback.
 bad.reject_release=false;
 assert(failing.disconnect_all().code==n::Code::Completed);
 assert(bad.words[kThirdSlot]==0u);
}
void invalid_entry_and_cycle(){
 Host h;n::AppSystemChain chain(h);
 assert(chain.register_entry(0u,1u,1u,1u).code==n::Code::InvalidArgument);
 assert(chain.register_entry(kFirst,kFirstSlot,0u,1u).code==n::Code::InvalidArgument);
 assert(h.words.at(n::kHeadVA)==0u);
 assert(chain.connect_all().code==n::Code::Completed);
 assert(chain.disconnect_all().code==n::Code::Completed);
 assert(chain.register_entry(kFirst,0u,kGetFirst,kFreeFirst).code==n::Code::Completed);
 // Malformed cyclic guest memory should be refused by portable host guard;
 // native x86 has no such guard and would loop indefinitely.
 h.words[kFirst+12u]=kFirst;h.resolver_returns[kGetFirst]=0u;
 const auto r=chain.connect_all();
 assert(r.code==n::Code::CycleGuard&&r.resolver_calls==65536u);
}
}
int main(){
 success_and_disconnect();partial_resolution_triggers_full_cleanup();
 optional_undestined_entry();partial_failure_and_reentry();
 invalid_entry_and_cycle();
}
