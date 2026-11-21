# Phase 552: Original `CVoiceStatus` RTTI, construction, registration, teardown and periodic update

## Scope and source identity

This phase re-examines the owner's unchanged 2006 Windows x86 SiN Episodes: Emergence `client.dll` paired with `server.dll`. Their private whole-DLL SHA-256 values are respectively `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d` and `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. The native PE32 client image base is `0x24000000`.

Phase 550 reconstructed `CVoiceStatus::UpdateServerState` and voice masks. Phase 551 decompiled the private 256-bucket, 32-byte identity-key membership table. This phase recovers how the client owns, initializes and destroys that structure, how the voice status callbacks are registered, and when ordinary unforced updates fire. The class identity is now verified by **original MSVC RTTI**, rather than debug-print labels alone.

All report pseudocode is an **independent behavioral transcription of authenticated original control flow**. It is not exact original developer C++ and is not a playable replacement `client.dll`, Source SDK, or Quest/Android implementation.

## Original RTTI and class vtable authenticated

The original client vtable starts at **`0x242A9EF8`**. Its native MSVC CompleteObjectLocator pointer at `vtable - 4` is **`0x242EA63C`**. The CompleteObjectLocator's type-descriptor field is **`0x2431D244`**, where the null-terminated MSVC mangled type string begins at +8:

```text
.?AVCVoiceStatus@@
```

The original three verified vtable entries are:

| Vtable byte slot | Native original method | Evidence-backed role |
| --- | --- | --- |
| `+0x00` | `0x241E5290` | Deleting-destructor wrapper |
| `+0x04` | `0x241E5C60` | Two-argument initialization, VGUI icon and callback registration |
| `+0x08` | `0x241E50F0` | Single-instruction `ret`, no-op in this build |

This identifies the original class **positively**. Source-style method names other than the RTTI class identity and original printed strings are role descriptions, not symbols recovered from a PDB.

## Thirteen complete original native methods, 749 distinct bytes

| Original VA | Length | Exact complete-function SHA-256 | Verified role |
| --- | ---: | --- | --- |
| `0x241E5050` | 27 | `b46aa3159ec436d2a49e839506758499c47fd13919dc8de46790e75fe01c58b6` | Destroy lazily allocated global singleton via vptr+0 and null global |
| `0x241E5070` | 79 | `c23b52c5dc6061d3a1b011cc2156349fabfb16ea4140c084bd74612b84ecb536` | Native `CVoiceStatus` destructor body, release texture, clear active pointer, hash cleanup |
| `0x241E50F0` | 1 | `ae3f4619b0413d70d3004b9131c3752153074e45725be13b9a148978895e359e` | Third vtable entry, one-byte no-op |
| `0x241E5240` | 71 | `992e840f91b5204c5040eb7fc61331082371192c4dd099b7507075a4a3928bb7` | Constructor, vptr and member init, embedded hash constructor |
| `0x241E5290` | 30 | `f5fa2ed765b07194937ec896864b8cb8bcf98276a164063393dd3a26fab062b8` | Deleting-destructor wrapper with conditional object deallocation |
| `0x241E5C00` | 47 | `aeeddef32d803938f3edbbb3bb022bbc3f2f0b0cada06bfd1ed6f4bb93f42e0c` | Lazy singleton allocate (literal `0x2834`) and construct |
| `0x241E5C30` | 48 | `8d70a352bdc6edd025a7586d33dbb7691dc3d17f1fe5dd480b18b1b5189ee820` | Registered `RequestState` callback relay to forced update |
| `0x241E5C60` | 150 | `fef610655478bc1259d0657447c73c845faa98440a40909c1e71a5fb98dc70d7` | Vtable +4 initialization and two named callback registrations |
| `0x241E5D00` | 34 | `33c2ff01ef064e8d4e3dffd5847bf8b2d3721e5b908233153e6150ffec02614f` | One-second threshold periodic unforced update |
| `0x241E5370` | 21 | `a71f0350df22aaf4588c13071c7805011d4ec9f4cda37ee143f185a32ca60907` | Registered `VoiceMask` callback relay |
| `0x241E52B0` | 98 | `5b6dd5061f8bf30ee046727ffb8a399f341d17a54b58198fcc16f0152fc69d61` | Voice-mask callback handler populating native state fields |
| `0x241E51B0` | 70 | `73d1e226ed8a9911a557a4eed32a2ad0ae39dd3fb8c5cdfa8f024a524e652085` | One-based player identity presence-query via Phase551 key set |
| `0x241E5D30` | 73 | `b82b3fe8d1b4abb93c0c100e9eda3805749b9b44fd52a250eccd51a772851115` | VGUI texture visibility/condition dispatch |

The 48-byte `0x241E5C30` callback function **ends immediately before** the 150-byte `0x241E5C60` initializer, with no `INT3` in between. All other methods listed above have original following `INT3` padding. The auditor verifies the different boundary explicitly. Every method is a separate, non-overlapping original code interval. The auditor checks **16 original E8 direct CALLs, one direct E9 tail JMP, 53 selected opcode and field witnesses, five exact original strings, the RTTI chain and the original vtable pointers**.

## Original native object layout and its two global pointers

Original `0x241E5C00` checks global **`0x24378658`** and, when null, calls original allocator `0x2416FCF0` with exactly **`0x2834`** (10,292) bytes. On success it calls original constructor at **`0x241E5240`**, then writes the returned object pointer into `0x24378658`. On allocator failure the global stays null. The source contains no visible concurrency lock.

The constructor writes original vtable pointer `0x242A9EF8`, zeros member fields at `+0x04`, `+0x10`, `+0x14`, `+0x18`, `+0x1C`, `+0x25`, `+0x26`, `+0x2828`, `+0x282C`, and `+0x2830`, sets member `+0x08` to `0xFFFFFFFF`, and invokes the hash-table constructor `0x241E4EB0` on `this+0x28`. The previously recovered embedded 256-bucket hash table spans **`0x28..0x2827`**, exactly ending where the separately initialized icon pointer **`+0x2828`** begins. The Boolean at **`+0x282C`** is a guarded initialization/cleanup flag for an engine-derived interface. The more specific exact types of other fields remain unresolved.

**Two distinct original global pointers must not be conflated:**

- **`0x24378658`**: lazy-constructed singleton owner set by `0x241E5C00`, destroyed and nulled by `0x241E5050`.
- **`0x2437865C`**: active voice-status instance set by vtable +4 initializer `0x241E5C60`, read by registered callbacks, and cleared in destructor `0x241E5070`.

The original code provides evidence for both pointer values and their distinct lifecycles, not for a general thread-safe singleton pattern.

## Vtable +4 initialization at `0x241E5C60`

This complete **150-byte** function returns **1** and pops two 4-byte stack arguments (`ret 8`). Source-backed order:

1. Reads engine client interface global at `0x2433F684` and calls its virtual **`+0x98`**. If the result is non-null, passes that pointer to helper `0x241E4F90` on the **embedded hash at `this+0x28`**, then writes **1** to byte **`this+0x282C`**. The full helper and engine object's meaning are not recovered in this phase.
2. Calls the VGUI-facing object global at `0x2433F69C`, virtual **`+0x6C`**, passing original literal **`voice/icntlk_pl`** and literal **`VGUI textures`** plus two integer arguments. The result is saved to **`this+0x2828`**. The original immediately dereferences this result and invokes its virtual **`+0x30`**. There is **no local null check** before that dereference; our synthetic reference does not reproduce unsafe behavior.
3. Publishes **`this`** to the separate active-instance global **`0x2437865C`**, before the texture virtual call and before callback registration.
4. Copies its two native stack-argument dwords into **`this+0x20`** and **`this+0x0C`** respectively; resets byte **`this+0x24`** to zero.
5. Passes native callback **`0x241E5370`** and original literal **`VoiceMask`** to original registration helper `0x241CFF70` via global object at `0x243010A8`.
6. Passes native callback **`0x241E5C30`** and original literal **`RequestState`** to that same helper, in this order.
7. Returns **true** unconditionally in the visible method code. It does not check a return value from either callback-registration helper.

The exact first entry's `0x241E5370` checks active-instance pointer and forwards the message's argument to the complete 98-byte handler `0x241E52B0`, which fills native mask fields `this+0x14`, `this+0x1C`, `this+0x08` using separately unresolved helpers. The second callback **`0x241E5C30`** checks the active pointer, optionally prints original `CVoiceStatus::HandleReqStateMsg\n`, then calls Phase550-authenticated **`UpdateServerState(1)`**, forcing a refresh. Callback registration is evidenced by original native pointers and two exact strings, not inferred from modern Source Engine code.

## Periodic refresh `0x241E5D00`

This complete 34-byte method reads float from original global object at **`0x242FA458 + 0x0C`**, subtracts **float `this+0x04`** with x87, and compares the elapsed difference to exact original constant **`1.0f`** at rdata **`0x242DE308`**. The `fnstsw ax; test ah, 0x41` branch invokes `UpdateServerState(0)` only on the normal path where the elapsed difference is **at least 1.0**, and skips it for ordinary smaller or unordered values. This is a native *threshold* observation; the meaning of the engine-global clock and any wall-clock guarantee are not established. Phase550 showed that `UpdateServerState` updates this object's last-time field in its tail path.

```cpp
// Bounded reference only; exact original x87 precision and engine clock
// semantics have not been exhaustively simulated.
if (original_engine_global_float_at_plus_0x0c - this_float_at_plus_0x04 >= 1.0f)
    UpdateServerState(false);
```

## Shutdown, native vtable stubs, and VGUI state

- Native global singleton teardown `0x241E5050` tests pointer `0x24378658`, when present invokes original instance vtable +0 with literal deleting flag **1**, then unconditionally writes **0** to the singleton global.
- Native deleting-destructor wrapper `0x241E5290` calls complete `0x241E5070`, then conditionally frees `this` when bit 1 of its native flag is set. The source does **not** establish a cross-engine memory ownership guarantee.
- Destructor `0x241E5070` first calls virtual **`+0x34`** on optional icon handle at `this+0x2828`, clears active pointer **`0x2437865C`**, requests engine client virtual `+0x98`, and if that pointer is non-null and byte `this+0x282C` nonzero, calls original `0x241E4D00` on the embedded table. It then tail-jumps via `0x241E4F80` to Phase551's hash cleanup `0x241E4EC0`. The semantics of `0x241E4D00` and virtuals are not fully decompiled.
- Original vtable **`+0x08`** entry `0x241E50F0` is literally **one `ret` instruction**; no invented shutdown behavior is attached to it.
- Complete VGUI helper `0x241E5D30` acquires a resource through original virtual `+0x6C`, exits if null, and queries its virtual `+0xB0`. If the result is nonzero it exits; otherwise a byte argument controls original virtual `+0x30` versus `+0x34`. Do not invent exact renderer/UI names for these virtual slots.
- Original player membership helper `0x241E51B0` obtains player info through engine virtual `+0x20`, derives the same player-info **32-byte key beginning at local buffer +0x24** as Phase551, and queries hash presence `0x241E4F00`. It returns false when the engine failed to return player info.

## Verification and integration limits

The read-only auditor at `research/tools/audit_sin_original_voice_lifecycle.py` uses Phase551's sibling PE32 and relative-branch verifier. Invoke both on the exact private retail pair:

```sh
python3 research/tools/audit_sin_original_voice_block_hash.py --self-test
python3 research/tools/audit_sin_original_voice_lifecycle.py --self-test
python3 research/tools/audit_sin_original_voice_lifecycle.py --bin-zip /owner/private/bin.zip
```

Original owner audit result: `ORIGINAL_CVOICE_STATUS_LIFECYCLE_AUTHENTICATED`, **13 complete original native methods, 749 additional distinct bytes, 16 direct call edges, one tail jump, 53 instruction witnesses, 5 original string literals, original `CVoiceStatus` RTTI/vtable chain, and the exact 1.0f threshold**.

The independently authored C++17 reference/test checks constructor default field values, the one-second update decision, optional key-table initialization, callback order, forced request relay and the separate VGUI virtual selector. Local strict GCC/Clang and synthetic mutation/ABI checks pass. No real game engine or original native object is executed by these tests, and the source is not a runnable original DLL or a gameplay/Quest port.

Further decompilation priorities: recover the engine-driven `0x241E4F90` and `0x241E4D00` hash import/export helpers and the actual named `VoiceMask` message contents; map the real data-provider type behind the 32-byte key; continue into SiN-specific HUD and player/NPC logic. Physics, surfaces, models and prop systems remain intentionally out of scope.