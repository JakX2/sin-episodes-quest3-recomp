# Phase 550: Original client voice-status update, player bitfields, and server command path

## Owner-original evidence and limit

This decompilation examines the owner's unchanged **2006 SiN Episodes: Emergence `client.dll`**, SHA-256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`, alongside original server SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. It follows the **`0x2433F680` client map-initialized global byte** recovered in Phase549 into a separate voice-status subsystem. Genuine original diagnostic strings explicitly name the source class **`CVoiceStatus`**. These are original strings, not reconstructed MSVC RTTI. Class attribution for the update/handle functions is corroborated by those strings and their mutual call relationship, not by an independently mapped CVoiceStatus vtable.

This phase verifies **four distinct complete native functions totaling 635 non-overlapping instruction bytes**. The description is a bounded reverse-engineered C-like view, **not** exact original Ritual C++ and not a functional voice-network system. Unknown virtual semantics, engine object types, ownership and dynamic buffer safety are recorded rather than invented.

## Four original complete methods

| Original native client VA | Length | Full SHA-256 | Verified role |
|---|---:|---|---|
| `0x241E5880` | 516 | `5cba2a0d5d5050b037efaadbf8e4f2d1b3301be419a1a8123a7752d3eacbddbe` | Voice-status update, command construction, 32-player bit scans, map-gate |
| `0x241E5A90` | 41 | `c65f98bb8d7dec1aa9f5d3e5a51f22845a4e5550a1063a0bf0aa3b864a7f40bd` | Named request-state handler; forces the update method |
| `0x241E5AC0` | 39 | `d6c9929c6d2dfe4663f62022a23270004b1873a578397bdd27baf4240ad48bda` | 1-based player mask query, storage starting at object `+0x14` |
| `0x241E5AF0` | 39 | `cc8e3aacf17ca10ebfaabc63b34fd1a8b1e7c7b4bdd286819f6aa38610c73a7d` | 1-based player mask query, storage starting at object `+0x10` |

All four end directly before original `INT3` padding; their exact instruction bytes and sizes were SHA-authenticated against the original PE32 owner source. New calls are original `0x241E5979` -> `0x241E4F00` and original `0x241E5AB0` -> `0x241E5880`. Original interface calls through native vtable pointers and imported formatting / diagnostic functions are verified separately as instruction witnesses, but are **not** miscounted as direct `E8` calls.

## The client map state controls voice status updates

The 516-byte method at `0x241E5880` has an approximately `0xB0C`-byte stack frame. Its first branch tests the same byte at **`0x2433F680`** that the original `CHLClient` map pre-entry and shutdown methods set and clear in Phase549:

- If the map flag is zero, the normal state update is **not** run. When a logging global at `0x243786C0 + 0x2C` is nonzero, the method emits the original diagnostic `CVoiceStatus::UpdateServerState: g_bLevelInitialized\n` via original imported logging callback at `0x242761EC`. It then returns with one 4-byte argument popped. When logging is disabled, it flows to a later return block without running the full update.
- If the map flag is nonzero, it reads another global object at `0x24378678`, derives a Boolean from its `+0x2C` field, compares that Boolean with stored voice-status field **`this+0x08`**, and checks its one-byte call argument (a forced-update flag in this observed path).
- If the passed flag is nonzero, or the stored and current Boolean differ, it stores the new value in **`this+0x08`**, formats `VModEnable %d` using an original 256-byte stack buffer and invokes original `VEngineClient012` interface virtual **byte `+0x18`**, passing command address and literal argument **1**. The exact semantics of the `+0x2C` field and virtual call flag are not established.
- If neither forced nor changed, it skips this particular `VModEnable` command; this is *not* proof that the whole method produces no subsequent command, because it continues into the voice-ban processing path.

Source-bounded sketch for this **one conditional command**, not the full method:

```cpp
// Descriptive only: native_g_bLevelInitialized = byte at 0x2433F680.
if (!native_g_bLevelInitialized) {
    // May emit original diagnostic when logging is enabled.
    return;
}
const bool newEnableBit = (native_global_24378678.field_0x2c != 0);
if (force || native_this.field_0x08 != newEnableBit) {
    native_this.field_0x08 = newEnableBit;
    char buffer[256]; // original code supplies literal 0x100
    original_format(buffer, sizeof(buffer), "VModEnable %d", newEnableBit);
    OriginalEngineClientVirtual0x18(buffer, 1);
}
// Separate original vban mask update follows.
```

**Important**: The original code's actual formatting calls and stack movements have not been evaluated for all overflow/null cases. Do not turn the sketch into a production engine command emitter without recovering the true imported formatting ABI.

## `vban` command construction and player scanning

After the enable-bit decision, the original function initializes a large stack buffer using exact literal **`vban`** at client rdata `0x242A9FF0` and a native formatting argument **`0x800`**. It loops with an upper bound of **32 player slots**, and within the loop:

1. Calls `VEngineClient012` virtual **byte `+0x20`** with a **1-based player index** (`esi+1` in original instructions) and a pointer to temporary player data. The original return in `AL` determines whether later per-player processing runs.
2. On a positive result, calls original helper **`0x241E4F00`** with a pointer into a voice-status subobject at `this+0x28` and a temporary data address. If the helper returns true, it adds the player's bit to a new 32-bit bitmask.
3. Independently tests a bit from original voice-status array at an address based on `this+0x1C` and the current bit index; if present, it updates an additional outgoing bitmask.
4. Compares one collected bitmask with another remembered bitmask and sets a local change indicator when they differ.
5. Formats the outgoing hex bitmask with exact literal **` %x`** at original `0x242A9FEC`, concatenates it to a command buffer using the original imported formatting routines, conditionally emits original `no change` or `Sending '%s'` debug messages, and in the sending branch calls `VEngineClient012` virtual **`+0x18`** with the resulting text and literal flag **0**.
6. Near return it assigns the original `0x242FA458 + 0x0C` field to `this+0x04` (an engine-global snapshot whose type is unverified).

The original formatting routines are indirect imported calls at e.g. `0x24276274`, `0x2427626C`, and `0x2427622C`. Their precise string-safety and truncation semantics remain unresolved. The two masked states at `this+0x1C` and the calculated outgoing mask must not be assigned modern voice-chat semantics without further source evidence.

The original diagnostic text is verified byte-for-byte:

- `CVoiceStatus::UpdateServerState: g_bLevelInitialized\n` (`0x242AA038`)
- `VModEnable %d` (`0x242AA028`)
- `CVoiceStatus::UpdateServerState: Sending '%s'\n` (`0x242A9FF8`)
- `vban` (`0x242A9FF0`)
- ` %x` (`0x242A9FEC`)
- `CVoiceStatus::UpdateServerState: no change\n` (`0x242A9FC0`)
- `CVoiceStatus::HandleReqStateMsg\n` (`0x242AA070`)

## Request-state message handler `0x241E5A90`

This complete **41-byte** method optionally logs the original `CVoiceStatus::HandleReqStateMsg\n` message, pushes literal **1**, calls original `0x241E5880`, restores saved registers and returns with a single 4-byte argument popped. Thus the observed original *request state* path invokes the same server-state refresh with a forced flag; it is not a wholly separate implementation of voice-ban bitsets.

## Two 1-based player bitfield getter methods

The original code contains two sibling **39-byte** functions:

- `0x241E5AC0` looks up a bit in the 32-bit dword array rooted at object **`+0x14`**.
- `0x241E5AF0` looks up a bit in the 32-bit dword array rooted at object **`+0x10`**.

Both compute **`playerOneBased - 1`**, the low five bits as bit position, and signed arithmetic shift five for the dword index. After a mask AND, the original x86 `neg; sbb; neg` sequence normalizes the result to **Boolean 0 or 1**.

```cpp
// Descriptive of original mask arithmetic; not a memory-safe original method.
int playerIndex = playerOneBased - 1;
int dwordIndex = playerIndex >> 5;            // original x86 arithmetic SAR 5
uint32_t bit = uint32_t(1) << (playerIndex & 31);
return (native_object_mask[dwordIndex] & bit) != 0;
```

The original doesn't visibly guard negative player indices or out-of-range dword indices. The separately authored C++17 reference deliberately returns false for invalid synthetic player inputs and null arrays instead of reproducing native unsafe memory reads.

## Verification and next targets

The reproducible original-bound auditor at `research/tools/audit_sin_original_client_voice_status.py` checks:

- Both exact owner original DLL SHA-256 identities and the original PE32 client image base.
- **Four whole native SHA-256 method digests = 635 unique code bytes**, all four executable `.text` mappings and post-method INT3 boundaries.
- **Two exact original rel32 CALL edges**, **seven original null-terminated strings**, and **40 source-level instruction witnesses** including the map gate, forced-refresh call, 32-player bound, source bitfield offsets, and engine virtual call byte offsets.
- Original-free `--self-test` rejects every single-byte mutation of a fabricated direct CALL and the hashes of fake DLLs.

Independent portable 
`include/sin_compat/original_client_voice_status.hpp` and
`tests/test_original_client_voice_status.cpp` capture only verified synthetic bit-index/enable-command decisions. They do **not** implement network voice commands, real CVoiceStatus objects, or the original imported formatting engine. GCC and Clang C++17 warnings-as-errors tests are run separately.

Next high-value original code: `0x241E5B20` (appears to contain `SetPlayerBlockedState` diagnostics and a player flag mutation path), `0x241E4F00` (checks a player within voice-status state), and engine lifecycle call sites tying this subsystem to the original SiN HUD, not VR/Android work.