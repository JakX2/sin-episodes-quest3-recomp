# Phase637: Source-authenticated original SiN LOCALID registration and the "task" wrapper

## Goal and verified retail binary scope

The prior Phase635 recovered the complete original identifier mapper `0x22073700`; Phase636 recovered three complete node constructors and owner registration. Constructors produce a 24-byte list-node sentinel, but **do not insert a usable LOCALID range**. This phase authenticates **the separately original range-population method** and one proven caller.

Exact owner-supplied 2006 retail PE32 x86 `server.dll` SHA256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. Companion `client.dll` SHA256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

| Original native executable method | Entire bytes | Full original method SHA256 |
| --- | ---: | --- |
| `0x22073760` original source/translated LOCALID range registrar | **217** | `31a68dbb81e3cdd2ea8ac037a1feb1c99028c6b1ab39b61dd663bd1798361356` |
| `0x22088FA0` mapping-owner `"task"` ID registrar wrapper | **31** | `a582842fd9f4d3c807f486c6f2f0cb8bf5c3074c1337459399c230c68a1150ed` |

**248 additional complete original native method bytes, two complete methods**, source-verified against the private retail binary. The C++17 source in this PR independently models observed decisions and source-ordered effects; it does not copy original binary bytes into GitHub or call the original engine.

## Native range registration `0x22073760`: exactly what it does

**Original __thiscall arguments:** `ECX = 24-byte range node`; `[ESP+4] = symbol/name/source native DWORD`, `[ESP+8] = source LOCALID DWORD`, `[ESP+0xC] = diagnostic category text pointer`, `[ESP+0x10] = diagnostic display-name text pointer`. Original `RET 0x10`.

1. **Resource must be initialized.** Read original `node+0x14` source-record pointer. If null, log `ERROR: Adding symbol to uninitialized table %s` with the original display-name string and return native false. No prior write.
2. **First LOCALID, special source sentinel `9999`.** On `node+4 == 9999`, load successor from `node+0x10`, **write `node+4 = localID` before inspecting successor**, then, if successor exists and `successor+4 != 9999`, reject with `ERROR: Bad %s LOCALID for %s` when **signed `localID <= successor+4`**. This early rejection preserves the already-written low bound, which is an original partial side effect.
3. **Reject IDs below current range start.** Re-read `node+4`. If signed `localID < node+4`, report `ERROR: %s First added %s must be first LOCALID!` and return false. This does not undo any earlier write.
4. **Establish or extend upper bound.** If `node+8` is native `-1`, set `node+8 = node+4`, then `node+0xC = node+0`. Otherwise if signed `localID > node+8`, first compute mapped upper `node+0 - node+4 + localID` using native 32-bit wraparound, then write `node+8 = localID` and `node+0xC = mapped_upper`. Otherwise keep the bounds unchanged.
5. **Convert through the actual Phase635 `0x22073700` algorithm**, called from native `0x2207380E`. Original converter returns the corresponding mapped index, or numeric `-1`. A `-1` causes the registrar to return **success** without forwarding the symbol or incrementing the source-record count.
6. **Forward the mapped symbol through original `0x22282370`.** On a mapped index, source code reads the resource context pointer from `[node+0x14]->[0]`; passes the original source-symbol DWORD and mapped index to original `0x22282370` at callsite `0x22073822`. This helper is *not* yet reconstructed: a typed host callback owns the call and may reject it. The model does not pretend forwarding is a no-op.
7. **Update the resource record after forwarding.** The original **re-reads** `[source_record+4]` after the engine helper returns; computes mapped index **`+1` modulo x86 DWORD**, and if the signed count is smaller than this value, overwrites `source_record+4`. This write is strictly ordered **after** the potentially irreversible symbol-forwarding call.
8. On all successful paths, returns native true.

The original does not check for null `source_record->context`, invalid native memory, pointer arithmetic overflow or reentrant callbacks. The portable model marks unsafe/null conditions explicitly, refuses host callback/exception failures and preserves all earlier committed host writes with no invented rollback.

Three original diagnostics and the literal `"task"` were authenticated as exact bytes from retail `.rdata`; no format strings were guessed.

## Important wrapper distinction: `"task"` is NOT the Elite event/flag node

Original complete wrapper `0x22088FA0` takes `this=owner` and **three** parameters, passing the following four parameters to original `0x22073760`:

- Original incoming symbol pointer, unchanged.
- Incoming local ID, unchanged.
- Original static category string pointer `0x223EB17C` containing **`task`**.
- Incoming display-name pointer, unchanged.

Before the call, it computes **`ECX=owner+0x1C`** and then returns with `RET 0x0C`. This is the **second** node in Phase636's three embedded list nodes. The original Phase634 Elite animation-event/flag functions instead translate using **`owner+0x34`**, the third node. They may share the registrar algorithm, but this exact wrapper does **not** prove that task registration populates the animation-event mapping. Treating source `"task"` IDs as Elite animation-event IDs would be an unsupported claim.

### Additional confirmed Elite startup call: `"schedule"`, owner+0x04

The original retail Elite startup handler has a **separate, direct** call to this very same registrar, `0x22355B3A → 0x22073760`. Its predecessor instructions at `0x22355B2D..0x22355B35` supply the raw 8-byte-per-row symbol and LOCALID entries, diagnostic display-name pointer from `[0x22534698]`, exact ASCII source label `"schedule"` at `0x223E5F78`, and **`ECX=0x2259C2FC`**, which is original Elite mapping owner `0x2259C2F8 + 0x04`. The next instruction tests `AL` and takes an error branch if the registrar returns false.

This is **the first embedded schedule mapping node**, distinct from the exact original task wrapper at owner `+0x1C` and the Elite animation-event/flag mapper at owner `+0x34`. It is verified as a *native caller edge and original argument witnesses*, **not** counted as an additional complete method, since the larger containing Elite startup method is not reconstructed here. A separate Phase637 test sends an invented schedule row through the same already authenticated registrar and the real compiled Phase635 mapper; its inputs are explicitly synthetic, not extracted retail records.

### Remaining original executable dependencies

The registrar's source-authenticated direct calls are:
- `0x22088FB7 → 0x22073760`: task wrapper.
- `0x22355B3A → 0x22073760`: original Elite startup schedule registration with `ECX=owner+0x04`.
- `0x2207380E → 0x22073700`: already recovered 58-byte ID converter.
- `0x22073822 → 0x22282370`: original engine symbol registration side effect, still an explicit host dependency.

The native three-error reporting indirect calls through `[0x223D214C]` are also authenticated opcode witnesses, not disguised as C++ exceptions.

## Implemented and tested gameplay references

- `include/sin_compat/original_elite_range_registration_phase637.hpp`: exact method and wrapper VAs, task/animation node offsets, original source arguments, diagnostic categories, typed host memory/mapping/forward interface and results with write provenance.
- `src/original_elite_range_registration_phase637.cpp`: source-ordered sentinel handling, ordering diagnostics, signed bounds, source/mapped high writes, exact Phase635 callback sequence, original symbol-forwarding callback, signed post-forward count update and safe wrapper address calculation.
- `tests/test_original_elite_range_registration_phase637.cpp`: construct a real source-referenced node through Phase636's `NodeInitializer` and `NodeLinker`, then use the **actual independently compiled Phase635 `Reference` mapping algorithm** on the mutated node. Verify first ID transition `9999→3`, upper-bound extension `3→4`, repeated registered ID, mapped high, resource max, diagnostic contents and absence of error-side rollback. Exercise null source records, successor ordering with an irreversible first low write, source below low, unmapped ID `-1`, 32-bit wrap, symbol-forward refusals, partial writes from invalid source pointer/host exceptions and reentrancy.
- `research/tools/audit_original_elite_range_registration_phase637.py`: owner-only exact original DLL SHA256 identity, SHA256 of **both complete** method bodies, three rel32 edges, separately pinned source x86 instruction witnesses, original `"task"` label and all three original diagnostics plus `"task"` and `"schedule"` in `.rdata`. Original-free `--self-test` mutates invented full bodies/calls and probes instruction/literal mutation refusals.

The test's source-record pointer and symbol values are **fabricated**, deliberately. Only the control flow and original field addresses are source proven. This is not an original in-game test, nor proof that the retail game fills the specific artificial nodes from the tests.

### Owner-side original verification command

```powershell
python research\tools\audit_original_elite_range_registration_phase637.py `
  --server "D:\SiN-Original\server.dll" `
  --client "D:\SiN-Original\client.dll"
```

## Next research priorities

1. **Find and audit the original registration callsites for the third Elite node `owner+0x34`**, not only this category `task` at `owner+0x1C`. Confirm the corresponding symbol source and literal category.
2. Recover the original `0x22282370` symbol-forwarding engine callback or trace its effects with a read-only owner process debugger. Without it, even an exact local-ID mapping cannot fully register original symbols.
3. Add an authenticated, read-only, owner-only Windows process probe of the BSS mapping owner after startup. Original owner `0x2259C2F8` and node `0x2259C32C` were previously source verified; actual populated map values have not yet been observed.

No unrelated Half-Life 2-equivalent props, materials, models, physics, surfaces or rendering are touched. No original Windows game has called our reconstructed C++, and Android/Quest 3 in-level gameplay is not claimed.
