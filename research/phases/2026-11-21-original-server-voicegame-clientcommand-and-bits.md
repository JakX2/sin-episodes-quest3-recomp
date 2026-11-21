# Phase 556: original 2006 `CVoiceGameMgr` command dispatcher and bit-state lifecycle

## Scope, provenance, proof boundary

This report continues Phase 555's original `server.dll` 9-byte `VoiceMask` / zero-byte `RequestState` sender into the authentic server voice command receiver. It is **decompilation-first evidence**, not gameplay runtime integration or a byte-exact restoration of Ritual's C++ class definitions. The source is the user's unchanged 2006 PE32 `server.dll` SHA-256:

`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`

The independently checked companion original `client.dll` SHA-256 is:

`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

The audit authenticates *whole original method bodies* using SHA-256, then separately verifies native call/jump targets, strings, key instruction addresses and RTTI. No complete commercial functions, executable machine-code dumps, or original game binaries appear in the GitHub research artifacts.

## Nine newly SHA-authenticated original methods

| Original server VA | Method bytes | SHA-256 of exactly that function's original bytes | Directly evidenced role |
|---|---:|---|---|
| `0x222938C0` | 25 | `244461d7dac17754a8145cb8b1d5231bab16312fe290d9fbdfa18cf2bb77a874` | Native voice-manager constructor: vptr, count and double timer setup |
| `0x222939B0` | 44 | `f9e26f4bf9cb49b43babb3767b91430846ed0ef3fc657d09b4b45b6a18d64eec` | Pair-of-pointers/index bit assignment (`OR`/`AND ~mask`) |
| `0x222939E0` | 28 | `330963a2a959d329b0a0a22efa34b4d41697a06a1c31780be324183d05dea26c` | Pair-of-pointers/index bit membership reader |
| `0x22293A00` | 78 | `6bfdc3b53311c56222ec6c1eead63d41feca77481e4a28f92e2c623ade072871` | Voice diagnostics gated by a global's `+0x2C` value, uses a 4,096-byte formatting buffer |
| `0x22293A50` | 31 | `0fb959725ca93ab3dc5e70a6d74c7279b08c5d8e3cfa946b434bda45e967d09a` | Virtual deleting destructor, optional object free |
| `0x22293AC0` | 66 | `8138935fb4f1b22d370719b96f38020a013937180b2addb2eb6873f2c6de61d0` | Mark per-recipient `RequestState` pending and invalidate two cached masks |
| `0x22293B10` | 473 | `3f06ebffe8e1a23a00aeead64bd1992ebb792ced6099fc26112ea2108a37f950` | `CVoiceGameMgr::ClientCommand` dispatcher for `vban` / `VModEnable` |
| `0x223A5574` | 67 | `d108c0f0291d63e422757a9255fa568f383a772601ba2a66a6f2b79ca9fe7f29` | Character-folded string comparison, dispatching to a locale-dependent or ASCII fallback implementation |
| `0x223A8410` | 78 | `9208efe7b20ebf707490aa3334dabf576a42bfabcb93356dfd2505c20dd6d658` | ASCII-only case-insensitive string comparison fallback |

**Newly authenticated complete, distinct function bytes: 890 across 9 methods.** The previous Phase 555 original server producer at `0x22293CF0` (619 bytes, SHA-256 `4b9346aca0bdbdf927aa5932929c452c1b10a1021d9973a2bbe8ce25fcceb8c8`) is rechecked as an adjacency/cross-phase dependency, **not counted again**.

**Function-boundary trap:** the original `0x223A5574` ends in `ret` at `0x223A55B6` and is immediately followed by a different entry at `0x223A55B7`, with **no `INT3` padding**. The ASCII fallback ends in `ret` at `0x223A845D` and another function begins immediately at `0x223A845E`. These full-function intervals are authenticated by their exact SHA, terminal `ret`, and next entry instruction, not by first-`0xCC` scanning. The remaining seven methods have verified `INT3` after the complete return.

## Original class attribution confirmed by MSVC RTTI

This is **not just a guess from the embedded debug strings**:

- Original vtable is `0x22456AD0` and its slot zero is original deleting destructor `0x22293A50`.
- The 32-bit pointer at `0x22456ACC` (vtable minus four) references MSVC CompleteObjectLocator `0x224C9A6C`.
- The COL's descriptor pointer `0x2251CC9C` contains original native type name `.?AVCVoiceGameMgr@@`.
- Original constructor `0x222938C0` writes vptr `0x22456AD0`, sets `this+0x0C` to zero, and initializes a double accumulator at `this+0x10` from the literal original zero constant.

The original source class is therefore `CVoiceGameMgr`; the other nonvirtual command methods are independently identified by native code and diagnostics, not assumed vtable slots.

## Native game-side state identified

| Original global | Access evidenced by original methods | Confidence |
|---|---|---|
| `0x22572198` | Engine-style service pointer; virtual `+0x48` yields a one-based sender/recipient index | Exact pointer and dispatch, source interface type unresolved |
| `0x22590400` | Per-recipient *voice-enabled* bitfield, set/cleared in `VModEnable` | Exact native bit operation, role supported by native command |
| `0x22590408` | 32-bit `vban` blocked-mask value per recipient, indexed by `4 * sender_zero_based` | Exact original memory write and parser result |
| `0x22590488` | Cached audible-mask 32-bit word per recipient | Exact zero writes in `0x22293AC0`, also used in Phase 555 |
| `0x22590508` | Cached banned-mask 32-bit word per recipient | Exact zero writes in `0x22293AC0`, also used in Phase 555 |
| `0x22590588` | Bitfield of per-recipient pending `RequestState` requests | Exact `OR` in `0x22293AC0`, `AND ~bit` in `VModEnable` |
| `0x225905A8` | Voice diagnostics object with conditional `+0x2C` gate | Exact original load and comparison; no inferred full ConVar type |

Bit helpers use `word_index = signed_bit_index >> 5`, `bit = 1u << (bit_index & 31)`. The original native helpers do not themselves validate bounds; independent host tests do so for safety.

## `0x22293B10`: C-like reconstruction, preserving native branches

This is **bounded pseudocode of verified decisions**, with names and argument types chosen for readability. It is neither reconstructed original source syntax nor a claim that all service methods have been identified.

```cpp
// Descriptive only. Exact class/command strings and immediate state mutations
// are authenticated. Native service vtable calling conventions remain open.
uint8_t CVoiceGameMgr::ClientCommand(void* sender, const char* cmd) {
    const int sender_number = service_virtual_0x48(*(void**)((char*)sender + 0x16C));
    const int index = sender_number - 1;
    if (index < 0 || index >= *(int*)((char*)this + 0x0C)) {
        voice_debug("CVoiceGameMgr::ClientCommand: cmd %s from invalid client (%d)\n",
                    cmd, index);
        return 1; // surprising but original instruction is al=1
    }

    if (original_casefold_cmp(cmd, "vban") == 0) {
        if (service_argc() < 2) return 0;
        for (int arg = 1; arg < service_argc(); ++arg) {
            uint32_t mask = 0;
            original_scanf_style(service_argv(arg), "%x", &mask);
            // Native ignores conversion result. A failed parse retains zero.
            if (arg == 1) {
                voice_debug("CVoiceGameMgr::ClientCommand: vban (0x%x) from %d\n",
                            mask, index);
                g_server_ban_mask[index] = mask;
            } else {
                voice_debug("CVoiceGameMgr::ClientCommand: invalid index (%d)\n", arg);
                // Additional mask arguments are NOT written.
            }
        }
        return 1;
    }

    if (original_casefold_cmp(cmd, "VModEnable") == 0) {
        if (service_argc() < 2) return 0;
        const int enabled = original_string_to_int(service_argv(1)) != 0;
        voice_debug("CVoiceGameMgr::ClientCommand: VModEnable (%d)\n", enabled);
        bit_set(g_voice_enabled, index, enabled);
        bit_set(g_pending_requests, index, false);
        return 1;
    }
    return 0;
}
```

The original specific calls are:
- `0x22293B4B` and `0x22293C16` -> `0x223A5574` for command name comparison.
- `0x22293BB7` -> `0x223A5CD6`, the native `%x` scan-style parser.
- `0x22293C4E` and `0x22293C84` -> numeric parser `0x223A56CB`, normalizing any nonzero integer into true.
- `0x22293C97` -> the exact `0x222939B0` bit writer; `0x22293CB8` directly clears the pending bit.
- Native command-argument interface virtual `+0xC4` supplies `argc`, `+0xC8` supplies `argv(index)`.

The original code uses a loop for all `vban` arguments but only index **1** changes the global per-recipient blocked mask. Additional arguments merely trigger an invalid-index diagnostic. Returning true for an invalid sender and ignoring `sscanf` conversion failures are verified native behaviors and are **not repaired in this decompilation report**.

## `0x22293AC0`: recipient refresh request

The native function calls original service virtual `+0x48` with its supplied client handle. It subtracts 1 from the returned index, sets the corresponding bit in the `0x22590588` pending-request global, and writes zero into **two per-recipient cached values** at `0x22590488 + 4*index` and `0x22590508 + 4*index`. The previously authenticated Phase 555 periodic sender tests the pending bit and emits the zero-payload `RequestState` message before clearing it.

```cpp
// Native original doesn't validate index bounds here.
void MarkVoiceStateDirty(void* client_handle) {
    int i = service_virtual_0x48(client_handle) - 1;
    g_pending_requests[i >> 5] |= (1u << (i & 31));
    g_cached_audible[i] = 0;
    g_cached_banned[i] = 0;
}
```

This is a concrete connection between **server command receipt**, **request scheduling**, and **the cross-DLL wire protocol** proven in Phases 554–555.

## Case-insensitive command comparison, exact original behavior boundary

The original dispatcher calls `0x223A5574`, which tests native locale-readiness global `0x2259F500`. When zero, a native `JMP rel32` goes to `0x223A8410`, whose 78-byte complete function implements unsigned byte comparison with ASCII `A`..`Z` case folding. In the locale-ready path, `0x223A5574` uses original character-fold helper `0x223A72DA` for both operands and returns their signed difference. This is an original **case-insensitive** name comparison; there is no need to assume case-sensitive `strcmp`.

The independent reference tests ASCII fallback matching, including mixed-case `VBAN` / `vmOdEnAbLe`. **Active retail locale state and all non-ASCII conversion rules are not proven**.

## Reproducibility

The new read-only auditor is `research/tools/audit_sin_original_server_voice_commands.py`, reusing the repo's Phase 551 original PE32 reader. With the private original owner archive:

```bash
python3 research/tools/audit_sin_original_server_voice_commands.py --self-test
python3 research/tools/audit_sin_original_server_voice_commands.py --bin-zip /path/to/private/bin.zip
```

Owner-local result: `ORIGINAL_SERVER_CVOICEGAMEMGR_COMMANDS_AUTHENTICATED` with **9 full original-method SHA checks, 890 unique server instruction bytes, 15 rel32 direct CALL edges, one direct tail JMP, 50 instruction witnesses, seven exact original command/log strings, one original class RTTI chain**, and Phase 555 producer reauthentication without double-counting.

The standalone original-free `original_server_voice_commands.hpp` plus `test_original_server_voice_commands.cpp` test 1-based/0-based player conversion, 32-bit word boundary, pending-state invalidation and clearing, first-argument `vban`, invalid sender return, insufficient argc, unknown commands, and ASCII fallback comparison. C++17 strict GCC/Clang tests and ASan/UBSan passed locally. Input bounds protection in the reference is **an explicit modern host safety rule**, not a discovery about the original 2006 code.

## Remaining original source work

- Determine exact engine service/command argv interface types and per-recipient storage ownership; currently only native virtual offsets and literal array addresses are authenticated.
- Resolve the complete per-recipient eligibility virtuals in original `0x22293CF0`, if needed for fully accurate voice routing and client/server state synchronization.
- Recover original client `vban` text formatting and numeric conversion nuances if reimplementing the full runtime protocol.
- Return to higher-priority SiN-specific weapons, NPCs, scripted entities, and original player code after completing this small cross-DLL voice subsystem. The user's HL2-working physics, models and surfaces remain out of scope.

**Proof level:** complete native byte authenticity for the nine methods; branch-level readable reconstruction for the dispatcher, bit helpers and refresh marker; not original-source equivalence for CRT, engine virtuals, locale folding, or gameplay port execution.