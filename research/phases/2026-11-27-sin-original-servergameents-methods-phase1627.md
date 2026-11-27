# Phase 1627: Original `ServerGameEnts001` touch-entry and transmit flow

## Evidence provenance

Read-only `objdump` and bounded PE32 reads of the **owner-local original** 2006 `server.dll`; SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`, image base `0x22000000`. The Phase 1626 seven-slot vtable inventory is the foundation. The two large method bodies were already authenticated there; **do not count their 653 bytes again** as new distinct methods. This phase recovers their original branch/call ordering and authenticates a related helper chain. The helper bodies cover 45 + 106 + 526 + 128 = **805 authenticated instruction bytes**; this is a method-region count only, not a claim that none were previously examined elsewhere. It provides source-level models and one new Windows x86 synthetic test, *not* an engine plugin or in-game behavior.

| Region | Original VA | Whole bytes | Full SHA-256 |
| --- | --- | ---: | --- |
| `ServerGameEnts001::MarkEntitiesAsTouching` (slot `+0x08`) | `0x221e9380` | 299 | `8352791af429bd1cf0d040a0765b42ec959181b34d67f98f818a8c039389f1c1` |
| `ServerGameEnts001::CheckTransmit` (slot `+0x18`) | `0x221e94d0` | 354 | `e069400e844edfe9c5e756bfd828abfc6764df7b37ce13dfca374f94c25ad176` |
| trace initializer | `0x22285a10` | 45 | `35e66fcdbd33a37b6440a98ed9b363478da2322bc6fc388461ddb5e0edd2a28d` |
| pair contact helper | `0x22221080` | 106 | `7b7c2ab2d13044215c84ceac8a39306ea31cb1e17f142be6ce2e64771252b5d3` |
| directed contact helper | `0x2221f350` | 526 | `bbdebe8fad62ff42eb88b6b282addbd23be90161aca0a7fd5aec4a814ced8dff` |
| global trace-prefix copy helper | `0x2206e2c0` | 128 | `383458ce02ffe087f4aa8fb8a3d4d68a761eeae74a77673b6e259949dad6f47d` |

All five functions end at observed `INT3` boundaries. Source-like snippets below express **reconstructed control flow**, not original source code.

## Touch entry: verified path

Original slot `+0x08` has `ret 8` and two `edict_t*`-like arguments. For each supplied edict independently, it loads the networkable pointer from edict `+0x0c`, then invokes its virtual `+0x14` to resolve a `CBaseEntity*`. A null edict or null nested networkable yields a null entity; the second edict is still inspected even if the first is absent. If either resolved game entity is null, it returns without trace initialization.

When both resolve, it invokes `0x22285a10` to initialize a **0x54-byte trace record**. This initializer zeros exactly 21 dwords, places float `1.0` at trace `+0x2c`, and seeds trace fields `+0x3c` and `+0x40` from original engine-side globals `0x2251c6e4/0x2251c6e8`. The touch wrapper then tests bit `0x800` of each actor's `+0xb8` flags. The important order is **second entity first** (dirty -> `0x2213fa60`), **first entity second** (dirty -> `0x2213fa60`). It subsequently reads the refreshed origins at each actor `+0x2bc/+0x2c0/+0x2c4`.

The midpoint is computed per component as a float-rounded *sum* of the two actors' coordinates, then multiplied by the same float `0.5` from original VA `0x223d9778`. Three x87 float stores precede the multiplier loads, so this is not a single infinite-precision expression. The final midpoint is written at trace `+0x0c/+0x10/+0x14`. It invokes `0x22221080` with first entity as receiver, second as an argument and the initialized trace record. The original slot returns `void`; there is no success Boolean to propagate. The derived independent wrapper *does* expose error status for missing/failed host services and invalid nonfinite data as explicit safety additions.

Source-like representation:

```cpp
void MarkEntitiesAsTouching(edict_t* firstEdict, edict_t* secondEdict) {
    CBaseEntity* first  = firstEdict  && firstEdict->networkable
        ? firstEdict->networkable->GetBaseEntity() : nullptr;
    CBaseEntity* second = secondEdict && secondEdict->networkable
        ? secondEdict->networkable->GetBaseEntity() : nullptr;
    if (!first || !second) return;
    Trace54 trace; // original constructor 22285a10, including globals
    if (second->flags_b8 & 0x800) second->RefreshOrigin();
    if (first->flags_b8  & 0x800) first->RefreshOrigin();
    for (int i=0;i<3;++i) {
        float roundedSum = float(first->origin[i] + second->origin[i]);
        trace.midpoint[i] = roundedSum * 0.5f;
    }
    first->RegisterPairTouch(second, &trace); // 22221080
}
```

## Directed contact helper chain

At `0x22221080`, the helper passes the trace record to a byte-copy helper `0x2206e2c0`, which copies exactly the first **0x38 bytes** of the trace to engine-owned global `0x22574af8`. It separately copies several trace fields `+0x38..+0x50` to globals `0x22574b30..0x22574b48`, then invokes **the same method `0x2221f350` in both directions**, first with the first entity as receiver and second as target (call `0x222210d8`), then second as receiver and first as target (call `0x222210e0`). The original 526-byte directed helper checks identity, parent handles (generation-aware), flags and existing links before allocating/updating a contact link and triggering callbacks. Its full helper semantics are *not* replaced by this phase; the existing `server_touch_mark_core.h` and `server_reciprocal_contact_core.h` remain higher-level independent models.

## `CheckTransmit` original control-flow audit

Slot `+0x18` ends with `ret 12`, corresponding to a recipient-info pointer, a 16-bit index array and a signed count. It gets an engine-owned table through process-global manager `0x22572198` virtual `+0x4c` (arg `0`) and obtains a recipient actor through a pointer at recipient info `+0` (or fallback manager call), nested `+0x0c` networkable, virtual `+0x14`. If any required pointer is absent, or count is not positive, no candidate loop runs. It snapshots recipient property `+0x828` and byte `+0x92c` **before** iteration.

Every iteration reads a `uint16_t` candidate index, selects a **20-byte entry** (`index*20`) from the engine table, then reads flags masked by `0x38`. Mode `0x10` (`DontSend`) is skipped before anything else. Already-selected indices are also skipped using bitset at context `+0x2008` (index / 32 and index % 32). Mode `0x08` (`Always`) directly sets selected bits, and, for the special recipient flag, bitset at context `+0x200c`; this path does not call `SetTransmit`.

Other candidates use the stored entity pointer at table-entry `+0x0c`. For mode `0`, virtual `+0x54` (`ShouldTransmit`) is called. If its result is `0x08`, virtual `+0x5c` (`SetTransmit`) gets Boolean `true` immediately. Mode `0x20` (either original or returned) enters an area/PVS path: entity subobject at `+0x160` virtual `+0x1c` returns an area number, compared with the previously snapshotted recipient `+0x828`. Equal area invokes `SetTransmit(true)`; mismatching area invokes `SetTransmit(false)` if the snapshotted special-recipient flag is true, or only when helper `0x2227ee90` confirms visibility for ordinary recipients. The loop retains original candidate order, including repeated indices, so a `SetTransmit` callback can mutate subsequent selected bits.

The existing `server_transmit_policy_core.h` implements a bounded independently authored version of the same selected/always bitsets, modes, recipient snapshot, PVS delegates and `SetTransmit` effects. This phase **does not duplicate it**; the newly authenticated instruction sites substantiate that model's flags and dispatch order. Null entities and invalid bitset extents remain safe in the independent model rather than blindly dereferencing trusted retail engine data.

## New implemented ABI-boundary component

`pc/independent_slot0/server_gameents_touch_entry_core.h` adds `MarkEntitiesAsTouchingEntry` with explicit host callbacks for edict/networkable resolution, original trace initialization, dirty-origin refresh and pair-contact registration. It uses a fixed-size 0x54-byte trace payload and writes just its three midpoint floats, preserving owner-initialized trace defaults. No original `edict_t`, `trace_t` or `CBaseEntity` memory layout is fabricated; no original game DLL is loaded by the fixture. The test executable `independent_server_gameents_touch_entry_native` validates ordered callbacks, dirty/clean paths, unresolvable entities, exact trace midpoint fields, init/refresh failure boundaries, invalid finite origins and a downstream contact callback failure. The original routine has no callback-error statuses; those are independent safety semantics and are tracked distinctly.

## Reproduction and limits

`python research/tools/audit_sin_original_servergameents_methods.py --self-test` exercises original-free function-boundary and callsite verifier corruption handling. With the owner-local SHA-identical `server.dll`, use `python research/tools/audit_sin_original_servergameents_methods.py --server /path/to/server.dll --self-test` to authenticate full code-region hashes, original vtable slots, seven call relationships, key instruction witnesses and targeted byte corruption rejections. Neither utility emits commercial executable bytes.

**No retail `ServerGameEnts001` factory is enabled.** Completing the engine-facing ABI still requires actual object owners, original global trace state, directed-contact allocation/destruction and `CheckTransmit` entity/recipient owners. A synthetic native regression and original byte authentication do not demonstrate retail startup, Android gameplay or Quest 3 execution. No physics, surfaces, props, models or rendering are rebuilt.
