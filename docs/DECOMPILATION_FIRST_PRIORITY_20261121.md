# Decompilation-first checkpoint (owner priority, 2026-11-21)

## Immediate project objective

**Recover clear, source-level behavior for the exact original 2006
`server.dll` and `client.dll` before prioritizing playable Windows/Android/VR
ports.** Preserve already working HL2 map, model, prop and physics coverage;
do not spend cycles reimplementing those systems unless new SiN-specific
evidence makes a particular difference necessary.

This is a **priority order**, not a claim that currently written C++
compiles into the retail game. Continue investigating both game DLLs, not
only the Grunt; the `client.dll` initialization and HUD, player interfaces,
server entity/network tables, player weapons, AI, game rules, dialogue,
scripted encounters, save/restore and SiN-specific objects all matter.

## Rules for recording real decompilation progress

1. **Original identity:** verify entire file SHA-256 and original image base,
   record function VA (and RVA where appropriate), instruction extent,
   complete-method SHA-256, vtable or call-site identity, and RTTI when found.
2. **Real control flow:** reconstruct the original branches, return values,
   side effects, calling convention and concrete field offsets. Verify
   original jumps, calls and constants, not merely strings or plausible code.
3. **Separate proof levels:** *exact authenticated machine code* vs *readable
   pseudocode with unknown dependencies* vs *independent host C++ model* vs
   *original-engine validated runtime*. Neither a synthetic CTest nor a
   retailer run with original DLLs is proof that a replacement DLL works.
4. **Preserve unknowns:** label original numeric condition/schedule IDs with
   names only when binary registration or comparable direct evidence proves
   the mapping. Do not graft familiar Half-Life 2 implementations into SiN
   just to fill gaps.
5. **Evidence and reproducibility:** source-readable report + original
   function SHA, an offline auditor with synthetic corruption tests, and an
   xref/dependency list. Never commit original proprietary PE32 DLL bytes,
   assets or raw full disassembly.
6. **Measure coverage honestly:** count independently authenticated original
   function boundaries and bytes, group by `client.dll` vs `server.dll`, and
   report unresolved helper calls/branches. Don't count research phase numbers
   as percent complete, and don't add new functions twice if re-audited.

## Decompilation tracks (parallel when possible)

| Track | Immediate scope | Main completion gate |
| --- | --- | --- |
| Server: gameplay entities | NPC factories, spawn, think, ownership, save/restore, entity inputs | Readable original methods and native field contracts linked |
| Server: AI and weapons | Grunt, SinTEK/other NPCs, schedule/tasks, weapons, difficulty | Decompile meaningful whole call trees, not isolated return constants |
| Client: startup and gameplay | Original `VClient011` Init and follow-up lifecycle, HUD, user messages, recv tables, player state | Complete recovered call-order and state mutation, with original ABI metadata |
| Cross-DLL engine boundary | `ServerGameDLL003`, `ServerGameEnts001`, `VClient011`, network send/recv classes | Exact interface methods and data contracts documented |
| Source-level assembly | Merge verified pseudocode into coherently named source modules | Every required dependency either decompiled or explicitly unresolved |

## Current checkpoint

Phase 537 demonstrates the intended method: from a known original vtable
entry through a 738-byte SHA-authenticated combat fallback (26 direct calls),
plus its 206-byte side-effect helper, their original-condition branches,
schedule return IDs, position tests and proven registry names.
A reproducer is `research/tools/audit_sin_original_grunt_fallback_844.py`,
and the readable decompilation is in
`research/phases/2026-11-21-original-mobgrunt-combat-fallback-844-decomp.md`.

**The ports remain later milestones.** Retain the existing Windows/Android
compile/test work as regression infrastructure, but direct new investigative
effort toward recovering the original DLL logic and its dependencies first.
## Phase 538: original client.dll startup decompilation

A second track now authenticates the **complete 1,220-byte original VClient011::Init method** at client VA `0x240FE0E0` and decompiles the first ordered engine-interface acquisition phase. Its **21 required original factory queries** all jump to the same failure return when null. A separate optional-style material stub query and two additional derived interface pointers have distinct null semantics. See `research/phases/2026-11-21-original-client-vclient011-init-interface-decomp.md` and `research/tools/audit_sin_original_client_init_interface_manifest.py`.

As with the server Phase537 report, authenticated whole-method bytes must **not** be misread as a fully decompiled 1,220-byte source function; the post-interface initialization tail remains unresolved. Research priority is now parallel **server gameplay decompilation and original client lifecycle decompilation**, ahead of new port shims.

## Phase 539: client Init registration and game-system lifecycle

Original client `VClient011::Init` tail now has directly verified calls and readable control-flow analysis for five complete original helper functions, an additional **525 authenticated original function bytes**. The `Init` method's original 1,220 bytes were authenticated in Phase538 and are **not counted again**. This phase distinguishes a 54-byte linked interface resolver with failure cleanup, a 23-byte dynamic-array registration adapter, a 277-byte game-system registration merge and initializer, a 97-byte second-factory interface preflight, and a 74-byte client-owned object allocation helper. It also verifies the twelve original game-system registrations and five post-interface failure gates. See `research/phases/2026-11-21-original-client-init-tail-and-game-system-lifecycle.md`. Source-like pseudocode remains a **partial annotated decompilation**, not exact original C++ or a running replacement client.

The second factory's interface queries are recorded solely as ABI dependencies. No implementation effort is authorized on physics, surfaces, models or props in place of the user's existing HL2 assets. Continue **game-specific DLL code recovery**.

## Phase 540: original client VGUI/HUD bootstrap

The original `VClient011::Init` calls `0x241EDFB0`, which SHA-authenticated original instructions show is a required VGUI startup helper. It requires two original interfaces `VGUI_InputInternal001` and `VGUI_System010` after a `CLIENT`-labeled prerequisite, executes a once-only `CHudTextureHandle` registration path, calls an actual one-instruction `ret` function, then returns the truthiness of a distinct native virtual `+0x04` call with argument `2`. Four complete additional method bodies total **239 original bytes**, separate from the 525 bytes in Phase539. See `research/phases/2026-11-21-original-client-vgui-hud-bootstrap-decomp.md` and its SHA-pinned self-testing verifier.

This decompiles program flow only; it does not implement a VGUI/HUD renderer, physics system, Android runtime or Quest port.

## Phase 541: original client VGUI interface resolver
The 586-byte 2006 `client.dll` function `0x240010D0` is SHA-authenticated
against both unchanged original DLL hashes. It independently scans a pointer
array of `CreateInterface` providers for eight original interface names,
stores eight native client globals, dispatches a virtual +0x04 on the primary
VGUI object with original provider array and count, then explicitly requires
all eight resulting globals. The virtual return is *not* a Boolean success
gate. Readable C-like decompilation and source-level proofs:
`research/phases/2026-11-21-original-client-vgui-factory-array-resolver-decomp.md`.
Full intro-prologue semantics remain unresolved; no new port/runtime work.

## Phase 542: original client type converter registry
The original HUD bootstrap's label `CHudTextureHandle` refers to a **type
converter registration** (not a drawn HUD panel). New SHA-verified native
methods: 261-byte string-token lookup/intern `0x241D4A80`, 230-byte
converter-linked-record query `0x2400A4D0`, and 128-byte record insertion
`0x24009F40` (total **619 new distinct complete bytes**). The method uses
a 16-bit token, `0xFFFF` missing-token sentinel, 24-byte record stride
and `0xFFFFFFFF` absent-converter index. The original diagnostic is
`Already have converter for type %s, ignoring...`.
Detailed source-level documentation and owner-local verifier:
`research/phases/2026-11-21-original-client-hud-type-converter-registry-decomp.md`.
The original comparator and intern pool ownership remain unresolved.
