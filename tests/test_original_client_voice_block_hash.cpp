#include "sin_compat/original_client_voice_block_hash.hpp"
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
using namespace sin_compat::original_client_voice_block_hash;
struct Host : IVoiceHost {
  Key32 identity{};
  bool player_exists = true;
  int get_count = 0;
  int refresh_count = 0;
  std::vector<std::string> events{};
  bool get_identity(int player, Key32& out) override {
    assert(player>=1 && player<=32);
    ++get_count; events.push_back("get");
    out = identity;
    return player_exists;
  }
  void update_server_state(bool force) override {
    assert(!force); ++refresh_count; events.push_back("refresh");
  }
};
int main() {
  static_assert(kBucketCount==256);
  static_assert(kNativeBucketCollectionBytes==0x2800);
  Key32 a{},b{},c{};
  a[0]=1;
  b[1]=1;  // same byte-sum hash, different 32-byte key
  c[31]=129; c[0]=128; // modulo-256 wrap -> 1
  assert(bucket_for(a)==1 && bucket_for(b)==1 && bucket_for(c)==1);
  KeyMembership table;
  assert(!table.contains(a));
  assert(!table.set_present(a,false));
  assert(table.set_present(a,true));
  assert(!table.set_present(a,true));
  assert(table.set_present(b,true) && table.set_present(c,true));
  assert(table.size()==3 && table.contains(a) && table.contains(b) && table.contains(c));
  assert(table.set_present(a,false));
  assert(!table.contains(a) && table.contains(b) && table.contains(c));
  table.clear(); assert(table.size()==0);
  Host host; host.identity=a;
  // Native calls player-info getter before +0x14 bit gate. Source guards invalid.
  assert(toggle_key_if_masked(table,host,0,true,0xffffffffu)==ToggleOutcome::InvalidPlayer);
  assert(host.get_count==0 && host.refresh_count==0);
  assert(toggle_key_if_masked(table,host,1,true,0)==ToggleOutcome::MaskNotSet);
  assert(host.get_count==1 && host.refresh_count==0);
  assert(toggle_key_if_masked(table,host,1,false,1)==ToggleOutcome::Added);
  assert(table.contains(a) && host.get_count==2 && host.refresh_count==1);
  // Original's second stack argument is not consulted: this method toggles.
  assert(toggle_key_if_masked(table,host,1,false,1)==ToggleOutcome::Removed);
  assert(!table.contains(a) && host.refresh_count==2);
  assert(host.events == (std::vector<std::string>{"get","get","refresh","get","refresh"}));
  // Last valid 1-based index, highest bit in the native first dword.
  assert(toggle_key_if_masked(table,host,32,true,0x80000000u)==ToggleOutcome::Added);
  assert(table.contains(a) && host.refresh_count==3);
  host.player_exists=false;
  assert(toggle_key_if_masked(table,host,32,true,0x80000000u)==ToggleOutcome::PlayerInfoUnavailable);
  assert(host.refresh_count==3 && table.size()==1);
  assert(toggle_key_if_masked(table,host,33,true,0xffffffffu)==ToggleOutcome::InvalidPlayer);
}