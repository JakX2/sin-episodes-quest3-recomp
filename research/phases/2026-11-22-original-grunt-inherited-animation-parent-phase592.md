# Gameplay Phase 592: Original Grunt inherited animation dispatcher and optional component consumption

## Scope and binary provenance

This phase follows the Phase591 Jetpack `+0x444` default handler, which passes original event `0x54` through the flying-attachment predicate and into the inherited Grunt `+0x444` at native VA `0x2235BFD0`. The user priority is original SiN Episodes: Emergence 2006 gameplay source, not generic Source/HL2 physics, surfaces, props, model rendering, Windows loader, Android or VR.

Owner-private full-file SHA256 prerequisites:

- Original `server.dll`: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- Original `client.dll`: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- Preferred PE image base `0x22000000`. The auditor rejects alternate or modified retail binaries. Commercial binaries and full disassembly remain private.

## Two new complete original native functions, 3,412 distinct bytes

| Native VA | Full extent | Exact SHA256 of complete native method | Source-bounded role |
|---|---:|---|---|
| **`0x2235BFD0`–`0x2235CCDB`** | **3,340 bytes** | `b927291398dd19d73f43fb0e2fbb842926b6008b90f0292645936ab5fafceebe` | Inherited Grunt NPC event `+0x444`; pre-event numeric update, 25-way table, original event-specific side effects, default forwarding |
| **`0x2202D930`–`0x2202D977`** | **72 bytes** | `969cc5659f53449812b567734994fbb656edc52fabffaa810dd15c0152f8f5a2` | Optional NPC member `+0xC18` event-consumption gate, original event `0x43` exception and shared NPC fallback |

The Grunt method's original code section ends at `0x2235CBDB`, followed by **100 original bytes of pointer-table data** (`0x2235CBDC..0x2235CC3F`) and **156 original bytes of byte-index table data** (`0x2235CC40..0x2235CCDB`). **256 table bytes are included in the one whole-method hash** and not counted as additional executable functions. The next four bytes at `0x2235CCDC` are genuine `INT3` alignment padding; the following method begins at `0x2235CCE0`. The intermediary method terminates with the original `ret 4` at `0x2202D975` and is followed by eight `INT3` bytes before the next function.

### Original Grunt event switch

Native entry `0x2235BFD0` unconditionally calls `0x22072100(owner+0x9E4,0.75f)` before reading event ID. The switch computes unsigned `event_id-3`, accepts the inclusive range **`0x03–0x9E`** (156 numeric IDs), then indexes a 156-byte table of case numbers into a 25-pointer dispatch table.

Exactly **26 numeric event IDs** choose 24 dedicated case numbers (two pairs share a case). The other **130 in-range event IDs** use case **24**, which points to original `0x2235C27F`:

| Case | Original event IDs | Original branch VA |
|---|---|---|
| 0 | `0x03` | `0x2235C14B` |
| 1 | `0x2B, 0x2C` | `0x2235C278` |
| 2 | `0x38, 0x3C` | `0x2235C41D` |
| 3 | `0x43` | `0x2235C58A` |
| 4 | `0x62` | `0x2235C465` |
| 5–10 | `0x8B–0x90` | Six different native branches |
| 11–23 | `0x92–0x9E` | Thirteen different native branches |
| 24 | All other in-range IDs | **`0x2235C27F`**, inherited event forwarding |

Out-of-range IDs also enter `0x2235C27F`. Crucially, **event `0x54` is in the default case**. It does not enter a Grunt-dedicated branch of this 3,340-byte method.

The source-verified default block at `0x2235C27F` calls original `0x2202D930` at `0x2235C282`. Another dedicated case can also call the intermediary at `0x2235C436`. The original direct Jetpack fallback `0x22364638 -> 0x2235BFD0` and another incoming Grunt override `0x22355519 -> 0x2235BFD0` are both authenticated. Original Grunt primary vtable `0x2249AAA4 +0x444` is verified to point to this exact method.

## Newly recovered member-level consumption policy: `0x2202D930`

Original function input: owner `ecx`, single event pointer stack argument, with original `ret 4`.

1. **Event ID equals `0x43`**: bypass the optional component entirely and invoke owner virtual **`+0x780(event)`**. Return immediately; the generic NPC handler is not called by this intermediary on this branch. This is a property of the intermediary, **not** a statement that the full Grunt parent selects its default case for `0x43`: the parent has its own dedicated case 3.
2. For any other event, read owner pointer **`+0xC18`**. If null, call original shared NPC dispatcher **`0x22050150(event)`**.
3. If nonnull, set the component's raw byte **`+0x08` to `1`**, invoke that component's virtual **`+0x60(event)`**, then check the component's **byte**, not the C++ virtual's return. A still-nonzero byte suppresses the generic dispatcher. If the component cleared that byte, call original `0x22050150(event)`.

This is an additional original dispatch decision *after* Phase591's attached-flying-entity filter. The fact that the Grunt's activity remapping also uses owner `+0xC18` is established in Phase587, but its concrete class identity and component `+0x60` method body are not yet recovered. Assigning a developer C++ type name to that component would be speculative.

### Exact `0x54` → `GRUNT_FLANK` chain and the remaining qualification

- Jetpack virtual `+0x444` at Phase590 sends unhandled outer event `0x54` to attached-flying filter `0x2238B3C0`. Phase591 proves the filter is false for `0x54` (its own table only covers `0x09–0x3C`).
- Jetpack then invokes inherited Grunt `0x2235BFD0`. The complete Grunt 156-entry table in this phase proves `0x54` maps to default case 24, which calls `0x2202D930`.
- The optional owner `+0xC18` component can **suppress** the event from reaching shared NPC `0x22050150`. If the component is null or explicitly clears its marker, the generic dispatcher receives the original event.
- Original shared NPC jump-table slot at `0x22052D14+4*(0x54-1)` points to native `0x2205259D`, which x87-truncates the event record's float field `+0x04`, restores the original control word and calls Grunt virtual `+0x5A8`.
- The Grunt vtable maps `+0x5A8 -> 0x2235AE30` (previously authenticated Phase565). **Inner integer 1** obtains native Grunt navigation route length from `0x2209D410`. Only **ordered strictly greater than `240.0f`** queues the original `GRUNT_FLANK` sentence with argument 0 / mode 2. Inner integer 0 calls another assault helper; other values do not trigger flank.

The original game's animation-model asset actually emitting `(event=0x54, float=1.0)` remains unidentified; the mere existence of these dispatch paths is not proof of playback in all game situations. The original component's consumption behavior is not yet reproduced beyond its call/marker contract. This is **not** complete source-equivalent NPC animation or a playable replacement DLL.

## Exact verification and independent host regressions

Read-only SHA auditor: `research/tools/audit_sin_original_grunt_event_parent_phase592.py`.

- Unchanged original DLL pair whole SHA256 pins, entire **3,340-byte** parent, entire **72-byte** intermediary, both true boundaries and padding: **PASS**.
- All **25 native case pointers**, all **156 index entries**, exactly 26 dedicated IDs / 130 in-range default IDs, and `0x54 -> 24 -> 0x2235C27F`: **PASS**.
- **100 original E8 rel32 direct calls** within the two new function extents and **two original incoming direct calls**: **PASS**. Every one of the 102 checked native five-byte E8 encodings is individually synthetic-mutated at every byte and rejected by self-test.
- **35 independent instruction/field witnesses**, Grunt `+0x444`/`+0x5A8` vtable pointers, original shared event `0x54` table word and route 240.0f constant: **PASS**.
- Independently authored header `include/sin_compat/original_grunt_event_parent_phase592.hpp` and `tests/test_original_grunt_event_parent_phase592.cpp` model the complete dispatch **partition** and optional component consumption mechanism. The source-specific dedicated branches use explicit unresolved callbacks instead of invented behavior. Host checks default/out-of-range, event `0x54`, component null, component marker retained or cleared, intermediary `0x43` exception, and all 156 case indices.
- Original-free tests: strict GCC/Clang C++17 and AddressSanitizer/UBSan **PASS**; CTest **2/2 PASS**. The host does not evaluate native x87 event-param conversion or actual animation source assets.

```bash
python3 research/tools/audit_sin_original_grunt_event_parent_phase592.py --self-test
python3 research/tools/audit_sin_original_grunt_event_parent_phase592.py --bin-zip /private/SE1/bin.zip
cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure
```

**Suggested next Grunt-specific target:** identify the concrete owner `+0xC18` component's implementation of virtual `+0x60`, alongside the activity delegate studied in Phase587, and determine which events it marks as consumed. Keep native animation asset enumeration as a separate task, not a guessed event name.