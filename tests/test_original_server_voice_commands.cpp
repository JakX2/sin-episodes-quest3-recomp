#include "sin_compat/original_server_voice_commands.hpp"
#include <cassert>
#include <cstdint>
#include <vector>

using namespace sin_compat::original_server_voice_commands;

int main() {
    assert(classify_ascii_fallback("VBAN") == CommandKind::vban);
    assert(classify_ascii_fallback("vBaN") == CommandKind::vban);
    assert(classify_ascii_fallback("vmOdEnAbLe") == CommandKind::VModEnable);
    assert(classify_ascii_fallback("unknown") == CommandKind::unknown);
    assert(classify_ascii_fallback("vban_more") == CommandKind::unknown);
    assert(ascii_fold(0xD1U) == 0xD1U);
    VoiceManagerState s{40}; // 2 bit words, distinct recipient cache array
    assert(s.voice_enabled_bits.size() == 2);
    assert(s.request_pending_bits.size() == 2);

    // Real CVoiceGameMgr ClientCommand owner indexes are 0-based internally.
    assert(apply_parsed_command(s, 0, CommandKind::VModEnable,{1}));
    assert(test_bit(s.voice_enabled_bits,0));
    assert(!test_bit(s.voice_enabled_bits,1));

    // Native 0x22293AC0 receives *one-based* player number, invalidates cache.
    s.cached_audible[0] = 0x12345678U;
    s.cached_banned[0] = 0xABCDEF00U;
    assert(mark_refresh(s,1));
    assert(test_bit(s.request_pending_bits,0));
    assert(s.cached_audible[0] == 0U && s.cached_banned[0] == 0U);
    assert(apply_parsed_command(s, 0,CommandKind::VModEnable,{0}));
    assert(!test_bit(s.voice_enabled_bits,0));
    assert(!test_bit(s.request_pending_bits,0));

    // Original vban parses 32-bit hex, applies *first* argument only.
    assert(apply_parsed_command(s, 0,CommandKind::vban,{0xDEADBEEFU,0x4321U}));
    assert(s.server_banned_masks[0] == 0xDEADBEEFU);
    assert(!test_bit(s.request_pending_bits,0)); // vban doesn't clear it
    assert(!apply_parsed_command(s, 0,CommandKind::vban,{}));
    assert(!apply_parsed_command(s, 0,CommandKind::VModEnable,{}));
    assert(!apply_parsed_command(s, 0,CommandKind::unknown,{1}));
    assert(s.server_banned_masks[0] == 0xDEADBEEFU);

    // Original invalid-recipient branch returns true, without modifying masks.
    assert(apply_parsed_command(s,-1,CommandKind::VModEnable,{1}));
    assert(apply_parsed_command(s,40,CommandKind::VModEnable,{1}));
    assert(!test_bit(s.voice_enabled_bits,0));
    assert(!mark_refresh(s,0)); // host-side guard, not a retail code claim
    assert(!mark_refresh(s,41));

    // Test 0-based index 31 / 32 crossing into a second original DWORD.
    assert(apply_parsed_command(s,31,CommandKind::VModEnable,{1}));
    assert(apply_parsed_command(s,32,CommandKind::VModEnable,{0x80000000U}));
    assert(test_bit(s.voice_enabled_bits,31));
    assert(test_bit(s.voice_enabled_bits,32));
    assert(!test_bit(s.voice_enabled_bits,33));
    s.cached_audible[32] = 0x12U;
    s.cached_banned[32] = 0x34U;
    assert(mark_refresh(s,33));
    assert(test_bit(s.request_pending_bits,32));
    assert(s.cached_audible[32] == 0 && s.cached_banned[32] == 0);
    assert(apply_parsed_command(s,32,CommandKind::VModEnable,{0}));
    assert(!test_bit(s.request_pending_bits,32));
    assert(!test_bit(s.voice_enabled_bits,32));
    assert(test_bit(s.voice_enabled_bits,31));

    // Native original bit helper +0x00 base and +0x04 bit index, unbounded;
    // reference rejects out-of-array operations for memory safety.
    const auto before=s.voice_enabled_bits;
    assign_bit(s.voice_enabled_bits,2048,true);
    assert(s.voice_enabled_bits == before);
    assert(!test_bit(s.voice_enabled_bits,2048));
    return 0;
}