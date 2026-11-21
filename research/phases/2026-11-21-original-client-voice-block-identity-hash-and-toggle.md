# Phase 551: Original 2006 `CVoiceStatus::SetPlayerBlockedState` and 256-bucket identity-key hash

## Original provenance and constraints

The inspected source is the owner's unchanged 2006 Windows x86 SiN Episodes: Emergence DLL pair in `bin.zip`, **not** a public repository file. Original whole DLL SHA-256 digests:

- Original `client.dll`: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- Original `server.dll`: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.

The original `client.dll` image base is `0x24000000`. The original `CVoiceStatus` name is witnessed directly by three embedded diagnostic strings, including `CVoiceStatus::SetPlayerBlockedState: setting player %d ban to %d\n`. Phase 550 authenticated the callee `0x241E5880` (original `UpdateServerState`). The present phase adds **eight completely SHA-checked, mutually non-overlapping functions, 657 original `.text` bytes**, covering the caller and its private hash-table dependency chain.

**Decompilation boundary:** the source-like algorithms below are independently authored behavioral reconstructions, not recovered original C++ symbols or exact original type layouts. The original `client.dll` is not a working source replacement and is not committed.

## Complete native method manifest

| Original client VA | Exact bytes | SHA-256 of complete method | Source-backed role |
| --- | ---: | --- | --- |
| `0x241E5B20` | 213 | `af3cd7e9683f955e486e00b59b8a68b26e4d177719c0d583d36ffae2c48a5ac0` | `CVoiceStatus::SetPlayerBlockedState` log/guard/key toggle/refresh path |
| `0x241E4F00` | 19 | `21c34399b2db440e778a7789b9c56ad818f816293ceb9443b681d1be78ff057a` | 32-byte key presence Boolean wrapper |
| `0x241E4F20` | 83 | `40cef3e9c91a2e90d37c45eb9f9a8fc91df0d277060d791a2106a6a3465203c9` | Idempotent key insert/remove based on passed Boolean |
| `0x241E4DD0` | 95 | `e33a2b3f9595fc338753845bda1c5dbe2b52402f7394ad612be75a325e00846a` | Sum-hash, bucket-list traversal and exact key match |
| `0x241E4E30` | 115 | `fb2136d9b01e84421fdbfcc6c1bffb0d1ea82782b0a110cd98afac0fa918290e` | Allocate/copy a 40-byte record and append to bucket |
| `0x241E4D90` | 60 | `7f2dfba82edd5099c5c57210b01203a3e1273e5e270158d87ff169c8c40c6b48` | Initialize 256 empty 40-byte bucket sentinels |
| `0x241E4EC0` | 60 | `e2950b9199e9d10df9c9511319baec6fff7d6e9903e146b28317a792034abba7` | Free all bucket records; tail-jump back to initializer |
| `0x241E4EB0` | 12 | `1212a5e5a78c9175e875d348db77f508999f0f7858308c4c28dedb3d0fbf13c8` | Constructor calls bucket initializer and returns owner |

All eight method boundaries are followed by original `INT3` padding. The auditor verifies **12 exact direct `E8 rel32` CALL edges**, **one `E9 rel32` tail JMP**, **44 exact instruction/field witnesses**, and **3 original diagnostic strings**. The complete method digests, rather than an unreliable first-`0xCC` scan, are the authoritative boundaries.

## Native hash-table and record layout: *not* a red-black tree

The original `0x241E4D90` initializes 64 groups of **four** empty sentinels, advancing `0xA0` bytes per group. This is **256 buckets** spaced **0x28 = 40 bytes** apart; the full array is **`0x2800` bytes**. Original voice-status methods address it by the subobject expression **`this+0x28`**. This matches the next observed client object fields at `this+0x2828`, without proving their higher-level C++ type.

Each bucket sentinel and dynamically allocated node share this 32-bit record layout:

| Native relative offset | Width | Observed role |
| --- | --- | --- |
| `+0x00 .. +0x1F` | 32 bytes | Opaque **identity key** on allocated nodes |
| `+0x20` | 4 bytes | Previous bucket-list link |
| `+0x24` | 4 bytes | Next bucket-list link |

A vacant sentinel links both directions to its **own** 40-byte bucket address. The constructor `0x241E4EB0` initializes this table; `0x241E4EC0` walks each bucket's next links, frees every non-sentinel record through original `0x2416FD60`, and *tail-jumps* to `0x241E4D90` to reset all sentinels.

The original key hash at both `0x241E4DD0` and `0x241E4E30` is the **unsigned 8-bit wrapping sum of all 32 key bytes**. The assembly accumulates eight groups of four byte additions, then zero-extends the final 8-bit sum and multiplies by 40 (`index*5*8`) to identify the bucket:

```cpp
// Descriptive translation of the native arithmetic, not source-line recovery.
uint8_t bucket_for(const uint8_t key[32]) {
    uint8_t sum = 0;
    for (int i = 0; i < 32; ++i)
        sum = uint8_t(sum + key[i]);
    return sum;
}
```

**Collision handling:** the native lookup walks `[bucket+0x24]` (next pointer) until it returns to the sentinel. At each entry, `repz cmpsd` compares eight 32-bit words, i.e. **all 32 key bytes**. This is not a string compare, case-insensitive compare, partial username match or numeric player-index hash. We have not established the key's semantic identity or its lifetime beyond its placement inside the native player-info temporary.

Native insert `0x241E4E30` first allocates **`0x28` bytes**, then copies exactly **eight dwords** into the record before linking it at the end of the selected bucket. A null native allocation returns without inserting. Native remove `0x241E4F20` unlinks the exact matching node's previous/next pointers and calls original deallocation at `0x2416FD60`. New duplicate protection is applied at the `0x241E4F20` caller, not shown inside the raw insertion routine itself.

## `0x241E4F00` and `0x241E4F20`: membership API

- `0x241E4F00` obtains an original node pointer from `0x241E4DD0` and converts non-null to true using the original `neg; sbb; neg` Boolean normalization.
- `0x241E4F20` takes the key and a Boolean. **True** performs lookup and, only if absent, appends a key node. **False** performs lookup and, only if present, splices/free the node. The method has an original `ret 8` and its two Boolean branches have different call sequences. It does not itself issue engine voice commands.

This is a private **hash-set membership** API, independently distinct from the **two red-black trees** recovered in client Phases 542–545.

## Original `CVoiceStatus::SetPlayerBlockedState` at `0x241E5B20`

Its entire **213-byte** body ends at `0x241E5BF5`, with `INT3` immediately following. The observed ordering:

1. Optionally print the original `CVoiceStatus::SetPlayerBlockedState part 1\n` diagnostic when a logging object's `+0x2C` field is nonzero.
2. Call original engine client global `0x2433F684` virtual **`+0x20`**, passing a **one-based player-index argument** and a pointer to an on-stack player-info structure. Exit without further work when the virtual Boolean result is zero.
3. Optionally print the original `... part 2\n` diagnostic.
4. Compute `index = playerOneBased - 1`, then arithmetic shift right by five and test a `1 << (index&31)` bit in the native **`this+0x14`** 32-bit array. If the bit is clear, exit without mutating anything.
5. Take a **32-byte opaque key** starting `+0x24` bytes into the on-stack player-info buffer. Call membership getter `0x241E4F00` with **the subobject at `this+0x28`**. If optional logging is enabled, print original `... setting player %d ban to %d\n` with the displayed Boolean equal to **`!contains(key)`**.
6. Recheck membership with `0x241E4F00` (a second original call), then call `0x241E4F20(key, !contains(key))`. Thus the **observed path toggles key membership**, not a direct write to the gating bitmask.
7. Call already authenticated `CVoiceStatus::UpdateServerState` at `0x241E5880` with **literal argument `0`** (unforced update). Return using original `ret 8`.

Bounded source-like sketch:

```cpp
// Illustrative only. Real player-info layout/engine virtual ABI unresolved.
void CVoiceStatus_like_SetPlayerBlockedState(int playerOneBased,
                                             /* second stack argument */) {
    // Original optional logging here.
    PlayerInfoOpaque playerInfo{};
    if (!original_engine_virtual_plus_0x20(playerOneBased, &playerInfo)) return;
    // Additional optional logging here.
    int idx = playerOneBased - 1;
    if ((this_mask_dwords_plus_0x14[idx >> 5] & (1u << (idx & 31))) == 0)
        return;
    const uint8_t* key = reinterpret_cast<const uint8_t*>(&playerInfo) + 0x24;
    // Native does this twice (once if debugging, again for the actual mutation).
    bool alreadyPresent = key_hash_this_plus_0x28.contains32(key);
    key_hash_this_plus_0x28.setPresent(key, !alreadyPresent);
    UpdateServerState(false);
}
```

**Semantics caveat:** although the original diagnostic says "ban", an exact user-facing mute/unmute mapping is not demonstrated without the peer network/voice update semantics. Additionally, the x86 `ret 8` means two stack argument slots are consumed, but only one argument is directly read in the complete inspected body; the second argument's original meaning is unresolved. We do not infer that the caller necessarily passes an explicit desired Boolean or that the method honors it.

**Safety caveat:** the original directly indexes the native mask array based on the provided integer and shows no local invalid-player-number guard; the independently authored C++ model limits players to `[1,32]` before accessing a single synthetic 32-bit mask. This safety guard is *not* original native semantics. The original method also uses an engine-returned player-info struct whose concrete definition and valid output bytes remain unknown.

## Integration points and remaining reverse-engineering targets

The previous Phase 550 original `CVoiceStatus::UpdateServerState` at `0x241E5880` already called original `0x241E4F00` as an opaque player-state predicate. Phase 551 resolves its dependency to **exact 32-byte hash membership**. The Phase 550 code can now be interpreted as consulting an actual per-identity key set, but its full outgoing voice protocol (`vban`, imported formatting and remote policy) is not yet reconstructed. The original engine virtual `+0x20` supplies the temporary player-info structure, but the specific key fields starting at +0x24 are not identified as a Steam ID, name, GUID or other proprietary player identifier.

Further useful work: resolve native callers of `SetPlayerBlockedState`, map the voice-status object's real constructor and teardown vtable, and decompile the remaining voice status player-state helper(s). Gameplay and game-specific HUD remain priorities; no new physics, surfaces, props, map rendering or Android/VR runtime integration is authorized by this research phase.

## Reproducibility

```sh
python3 research/tools/audit_sin_original_voice_block_hash.py --self-test
python3 research/tools/audit_sin_original_voice_block_hash.py --bin-zip /private/owner/bin.zip
```

Local original owner DLL audit output: `ORIGINAL_CLIENT_VOICE_BLOCK_HASH_AUTHENTICATED` with **8 methods, 657 whole-method bytes, 12 native direct calls, 1 direct tail jump, 44 opcode/field witnesses and 3 exact original diagnostic literals**. The original-free mutation selftest separately corrupts each byte of a synthetic direct CALL/JMP and rejects the wrong original hashes.

The portable reference in `include/sin_compat/original_client_voice_block_hash.hpp` and tests in `tests/test_original_client_voice_block_hash.cpp` verify hash calculation, 32-byte collision comparison, idempotent insertion/removal, bitmask gate and `UpdateServerState(false)` call order, with explicit safe guards. Independent GCC and Clang C++17 warnings-as-errors CMake/CTest passed **2/2** each. These host tests **do not** prove original binary runtime behavior, original Source Engine ABI or complete voice networking.