#include "sin_compat/original_client_voice_status.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
using namespace sin_compat::original_client_voice_status;
int main() {
    static_assert(kNativeMaxPlayersInUpdate == 32);
    static_assert(kNativeUpdateServerState == 0x241E5880u);
    assert(kModEnableFormat == "VModEnable %d");
    assert(kVoiceBanPrefix == "vban");
    const std::uint32_t words[]{0x80000001u, 0x00000002u};
    assert(bounded_original_player_mask_bit(words,2,1));
    assert(!bounded_original_player_mask_bit(words,2,2));
    assert(bounded_original_player_mask_bit(words,2,32));
    assert(!bounded_original_player_mask_bit(words,2,33));
    assert(bounded_original_player_mask_bit(words,2,34));
    assert(!bounded_original_player_mask_bit(words,2,64));
    assert(!bounded_original_player_mask_bit(words,2,65));
    assert(!bounded_original_player_mask_bit(words,2,0));
    assert(!bounded_original_player_mask_bit(words,2,-1));
    assert(!bounded_original_player_mask_bit(nullptr,0,1));
    assert(!should_send_mod_enable(false,true,false,true));
    assert(!should_send_mod_enable(true,false,false,false));
    assert(should_send_mod_enable(true,false,false,true));
    assert(should_send_mod_enable(true,false,true,false));
    assert(should_send_mod_enable(true,true,true,true));
    return 0;
}