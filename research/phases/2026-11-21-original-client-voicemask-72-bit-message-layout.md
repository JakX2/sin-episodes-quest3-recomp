# Phase 554: original `VoiceMask` message's 32 + 32 + 8-bit payload

## Identity and original-function accounting

This research reads the same unchanged private 2006 SiN Episodes: Emergence client DLL as the prior CVoiceStatus phases, with exact whole-file SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d` and original companion server SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. The original client PE32 image base is `0x24000000`.

Phase552 had already authenticated and counted the original **98-byte** `CVoiceStatus` message callback handler at `0x241E52B0`, and the separate **21-byte** callback relay at `0x241E5370`. This phase traces the native bit-reading *callees*, proves the payload width and field ordering, and **does not count those earlier methods again**.

### Four newly authenticated whole methods (232 code bytes)

| Original VA | Bytes | Full original method SHA256 | Interpretation |
| --- | ---: | --- | --- |
| `0x240A4760` | 118 | `1e35e98014bd1194e61f7304aea1064290c9e380acc4945687c0c763745450c2` | Unsigned bit-range reader over native 32-bit words |
| `0x240A5080` | 98 | `5d3803e84d3a3f68627ba0cb9a57cfcb4421840e76baee1bc33a4a967e1a9e1a` | Signed field reader: reads `width - 1` bits plus sign bit, sign-extends |
| `0x240A55F0` | 8 | `a3195220e8ee7a63daeaa179d3081e3f75e76b5e964cc9ce33090534ebadbc2c` | Literal 8-bit unsigned wrapper for above reader |
| `0x240A5620` | 8 | `888dfba4d3941fcd6d2922391ae091bdde162a21986da7d6d4a71b6e4da05a68` | Literal 32-bit signed wrapper for above reader |

All four methods occupy disjoint original `.text` ranges and end with original `INT3` padding. The auditor verifies **six original E8 rel32 CALL edges**, including three from the previously verified 98-byte handler, plus **41 exact opcode/field witnesses** and **four original message/debug strings**. Both original DLL whole-file SHA-256 hashes must match. The methods and their direct callers total only **232 newly verified native bytes** this phase.

## Native registered callback and wire order

The original `CVoiceStatus` initialization at `0x241E5C60` registers a client message named **`VoiceMask`** with the original handler `0x241E5370`. That relay reads active instance global `0x2437865C` and invokes complete message handler `0x241E52B0` with the message reader.

The complete original handler's relevant sequence is:

1. At `0x241E52BA`, call native wrapper `0x240A5620` with its hard-coded **`0x20`** bit count. Store the returned 32-bit word at `CVoiceStatus` **`this+0x14`** at `0x241E52C1`.
2. At `0x241E52C4`, call the **same** native 32-bit wrapper. Store the returned 32-bit word at **`this+0x1C`** at `0x241E52C9`.
3. Optionally print the original diagnostic strings for the two masks, guarded by original logging state. The string literals are **`m_AudiblePlayers[%d] = %lu`** and **`m_ServerBannedPlayers[%d] = %lu`**, and the handler provides the index argument 0 in this build.
4. At `0x241E5305`, call unsigned 8-bit wrapper `0x240A55F0`. Store the result as a 32-bit value at **`this+0x08`** at `0x241E530A`.
5. Return via `ret 4`. There is no additional read visible in the **complete authenticated 98-byte body**.

This means the original handler **consumes exactly 72 bits** from its existing bit-reader cursor, with no local byte-alignment operation between fields. The third field's precise higher-level name is not proven from this function alone: although it influences later voice-status decisions, it must not be labelled as a specific enabled/muted/connection flag without additional source evidence.

| Wire order from cursor | Read type and width | Native destination | Native diagnostic |
| ---: | --- | --- | --- |
| First | Signed-bit read, 32 bits | `CVoiceStatus+0x14` | `m_AudiblePlayers[0]` |
| Second | Signed-bit read, 32 bits | `CVoiceStatus+0x1C` | `m_ServerBannedPlayers[0]` |
| Third | Unsigned-bit read, 8 bits | `CVoiceStatus+0x08`, stored as a dword | No directly corresponding diagnostic |

The first two calls enter `0x240A5080`, which reads **31 unsigned bits** and then one sign bit. At width 32, the result preserves the full original **32-bit pattern** despite its signed extension path, including values with bit 31 set. It is therefore appropriate to store these results in unsigned `uint32_t` fields in a behavioral reference. The 8-bit wrapper enters the simpler unsigned bit reader directly.

## Underlying native reader semantics

The complete `0x240A4760` method reads a requested number of bits from a bit-reader object with the following directly observed fields:

| Offset | Original use |
| --- | --- |
| `+0x00` | Pointer to DWORD-oriented bit storage |
| `+0x08` | Total available bits / native logical limit |
| `+0x0C` | Current bit cursor |
| `+0x10` | Overflow flag byte |

The method computes the ending cursor, compares with the total bit limit, and on overflow clamps the cursor, sets the overflow flag and returns zero. Otherwise it reads an initial 32-bit little-endian storage word shifted right by cursor modulo 32. It uses an original width-to-mask lookup table at `0x2432E228` and, when a request spans a 32-bit boundary, reads and merges additional bits from the next word. It advances the cursor by the requested bit width.

The signed wrapper at `0x240A5080` delegates for `width-1` bits and consumes the sign bit separately, extending into the returned word. The native client does **not** explicitly check the reader overflow byte after the three reads in `CVoiceStatus::HandleVoiceMaskMsg`. If a message is truncated, original helper return values may be zero while earlier field state has already changed. The host-side model uses a strict all-or-nothing decode, which is deliberately safer rather than a claim of exact malformed-packet parity.

### Source-bounded C-like transcription

```cpp
// 'reader' is already positioned at the VoiceMask body by the owner engine.
// Original Phase552 method body is not included in new byte accounting.
void HandleVoiceMaskMsg_like(NativeBitReader& reader) {
    this_audible_player_mask_at_0x14 = reader.read_signed_bits(32);
    this_server_banned_mask_at_0x1C  = reader.read_signed_bits(32);
    // Original optional debug prints, including index 0.
    this_opaque_status_dword_at_0x08 = reader.read_unsigned_bits(8);
}
```

**Do not assert the entire network message is nine bytes.** The directly verified fact is that the handler consumes **72 bits = nine payload bytes at an aligned starting bit cursor**. The containing message name, framing, offset and possible trailing fields outside the handler may be controlled by the original engine's message transport, and have not been reconstructed.

## Tested independent reference and remaining work

The new `include/sin_compat/original_voice_mask_wire.hpp` contains an original-free C++17 LSB-first reference bit reader and a strict decoder for the three field values. `tests/test_original_voice_mask_wire.cpp` verifies both byte-aligned and bit-offset-three samples, values with the high bit set, the exact native destination offsets and rejection of truncation. Extra overflow checks and all-or-nothing parsing are **modern host-side guards** absent from the native message handler.

To verify on an owner-supplied original ZIP, execute:

```sh
python3 research/tools/audit_sin_original_voice_mask_wire.py --self-test
python3 research/tools/audit_sin_original_voice_mask_wire.py --bin-zip /path/to/private/bin.zip
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Local owner audit returned `ORIGINAL_VOICE_MASK_WIRE_AUTHENTICATED`: **four complete new functions, 232 unique code bytes, six native call edges, 41 opcode witnesses and four source-native message strings**. Strict GCC and Clang C++17 tests and 2/2 local CTest passed. The original handler's 98 bytes and callback relay's 21 bytes remain attributed to Phase552 and are **not counted twice**.

**Unresolved:** actual engine net-message routing/framing, message creation on the server, server-side `VoiceMask` send payload, the exact semantic name for `this+0x08`, other mask blocks for players 33+, potential network endian differences in external adapters, real-game playback. This phase decompiles original client logic, not a standalone client DLL or Quest port.