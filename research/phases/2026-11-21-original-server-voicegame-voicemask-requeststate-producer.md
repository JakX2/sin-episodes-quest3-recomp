# Phase 555: original server `VoiceMask` and `RequestState` production, verified against client decoder

## Source integrity and evidence boundary

This cross-DLL phase uses the user's exact original **2006 SiN Episodes: Emergence Windows x86** `server.dll` SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`, and `client.dll` SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`. Native server image base is `0x22000000`, native client base `0x24000000`. The report's C-like reconstructions are independently authored descriptions of original bytes, **not original Ritual source code**. No commercial DLL bytes or game assets are committed.

Phase554 authenticated the client `VoiceMask` parser's **32 + 32 + 8 bits**. Original server string references now lead directly to its **user-message registration and corresponding message writer**. These confirm the order and fixed size end-to-end, rather than extrapolating from just one DLL.

### Eight new complete original server methods, 855 code bytes

| VA, server.dll | Exact bytes | Complete original function SHA256 | Bounded role |
| --- | ---: | --- | --- |
| `0x22293CF0` | 619 | `4b9346aca0bdbdf927aa5932929c452c1b10a1021d9973a2bbe8ce25fcceb8c8` | Server voice manager per-recipient update, RequestState and VoiceMask decision/sends |
| `0x22293F60` | 31 | `6ac092bb3d7ca051f8340b9c10db7f90a75f6d9eb80091a6d1a231bd2ea3e619` | Periodic elapsed-time accumulation, native 0.3-second compare |
| `0x221E86D0` | 68 | `3709a3e1d6548cfab75e1c2a036a3983e795036dae5c66542fa1a6520d6c7642` | Named message registration lookup, begin native user message |
| `0x221E8720` | 25 | `6621698cfb136a2fa1a7184901662316a923e84481fc32a525305c00e9760f2d` | Finish native message and clear current writer global |
| `0x221E8740` | 40 | `e20a47c6e04e4bc52e739107ac04d4dacd26ea7507d160dd97985d438de0a275` | Message 8-bit write wrapper |
| `0x221E8800` | 40 | `a6a02d0c8fb9ed8b6b8ac07c79a09b288e9372ff14b09bd24e66634d7fbf0ab8` | Message 32-bit write wrapper |
| `0x22258EC0` | 17 | `feb386edf6daf296630273762eb63d35673fcfafa9afabaec4c70816cb180abb` | Low-level bit writer, literal count 8 |
| `0x2225A170` | 15 | `659ae1684a01183326c1249adad18958db0475afc4490097147cc6645292529c` | Low-level bit writer, literal count 32 |

Eight disjoint complete original server method ranges, **855 new original server code bytes**. All eight full digests and their following `INT3` method padding are authenticated. Twenty native relative CALL edges (some in already existing registration code), 49 individual opcode witnesses and three original message/debug strings are checked by the binary auditor. The original client handler and two client read wrappers are reverified by their earlier known complete SHA digests, **without counting the client code a second time**.

## Original registration: 9-byte `VoiceMask`, 0-byte `RequestState`

The original **server** user-message registration method (analyzed only at these instruction witnesses, not counted here as a complete new function) contains:

| Native instruction site | Evidence | Result |
| --- | --- | --- |
| `0x222CFD10` | `push 9` | Registered fixed message size **9 bytes** |
| `0x222CFD12` | `push 0x22456BC4` | Original string literal `VoiceMask` |
| `0x222CFD17` | direct CALL `0x220229A0` | Register that exact string/size pair |
| `0x222CFD22` | `push 0` | Registered fixed message size **0 bytes** |
| `0x222CFD24` | `push 0x22456BD0` | Original string literal `RequestState` |
| `0x222CFD29` | direct CALL `0x220229A0` | Register the zero-size counterpart |

These exact original server strings reside in `.rdata` at `0x22456BC4` and `0x22456BD0`. This corroborates the client registration of its `VoiceMask` and `RequestState` handlers in Phase552. `0` is **not** a guessed variable-length marker: here the sender's original `RequestState` path begins and immediately ends the message with no value writes. However, the full registration helper implementation/engine framing is not translated in this phase.

## Server producer: original `0x22293CF0`

The original nearby diagnostic strings use **`CVoiceGameMgr::ClientCommand`** wording, such as `VModEnable` and `vban`. We use **server voice manager** as a descriptive role without asserting its complete C++ RTTI. The 619-byte method receives a native object pointer in ECX, resets its double accumulator at `this+0x10`, and loops an object-owned player count at `this+0x0C`. It obtains recipients by one-based index through original `0x22286C70` and checks an original player virtual byte offset `+0x120` before constructing recipient-scoped messages.

### Original `RequestState` send (no payload)

At a native bitmask global based at `0x22590588` the server checks whether the current recipient has a pending request-state bit. If set:

1. Begins a message with exact native literal **`RequestState`** by calling `0x221E86D0` at **`0x22293DA2`**.
2. Immediately finishes it with `0x221E8720` at **`0x22293DAA`**, with **no write calls** between.
3. Clears that recipient's corresponding request bit at **`0x22293DB3`**.

This matches the exact registered size zero and the client's forced-refresh callback, which calls `UpdateServerState(1)`.

### Original `VoiceMask` send (9-byte fixed payload)

For each valid recipient the method also computes a 32-bit candidate mask based on the server's per-player voice state and a separate original virtual evaluation. The method compares this new value with a cached mask at native global `0x22590488`, and compares another native server mask based at `0x22590408` with its cached copy based at `0x22590508`. If the compared state has changed, it updates the cached values then begins a **`VoiceMask`** message (literal pointer `0x22456BC4`, begin at `0x22293EA7`). The exact original writes follow in this order:

| Native CALL | Writer | Value source | Serialized size |
| --- | --- | --- | --- |
| `0x22293EB1` | `0x221E8800` | 32-bit *computed audible mask* from recipient loop | **32 bits** |
| `0x22293EBD` | `0x221E8800` | 32-bit *server blocked-state mask* from global `0x22590408` | **32 bits** |
| `0x22293ECF` | `0x221E8740` | Boolean normalized from current recipient's bit in global `0x22590400` | **8 bits** |

The method then calls original message-finish `0x221E8720` at `0x22293ED7`. The wrappers themselves are independently authenticated: `0x221E8800` delegates with a literal 32-bit width to `0x2225A170`, while `0x221E8740` delegates with literal 8-bit width to `0x22258EC0`. These low-level methods in turn forward to original bit-packing helpers. **Total: 72 bits = exactly 9 bytes**, independently matching the original registration length.

A bounded pure reference of the *sender's serialization decision* is:

```cpp
// Native pointers and recipient eligibility are deliberately abstract.
if (request_state_pending_for_recipient) {
    SendMessage(recipient, "RequestState", /*payload*/{});
    clear_request_state_pending_for_recipient();
}
if (new_audible_mask != cached_audible_mask ||
    new_server_banned_mask != cached_server_banned_mask) {
    cached_audible_mask = new_audible_mask;
    cached_server_banned_mask = new_server_banned_mask;
    WriteMessageBegin(recipient, "VoiceMask");
    WriteLong32(new_audible_mask);
    WriteLong32(new_server_banned_mask);
    WriteByte8(bool(current_recipient_mask_bit));
    MessageEnd();
}
```

**Important caveats:** the actual recipient mask-computation policy, team rules and native eligibility virtuals are not completely decompiled. Global addresses are not proof that fields belong to a particular source C++ array/class. The sketch does not implement the native engine network writer and must not be treated as a game server DLL.

## Timer and message ABI: no invented types

The complete 31-byte method `0x22293F60` adds the supplied floating-point delta to the native double at manager `+0x10`, compares against original double constant at `0x223DA4A0`, and invokes the 619-byte update at `0x22293F77` on the elapsed-threshold path. That original double is **0.3**, confirmed from its eight native bytes. The update resets its accumulator to **0.0** using an original QWORD constant at `0x223D8F98` at entry. This documents a server **~0.3-second accumulation threshold**, not a guaranteed wall-clock interval under all frame schedulers.

The original message-start helper `0x221E86D0` calls registered-name lookup `0x22022710`, then the engine interface at global `0x22572198` virtual byte `+0xAC`, storing the current writer pointer in global `0x225721F8`. The original finish helper calls virtual `+0xB0` and clears that pointer. Each value-writing wrapper checks whether the native writer exists and emits an original diagnostic if absent, but the wrapper then proceeds into its writer call. The complete ABI and invalid-message behavior of the engine object are unresolved. The phase reuses previously established client reader types as **binary evidence**, not as proof of direct native server/client pointer interoperability.

## Cross-DLL verification and original-free regression

The original client VoiceMask handler from Phase552, at `0x241E52B0`, was SHA-authenticated as a 98-byte complete function and its two read wrappers at `0x240A5620` and `0x240A55F0` were SHA-authenticated in Phase554. This verifier rechecks these three whole method digests against the exact original client. Client consumption is the **same 32+32+8** and stores the same ordered values in 
`CVoiceStatus+0x14`, `+0x1C`, `+0x08`. The field named **`m_AudiblePlayers[0]`** is from original client diagnostics. The final status-byte semantics are proven at the server as a **per-recipient bit-test Boolean**; further high-level voice enable/mute policy is not inferred.

An original-free C++17 reference in `include/sin_compat/original_server_voice_mask_wire.hpp` and test `tests/test_original_server_voice_mask_wire.cpp` serialize the three values into the nine-byte source-supported layout, then immediately decode them using the Phase554 client reference. Tests include high-bit and all-ones mask words, zero-length RequestState, change-gated VoiceMask and ordering when both messages are due. Defensive synthetic host logic is not claimed to be original production source.

Verify with:

```sh
python3 research/tools/audit_sin_original_server_voice_mask_producer.py --self-test
python3 research/tools/audit_sin_original_server_voice_mask_producer.py \
   --bin-zip /path/to/owner/original/bin.zip
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Local owner-original auditor reported `ORIGINAL_SERVER_VOICE_MASK_PRODUCER_AUTHENTICATED`: **eight new complete original server methods, 855 distinct server instruction bytes, 20 selected CALL edges, 49 opcode witnesses, three original native labels, original registration 9/0, 32+32+8 writers, and 0.3-second native double threshold**. GCC and Clang C++17 and 2/2 local CTest pass. No real multiplayer gameplay or original server network call was executed.

## Next decompilation

Research priorities now include: original server `VoiceGameMgr::ClientCommand` parsing of `VModEnable` and `vban` (evidenced by its native diagnostic strings); client voice-status blocklist synchronization; the remaining per-player gating virtuals in server `0x22293CF0`; and eventually SiN-specific HUD, player, weapon and NPC logic. Physics, surfaces, prop/model reading/rendering and VR/Android implementation remain explicitly deferred.