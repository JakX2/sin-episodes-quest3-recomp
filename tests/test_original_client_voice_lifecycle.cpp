#include "sin_compat/original_client_voice_lifecycle.hpp"
#include <cassert>
#include <cmath>
#include <string>
#include <vector>
using namespace sin_compat::original_client_voice_lifecycle;
struct Host : IVoiceEvents {
  std::vector<std::string> events{};
  void seed_voice_key_table() override {events.emplace_back("seed");}
  void acquire_vgui_texture(std::string_view t) override {
    assert(t=="voice/icntlk_pl");events.emplace_back("texture");
  }
  void publish_active_instance() override {events.emplace_back("active");}
  void initialize_vgui_texture() override {events.emplace_back("virtual30");}
  void register_message(std::string_view t) override {events.emplace_back(t);}
  void update_server_state(bool force) override {
    assert(force);events.emplace_back("force");
  }
};
int main() {
  ConstructedFields x;
  assert(x.this_plus_0x04==0);
  assert(x.this_plus_0x08==0xffffffffU);
  assert(x.this_plus_0x0c==0 && x.this_plus_0x14==0);
  assert(x.this_plus_0x2828==0 && !x.this_plus_0x282c);
  assert(kNativeHashTableOffset+kNativeHashTableBytes==kNativeTextureHandleOffset);
  assert(!should_unforced_refresh(10.75f,10.0f));
  assert(should_unforced_refresh(11.0f,10.0f));
  assert(should_unforced_refresh(12.0f,10.0f));
  assert(!should_unforced_refresh(9.0f,10.0f));
  assert(!should_unforced_refresh(std::nanf(""),10.0f));
  Host h;
  describe_original_init_sequence(h,true);
  assert(h.events==(std::vector<std::string>{"seed","texture","active","virtual30","VoiceMask","RequestState"}));
  registered_request_state(h,false);
  assert(h.events.size()==6);
  registered_request_state(h,true);
  assert(h.events.back()=="force");
  Host h2;
  describe_original_init_sequence(h2,false);
  assert(h2.events==(std::vector<std::string>{"texture","active","virtual30","VoiceMask","RequestState"}));
  assert(choose_vgui_dispatch(true,true)==VguiDispatch::AlreadyHandled);
  assert(choose_vgui_dispatch(true,false)==VguiDispatch::AlreadyHandled);
  assert(choose_vgui_dispatch(false,true)==VguiDispatch::Virtual0x30);
  assert(choose_vgui_dispatch(false,false)==VguiDispatch::Virtual0x34);
}