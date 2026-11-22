# Gameplay Phase601: complete original Grunt behavior event handlers and event 0x54

## Authentic original 2006 binaries

This research directly examines the user's 2006 SiN Episodes: Emergence server.dll and client.dll, recovered from their connected Google Drive archive SE1/bin.zip. Binary hashes are checked before any source assertions:

- server SHA-256: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650
- client SHA-256: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d

Original commercial DLLs and raw disassembly dumps are **not committed or distributed**. Eight original MSVC RTTI-verified Grunt behavior vtables from Phase535 and their virtual+0x60 native pointers from Phase598 provide the actual entry addresses. The original Grunt optional event gate 0x2202D930 remains a completely verified 72-byte native function from Phase592, digest 969cc5659f53449812b567734994fbb656edc52fabffaa810dd15c0152f8f5a2.

## Nine complete source-authenticated native functions: 7,925 bytes

| Original RTTI behavior | Original +0x60 method | Length (bytes) | Exact original whole-body SHA256 |
| --- | --- | ---: | --- |
| CAI_RappelBehavior | 0x220DEA60 | 387 | eff73b12af6f7b712a89cc88d56a1f27fd12cddb9e8b22cc6b2dbe68a1ec3446 |
| CAI_FuncTankBehavior | 0x22392970 | 1,128 | fed2eda6f89634f075fd61cda6189a04f0dec3675c7a64bcf98ced101f3cdf3b |
| CAI_TakecoverBehavior | 0x22399650 | 840 | db6b11082b545915b8cc97007056bed4fe71a5f362731d00cfa1f25f63545d29 |
| CAI_BeserkBehavior | 0x22391C90 | 5 | d9e092b8790db44da7e09f1e51e2fbc250ed845d6cdd63e39bb92d8a7a820932 |
| CAI_ActBusyBehavior | 0x2238EB40 | 1,460 | 7de85ad368cc51b040842a38460eeca8c56a3f10e94b3e16cdd1f542d074d09e |
| CAI_AssaultBehavior | 0x220CF190 | 1,492 | 7b91ac5bca99404555e57dc9aa7de45850fc0fd5c0934aab996d3e172c03057a |
| CGruntStandoffBehavior | 0x220E0290 | 738 | dd6a9103d73cd60fa0c0ab87219dd860f08a9a02467d5ecb382dfe793953b5cc |
| CAI_FollowBehavior | 0x220D6FF0 | **1,868** | c1a882ec7bf3e7e506a162eb7f1507776fdcab066a3055112550e62717ee5491 |
| Shared original default event handler | **0x220CD2F0** | **7** | 1823d20cb51f1dda02bb824b26fb3f0b5d377c30ac6c844f2419d5cd71db5186 |

These extents are checked by complete SHA256, actual 2006 vtable pointers and 4+ original INT3 bytes following each method. Original dispatch tables included in method whole-function hashes: FuncTank has 7 pointer entries, Takecover 6, ActBusy 6, Assault 6, Follow 6. The auditor validates **31 native pointer-table entries** and refuses targets outside the corresponding complete method.

**Critical Follow boundary correction:** the Follow event method ends at exclusive VA 0x220D773C. There are four authentic INT3 bytes at 0x220D773C–0x220D773F; 0x220D7740 begins a SEPARATE method. Do not erroneously absorb this adjacent function into the preceding handler. The Follow function's correct hash is for 1,868 original bytes, including its own six-entry dispatch table.

## Newly recovered default event method

The authentic original seven-byte handler 0x220CD2F0 stores zero at behavior+0x08 and returns after consuming its four-byte event argument. This makes it a real marker-reset/default-forwarding component of the original 2006 Grunt event chain. A nearby but distinct 0x220CD2E0 method also clears +0x08; these two native function boundaries are not interchangeable.

### Actual original event 0x54 fallback callsites

| Original behavior | Method event fallback site | Native call form |
| --- | --- | --- |
| Rappel | 0x220DEAA7 | direct E8 CALL |
| FuncTank | 0x22392DA9 | direct E8 CALL |
| Takecover | 0x2239996D | direct E8 CALL |
| Beserk | 0x22391C90 | **direct E9 tail JMP** |
| ActBusy | 0x2238F0CE | direct E8 CALL |
| Assault | 0x220CF73B | direct E8 CALL |
| GruntStandoff | 0x220E0565 | direct E8 CALL |
| Follow | 0x220D7713 | direct E8 CALL |

The independent source verifier checks the true original rel32 CALL/JMP encodings and target 0x220CD2F0 at all eight sites. It separately verifies original branch opcodes to identify the event-ID partition.

Event **0x54**, decimal 84, reaches this default fallback in **all eight** classes:
- Rappel distinguishes raw 0x0D and special 0x186A0-series event IDs. Event 0x54 is default.
- FuncTank handles seven entries indexed from 0x186A0; unsigned out-of-range event 0x54 is default.
- Takecover and ActBusy each handle six entries indexed from 0x186A0; event 0x54 is default.
- Beserk unconditionally tail-jumps to default.
- Assault has original raw 0x43 and 0x186A0-series branches; event 0x54 falls through to default.
- GruntStandoff distinguishes raw 0x43 and 0x62. Event 0x54 is default.
- Follow distinguishes raw 0x43/0x62 and multiple 0x186A0-series codes. Event 0x54 is default.

These are source-authenticated observations **for event 0x54**. They do not reconstruct every other event path or the side effects of all special cases.

## Original Grunt chain is now more complete

Original event gate 0x2202D930 reads owner+0xC18, sets pointed object's byte+0x08 to one, invokes that object's vtable virtual +0x60, then tests that byte. If it is zero, shared native NPC handler 0x22050150 receives the event.

For **any of the eight authenticated original Grunt behavior objects**, the original +0x60 handler on **event 0x54** reaches 0x220CD2F0 and clears +0x08. If owner+0xC18 is null, the same shared handler is called directly. Thus the optional active-behavior gate does not block 0x54 for any of these eight original Grunt behavior subobjects.

Phase592/565 previously authenticated shared NPC event 0x54 as dispatching the Grunt numeric flank path after x87-truncation of the event float. Inner integer **1** still requires real navigation route distance **strictly greater than 240.0f** before queuing GRUNT_FLANK sentence. These separate requirements and exact animation asset emitter remain unverified for real in-game playback on every Grunt instance.

The conceptual 2006 developer member name for owner+0xC18 remains a structural inference (likely current active behavior), and it is NOT proven that this pointer can never refer to a ninth possible subclass. Likewise the marker+0x08 can be described by its verified fallback/override behavior but the historical C++ declaration name is not recovered.

## Reproduction and validation

Read-only auditor: research/tools/audit_sin_original_grunt_behavior_event54_phase601.py

    python research/tools/audit_sin_original_grunt_behavior_event54_phase601.py --self-test
    python research/tools/audit_sin_original_grunt_behavior_event54_phase601.py --bin-zip /private/SE1/bin.zip

Authenticates original full binary SHA256 pair, 9 method full hashes, eight original 2006 vtable entries, method end padding, original Grunt +0x444, original 72-byte gate, 31 switch targets, **7 E8 and 1 E9** fallback transitions and **31** sparse opcode/field witnesses. Local authentic retail audit: **PASS**.

Synthetic self-test tests all 8 authentic original E8/E9 source-specific targets against every single-byte corruption of the five-byte encoding (**40 rejections**), and rejects incorrect original DLL SHA256. Registered as original-free CTest; CI does not include commercial retail DLLs.

**New source-authenticated methods: 9, complete bytes: 7,925.** This proves source-backed control flow and specific logic, not a complete engine-loaded Windows replacement DLL, a VR port or runtime equivalence. Generic Half-Life 2 physics, surfaces, materials, props and rendering remain out of scope per owner priority.
