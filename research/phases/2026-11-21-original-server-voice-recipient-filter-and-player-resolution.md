# Phase 557: Original 2006 server `CRecipientFilter` voice routing and player resolution

## Evidence scope

Follows Phase 555 original server `VoiceMask` producer at `0x22293CF0` into the **actual temporary recipient-filter constructor**, **per-recipient insertion**, and **stack teardown** directly called by that native method. This is needed to understand *who receives the voice mask*; it does not introduce unrelated Source physics, assets, prop work, game runtime shims or VR support.

Original owner binary identities (independently checked before all opcode validation):

- `server.dll`: SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- `client.dll`: SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

Native game DLL bytes are private. GitHub contains only independently authored C-like research, addresses/checksums, original-free reference code and tests.

## Five new complete native methods

| Original server VA | Size | SHA256 of exact full native method | Source-backed role |
|---|---:|---|---|
| `0x222701C0` | 41 | `71cd95fee2a0a21a5ef5f62c48d06a9a3fba1b4ad5f7b92064fef5b7fc5c4ce2` | `CRecipientFilter` default constructor, initializes member flags, dynamic list pointer and count |
| `0x222701F0` | 25 | `7993bbe405b75946ccbdcb6a8057f7392b00e05d6da96bf05fb424de79009fbb` | Stack teardown: release internal list, restore base interface vptr |
| `0x222702A0` | 45 | `ef0da906d7a86f04913764f18e12c03f17356c724774f54516eb896306d4ad4e` | Virtual deleting destructor for heap-owned recipient filter |
| `0x222706B0` | 137 | `375dfd003e35d9619921253deb1f78fa29b70d102e770b842e59ac37efb2f4c5` | Resolve native player number, deduplicate recipient list, append if eligible |
| `0x22286C70` | 70 | `711bbc293e6224c764f1d535973921556ba395ac55b85910f3e81b75af962e55` | Resolve 1-based player number to an active server player object through native edict and virtual dispatch |

**Newly SHA-verified complete original server bytes: 318, 5 distinct functions.** The Phase 555 original 619-byte `0x22293CF0` is rechecked, not counted again. All five new return boundaries have original `0xCC` padding immediately afterward; each method length is verified independently.

## Class identities are genuine original RTTI, not guessed from names

| Object | Vtable | COL at vtable - 4 | Native type descriptor | Exact original MSVC type string |
|---|---|---|---|---|
| Concrete filter | `0x224546B0` | `0x224C8B8C` | `0x224D87DC` | `.?AVCRecipientFilter@@` |
| Base interface | `0x224321D0` | `0x224C2138` | `0x224D87BC` | `.?AVIRecipientFilter@@` |

The concrete filter's vtable slot 0 points at the complete original deleting destructor `0x222702A0`. The native constructor writes concrete vtable `0x224546B0`. The ordinary cleanup method first restores the concrete pointer while freeing the owned dynamic list then leaves base vtable `0x224321D0` when done. This is a native x86 lifecycle fact, **not a claim that the independent Quest port implements those C++ ABIs**.

## Original producer-to-recipient call chain

Exact original Phase 555 sender calls:

| Caller original VA | Native E8 target | Implication |
|---|---|---|
| `0x22293D64` | `0x222701C0` | Construct a temporary `CRecipientFilter` at the start of voice recipient setup |
| `0x22293D76` | `0x222706B0` | Add the intended player object to that temporary filter |
| `0x22293F39` | `0x222701F0` | Tear down the filter and its dynamic array after message work |

The original `CRecipientFilter` contains an embedded dynamic recipient-index array starting at `this+0x08` and a logical entry count at `this+0x14`. Its constructor clears multiple flags and array fields; its destructor frees the owned vector through original helper `0x2206F3D0`.

## `0x222706B0`: bounded source-like reconstruction

1. Takes an original player pointer, loads its network/engine linkage at `player+0x16C`, then obtains a **one-based player number** via engine service virtual `+0x48` from global `0x22572198`.
2. Checks the filter flag byte at **`this+0x1C`**. If nonzero, performs an additional pointer-based branch involving native globals `0x2251BA48` / `0x2251BA4C`. The complete meaning of these globals is **not yet established**. This special path can skip addition.
3. In the normal path, iterates all existing 32-bit player numbers stored in the dynamic array `this+0x08` of logical size `this+0x14`.
4. **Does not add a duplicate.** If the given player number is absent, invokes original dynamic-array helper `0x22095600` to append it. The empty-list path calls the same append helper.

Approximate, **normal-path only** pseudocode:

```cpp
// Source-bounded pseudocode, not retail exact class or ABI.
void CRecipientFilter::AddPlayer(/* original engine player */ void* player) {
    int number = service_virtual_0x48(*(void**)((char*)player + 0x16C));
    if (native_special_filter_flag_at_0x1C) {
        // Unresolved original pointer-selection rule: can skip this player.
        if (original_special_filter_rule_skips_player(player)) return;
    }
    for (int i = 0; i < recipient_count; ++i) {
        if (recipients[i] == number) return; // duplicate suppressed
    }
    original_dynamic_array_append(number);
}
```

The original code itself has no newly inferred host player-index bounds checks in this insertion method. The C++ reference intentionally rejects non-positive numbers for host safety; **that behavior is not attributed to the original**.

## `0x22286C70`: original player resolver

The independent 70-byte helper is called by the already-authenticated Phase 555 voice sender at `0x22293D38` and `0x22293DDF` to resolve one-based recipient/other-player numbers. Original control flow:

1. Reject `player_number <= 0` with null.
2. Reject `player_number > *(service_global_0x22572190+0x14)` with null.
3. Invoke native service `0x22572198` virtual `+0x4C` to obtain an edict-like pointer for that number. Null means not present.
4. Read its original first DWORD flags; reject when original bit **`0x02`** is set (`shr flags,1`, then `test bit0`).
5. Read owner pointer at original edict `+0x0C`; null means not present.
6. Tail-dispatch owner vtable byte `+0x14`; that virtual's exact named type and return semantics are not yet recovered, so the report records the dispatch rather than inventing its purpose.

```cpp
void* ResolveServerPlayer(int player_number) {
    if (player_number <= 0) return nullptr;
    if (player_number > original_engine_player_count) return nullptr;
    void* edict = engine_virtual_0x4C(player_number);
    if (!edict || (original_edict_flags(edict) & 2)) return nullptr;
    void* owner = *(void**)((char*)edict + 0x0C);
    if (!owner) return nullptr;
    return owner_virtual_0x14(owner); // exact original virtual call, name unknown
}
```

This validates that the original voice producer selects actual eligible player objects, rather than merely using arbitrary `VoiceMask` string recipients.

## Reproducibility and current limits

The owner-local auditor `research/tools/audit_sin_original_server_voice_recipients.py` performs exact SHA verification on both original game DLLs, then checks **5 complete original method digests, 318 distinct native bytes, 8 original E8 call edges, 29 instruction witnesses, two native MSVC RTTI chains, vtable destructor pointers and the previously counted Phase 555 server sender**.

```bash
python3 research/tools/audit_sin_original_server_voice_recipients.py --self-test
python3 research/tools/audit_sin_original_server_voice_recipients.py --bin-zip /path/to/private/bin.zip
```

The independently authored C++17 reference tests only proven normal-path filter insertion (list order/dedup), numeric player-range and missing-edict/flagged-edict/owner-dispatch gates. Tests pass strict GCC, Clang, CTest and sanitizers. They do **not** implement the special filter flag at `+0x1C`, the original dynamic-array growth/failure behavior or engine virtual calls.

Remaining open: full engine recipient-filter ABI, explicit semantic name for the virtual `+0x14` in the player resolver, and the optional filter flag's global-pointer selection rule. Higher priority after this voice-specific chain is authentic SiN player/NPC/weapon gameplay, not generic Source physics or prop/model code.