# Gameplay Phase598: SHA-locked original Grunt behavior vtable target probe

## Research focus and evidence boundary

The owner's Phase595 JSON confirms exact original 2006 server.dll SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650 and previously whole-method-authenticated event gate 0x2202D930. The Phase597 structural hypothesis identifies original NPC owner+0xC18 as the behavior-host *active behavior pointer* and behavior+0x08 as an interception marker. This plausible identity is still not directly proven by 2006 class RTTI for the pointer.

Phase535 already independently authenticated the original Grunt's **eight embedded behavior objects** and eight **final** RTTI-backed vtables. Phase587/592 also authenticated the original owner+0xC18 indirect call to its object's virtual byte offsets **+0x7C** (activity mapping) and **+0x60** (event interception). This phase links those earlier findings by reading the 16 native vtable pointers *only after* SHA and RTTI checks. It neither claims new original method decompilation nor exports any retail DLL bytes.

## Eight already-verified 2006 behavior class vtables

| Original RTTI class | NPC embedded offset | Exact final vtable |
| --- | --- | --- |
| CAI_RappelBehavior | +0xCFC | 0x223F956C |
| CAI_FuncTankBehavior | +0xCD8 | 0x224ABEC4 |
| CAI_TakecoverBehavior | +0xD20 | 0x224AE624 |
| CAI_BeserkBehavior | +0xDA0 | 0x224AB88C |
| CAI_ActBusyBehavior | +0xF88 | 0x224AB494 |
| CAI_AssaultBehavior | +0xDB8 | 0x223F5544 |
| CGruntStandoffBehavior | +0xDE8 | **0x22495344 (derived vtable)** |
| CAI_FollowBehavior | +0xEB8 | 0x223F6B7C |

The derived Standoff vtable is verified by Phase535 after its base class constructor; using only 0x223FA1B4 would analyze the wrong runtime handler.

## Reproduce on owner's local original files

The new source-specific no-dependencies Python script is:

research/tools/probe_sin_original_grunt_behavior_vtables_phase598.py

Use PowerShell:

    cd "D:\SiN-DisposableTest\SE1\bin"
    python .\probe_sin_original_grunt_behavior_vtables_phase598.py --self-test
    python .\probe_sin_original_grunt_behavior_vtables_phase598.py --server ".\server.dll" --client ".\client.dll" --out ".\grunt_behavior_phase598_targets.json"

Or provide --bin-zip pointing to the original ZIP with both DLLs. Running with --client authenticates the pair; --server alone still authenticates the original server.

The script refuses an unsupported full-file SHA; verifies PE32 image base, authenticates the original intermediary's 72-byte SHA, confirms the original Grunt event handler vslot, all eight original MSVC RTTI class identities and original owner-binding virtual +0x04. Only then does it read native +0x60 and +0x7C virtual pointers and group duplicates. The output includes no proprietary code bytes or assembly.

## Expected utility, not yet claimed

The generated JSON will contain actual native target addresses under all eight existing 2006 behavior vtables. These entries supply source-specific *candidate method targets* for precise future SHA-bounded function reconstruction. Shared targets may be inherited common handlers, while distinct targets may be gameplay specializations. Vtable slot addresses **do not prove** that owner+0xC18 points to any specific behavior at a particular event, and do not prove that any specific behavior clears marker+0x08 for event 0x54.

**Original retail execution status:** The owner has not yet provided the Phase598 output. No 16 slot values, exact native function boundaries, handler semantics or new complete method bytes are claimed in this phase. The next step is to inspect owner-generated report, rank addresses, and source-authenticate the relevant original event bodies.

This scope explicitly excludes HL2-equivalent surfaces, models, prop rendering, physics, native PC replacement DLL boot and Android/Quest 3 integration.
