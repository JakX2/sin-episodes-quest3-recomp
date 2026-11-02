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
