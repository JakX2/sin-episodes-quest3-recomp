# Gameplay Phase 595: Original Grunt +0xC18 component provenance probe

## Status: probe implemented; retail evidence not yet collected

Phase 592 verified that native `0x2202D930` obtains the Grunt's optional member at owner `+0xC18`, initializes that object's marker byte `+0x08=1`, invokes its vtable virtual `+0x60(event)`, and forwards to shared NPC handling only when the post-call marker is zero. Phase 594 now regression-tests this gate across Grunt, Elite Grunt, and Jetpack Grunt event `0x54` paths.

**The remaining native unknown is the concrete component's allocation/assignment and virtual +0x60 implementation.** This phase adds the specialized read-only Python inspector `research/tools/probe_sin_original_grunt_component_c18_phase595.py` to collect clues from the untouched owner-private original 2006 `server.dll`. No retail original has been executed against the new probe in this session, and no original code bytes or new methods are claimed authenticated.

## Authentication before scanning

The probe requires original full-file SHA-256:

- `server.dll`: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- Optional but recommended paired `client.dll`: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- Original server PE32 x86 base `0x22000000`.
- Previously authenticated complete 72-byte method `0x2202D930`: `969cc5659f53449812b567734994fbb656edc52fabffaa810dd15c0152f8f5a2`.

Modified or mismatched inputs are rejected, not silently scanned as retail.

## What the probe produces

A JSON report with original addresses only, not assembly or DLL bytes:

1. Candidate x86 `mod=10` memory read/write/LEA/other opcode shapes using exact signed displacement `+0xC18`, annotated with candidate instruction VA, base register, and whether the shape *could* assign a pointer.
2. Candidate `E8 rel32` callers directly targeting the **already verified** `0x2202D930` gate.
3. Counts, strict input hashes, source-method authentication evidence, and explicit Boolean warnings that **no instruction boundaries, owner-class identity or concrete virtual +0x60 implementation are proven** by this scanner.

It is deliberately a *candidate encoder search*, not a full x86 disassembler. Other Source classes, including the attached flying entity whose own `+0xC18` is a vector component, can share the offset. Pattern matches and even writes must be examined in Ghidra with their containing function, receiver provenance and constructor/destructor cross-references. The scanner cannot by itself assign a C++ type.

## Reproduce on the owner's machine

```powershell
python research/tools/probe_sin_original_grunt_component_c18_phase595.py --self-test
python research/tools/probe_sin_original_grunt_component_c18_phase595.py --bin-zip "C:\private\SE1\bin.zip" --out "grunt_c18_phase595_candidates.json"
```

Or use `--server "C:\path\to\server.dll"` for server-only SHA verification. The report contains VAs/metadata only. Keep the private retail files local; **do not commit or upload either DLL to the public repository**.

The pure synthetic fixture tests representative register-read, register-write, SIB-stack-use, immediate-write and invalid-displacement matches; a synthetic `E8` caller, PE32 section mapping, and exact-original hash refusal.

## Ghidra handoff: discriminate real Grunt assignment from false positives

Prioritize `could_assign_pointer=true` records and inspect their containing functions. For each plausible candidate:

1. Establish that the destination base register really points to **the Grunt object** and not another entity with an unrelated `+0xC18` field.
2. Trace the stored pointer backward to allocation or factory acquisition, constructor and RTTI/vtable initialization; record raw evidence and class name only if actually recovered.
3. Resolve the resulting vtable's entry at byte offset **`+0x60`**, then its exact method boundary and hash. Trace writes to the component marker `+0x08`, especially branches conditioned on animation event `0x54`.
4. Verify whether this method clears the marker for `0x54`; only then can we say that the original component permits or suppresses the shared event in each real state.
5. Correlate findings with original callers of `0x2202D930` and the Grunt parent `0x2235BFD0`.

**Separate pending target:** original Elite Grunt event `0x47` dependency `0x22355300` / `0x22355370`.

This is a source-recovery instrument, not a gameplay implementation, Windows game-DLL replacement, Android port or Quest 3 runtime.
