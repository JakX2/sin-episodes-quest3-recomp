# Original SiN Episodes 2006: whole client.dll/server.dll code-entry census

**Source**: user-owned original 2006 x86 SiN Episodes: Emergence Windows PE32 client.dll SHA-256 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d, and server.dll SHA-256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. Their original binaries remain private, not committed to GitHub. This is actual game-binary analysis, not map/APK loading.

## Broad code evidence, not only selected game classes

The census combines previous verified 2006 RTTI/vtable function pointers with a new **whole-.text GNU objdump** pass extracting **CALL rel32 destination addresses**. It also counts indirect CALLs and branches, while storing direct jump evidence and .rdata literal hints separately. This is the first whole original-client and original-server static code-index pass in the project.

| Code evidence | client.dll | server.dll |
|---|---:|---:|
| .text raw bytes examined | 2,576,384 | 4,001,792 |
| Linearly decoded instruction-like entries | 949,834 | 1,427,848 |
| Classes with validated RTTI hierarchy | 797 | 1,485 |
| Validated RTTI-associated vtables | 1,428 | 1,750 |
| Unique virtual function targets | 5,919 | 8,664 |
| Unique direct CALL destination addresses | 6,099 | 9,579 |
| Virtual/direct-CALL target overlaps | 214 | 435 |
| **Combined entrypoint candidates** | **11,804** | **17,808** |
| Direct CALL sites into executable .text | 36,845 | 61,913 |
| Indirect CALL sites | 17,893 | 22,902 |
| Branch instructions | 57,604 | 99,452 |
| Direct JMP sites into executable .text | 10,284 | 15,159 |

**Combined potential code entrypoints across both DLLs: 29,612.** These are NOT 29,612 recovered C++ methods. Most of them have no validated function body or source-level meaning. Objdump's linear decoder can parse embedded code/data incorrectly, and an instruction CALL target may be a compiler thunk or an incorrectly decoded address. Indirect and data-address-only calls remain to be recovered, and many virtuals are inherited. Zero claim of full decompilation.

The dedicated full CSV index lists all candidate x86 addresses, how they were discovered, RTTI class hints (when present), inbound direct CALL counts, and nearby string prioritization **hints**, but not guessed function identities. The authoritative recovery status is **candidate_only_no_function_boundary_or_C++_semantics** unless a separate original-machine-code audit has established more.

## Concrete original client/server systems now visible in the recovered class graphs

| System | Original client | Original server |
|---|---|---|
| Dynamic difficulty | C_DynamicDifficulty (12 virtual slots) | CDynamicDifficulty (1 slot), CDynamicDifficultyAction (11), CTriggerDynamicDifficulty (167) |
| SinTEK grunt / enemies | C_NPC_GruntBase | CNPC_GruntBase, CNPC_SintekGrunt, CNPC_SintekJetpack, CNPC_Quadralex |
| Weapons | C_WeaponMagnum, C_WeaponAssaultRifle, C_WeaponScatterGun | CWeaponMagnum, CWeaponAssaultRifle, CWeaponScatterGun |
| Grenades | C_BaseGrenade; no specific CGrenadeAR class descriptor | CGrenadeAR : CBaseGrenade with original Detonate override |
| Poison/antigen | CHudPoisonTimer | CPoisonGas, CAntigenDispenser, CAntigenCanister |
| Phone | C_PropPhone | CPropPhone |
| Vehicles | C_PropSinVehicle | CPropSinVehicle and vehicle physics classes |
| Player prediction | CPrediction and client movement | Server game player/weapon state |

Do not conclude that a class with a small vtable has little implementation: much code is nonvirtual or shared through helper routines. Nor does a client/server class name match prove matching class layout or network wire protocol.

## Reproduce and audit

Run the newly committed pure Python (standard library) source and GNU binutils against the *exact original user-owned binaries*, after regenerating the matching original RTTI inventory using the previous PR #66 RTTI scanner:

    py research/tools/audit_sin_entire_dll_code_census.py --client "C:\SiN\bin\client.dll" --server "C:\SiN\bin\server.dll" --rtti "C:\private\sin-rtti-vtables.json" --output-dir "C:\private\sin-whole-code"

The script supports both previously used RTTI JSON formats, with either vftable functions or function_va arrays. It checks both DLL fingerprints and the matching RTTI fingerprint before making any code claims. Every candidate is in the original x86 executable region. Synthetic x86 test cases check direct/indirect calls, virtual overlap, branch-only exclusion and actual .rdata string pointer validation.

## Work plan toward all gameplay recovery

Maintain **separate completion states** per original address, with confidence and proof:
1. Original entry address found (vtable / direct call / pointer).
2. Verified exact function body and control-flow graph, disambiguating thunks, jump tables, indirect calls and embedded data.
3. Understood original inputs, outputs, owner class, script usage, network field layout and observable behavior.
4. Independently implemented clean-room C++ in the *verified real* ARM64 SourceVR server/client interfaces.
5. Validated in live game against original SiN behavior using the user's game assets and supported maps.

The largest immediate opportunities are **client presentation, prediction and dynamic difficulty** (underserved by our prior server-centered grenade focus), then the corresponding server difficulty logic, grunt AI schedules/poison/antigen state, weapon firing, phone interactions and effects.

Previous findings in PRs #63-70 still apply: 510 server entity registrations; original versioned engine interfaces; network tables; original weapon/combat/phone/vehicle scripts; grunt/phone/Quadralex function addresses; grenade creation, Detonate, blast damage, AI warning and explosion combat sound. Existing tools and verified semantics should be reused rather than rewritten.

**Reality check:** This milestone inventories executable x86 code, but the original C++ source is not fully decompiled and no complete SiN-compatible Quest ARM64 gameplay client.so/server.so is running. The original x86 code cannot simply be loaded or relinked in Android.

**GitHub backup:** source analyzers, synthetic tests, notes and PRs live on remote branches. The owner should retain local backups of original DLLs, scripts, the full generated machine-readable candidate lists and their exact hashes. No merges are implied by draft PR status.


## Immediate cross-DLL lead: original dynamic-difficulty code

First use of the whole-DLL inventory on this *SiN-specific* subsystem, verified independently against the original compiled RTTI/vftables and original 2006 .rdata strings (not inferred from modern HL2):

**Client:**
- **C_DynamicDifficulty : CBaseGameSystem** has 12 primary vtable slots, but only **2 differ** from CBaseGameSystem.
- **Corrected by the actual disassembly (PR #72):** slot **0** has child 0x241FAC90, which registers original user-message hooks via 0x241CFF70, and slot **11** has child 0x241FAC70, which follows a deleting-destructor path. The earlier tentative destructor label for slot 0 was backwards. Their original parent addresses were 0x240566C0 and 0x2413D2B0, respectively.

**Server:**
- **CDynamicDifficulty** has only **one RTTI-identified virtual slot** (0x222B9050). This means the vtable census alone is *especially inadequate* for reconstructing its manager logic; nonvirtual methods and global/static references must be traced separately.
- **CDynamicDifficultyAction : CEntityAction**: 11 shared slots, **9 changed**. Replacement original virtual entry addresses by slot: 0=0x2215C6A0, 1=0x2215C680, 3=0x2215C6C0, 5=0x2215C690, 6=0x2215BD00, 7=0x2215BD10, 8=0x2215BD30, 9=0x2215D090, 10=0x2215BDD0. Several parent slots were generic/pure-virtual stubs, so the action subclass carries substantive behavior candidates.
- **CTriggerDynamicDifficulty : CBaseTrigger**: 167 shared slots, **4 changed**. Replacement entries: slot 0=0x221D5510, slot 13=0x221CCB60, slot 25=0x221D0740, slot 89=0x221CCB70. Do not assume these slots' method names without callsite disassembly.
- Original server literal strings include **dynamic_difficulty**, **Dynamic Difficulty Action**, **EstimatedDifficulty**, **m_estimatedDifficulty**, **m_expectedDifficulty**, **g_debug_dynamic_difficulty**, and **Dynamic Difficulty Mode: No saving allowed**. String presence alone is only data evidence, but original machine-code PUSH-imm32 references are also present:
  - 0x2215D6CC points at original dynamic_difficulty literal 0x2241129C.
  - 0x2215D39C points at original Dynamic Difficulty Action literal 0x2241130C.
  - 0x223C2D77 points at original debug cvar literal 0x2245CDBC.
  - 0x223C2E2B points at original No saving allowed diagnostic 0x2245CE4C.
  - 0x222B687F and 0x222B737D point at original dynamic_difficulty literal 0x2245C8D4.

These addresses are **next original function-disassembly targets**. This does not yet establish calculation formulas, save restrictions, client/server message semantics or gameplay difficulty changes. Follow the executable CALL sites and field reads/writes before naming original methods or porting behavior.

**Why this is a better next implementation priority:** dynamic difficulty appears across both binaries, and the server's main controller mostly lives outside obvious virtual slots. Reconstructing it links player performance, AI, grenade damage and client presentation rather than merely adding another isolated projectile.
