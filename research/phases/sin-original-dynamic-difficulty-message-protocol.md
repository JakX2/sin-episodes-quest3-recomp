# Original SiN Episodes 2006: dynamic-difficulty client/server protocol

**Primary source:** owner-held unmodified Windows x86 SiN Episodes: Emergence DLLs: client SHA-256 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d and server SHA-256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. No original DLL bytes are committed; x86 virtual addresses cannot be used on Android ARM64. No full C++ decompilation or native Quest DLL is claimed.

## Nine identical user-message declarations in the actual original DLLs

The original client and server .text both contain exact register-message instructions with matching names and declared size arguments.

| Original message name | Client registration push | Server registration push | Declared size, both |
|---|---|---|---:|
| GamePlayVarValue | 0x2420F882 | 0x222CFE32 | 6 |
| Progression | 0x2420F894 | 0x222CFE44 | 4 |
| DDInfo | 0x2420F8A6 | 0x222CFE56 | -1 |
| DDAdvisorScores | 0x2420F8B8 | 0x222CFE68 | -1 |
| DDAdvisorChannels | 0x2420F8CA | 0x222CFE7A | -1 |
| DDAdvisorsUpdate | 0x2420F8DC | 0x222CFE8C | -1 |
| DDDecisionMade | 0x2420F8EE | 0x222CFE9E | -1 |
| DDDecisionsPerf | 0x2420F900 | 0x222CFEB0 | -1 |
| DDSuccessModifiers | 0x2420F912 | 0x222CFEC2 | -1 |

Client registration helper: original x86 0x241D0280; server registration helper: 0x220229A0. -1 means a variable-length registration, not a negative or one-byte payload.

## Actual original client callback registrations

The original C_DynamicDifficulty routine **0x241FAC90** registers callback/name pairs through 0x241CFF70:

| Message | Callback | Confirmed underlying reader |
|---|---|---|
| GamePlayVarValue | 0x241F9960 | Additional inner reader not yet classified |
| Progression | 0x241F9970 | Uses 0x240A5620 |
| DDInfo | 0x241F9F80 | 0x241F99A0 |
| DDAdvisorChannels | 0x241FAC40 | 0x241FA850 |
| DDAdvisorScores | 0x241FAC50 | 0x241FAA60 |
| DDDecisionMade | 0x241FAC60 | 0x241FAB10 |
| DDDecisionsPerf | 0x241F9F90 | 0x241F9C20 |
| DDSuccessModifiers | 0x241F9990 | 0x241F9770 |

Immediately adjacent client callback registrations also exist for **ArenaMode**, **NewGameInfo** and **Cheating**; do not conflate these with adaptive-difficulty metrics. **DDAdvisorsUpdate** has an original engine message registration in both DLLs, but no hook in this specific C_DynamicDifficulty callback block. Do not infer that it is unused or unhandled elsewhere.

**Correction to tentative PR #71 commentary:** in the original C_DynamicDifficulty primary vtable, **slot 0 is 0x241FAC90**, a routine registering message hooks; **slot 11 is 0x241FAC70**, a deleting-destructor-like function that conditionally frees the object. Previous speculation that the first slot was the destructor was reversed. The actual disassembly settles the distinction.

## Exact sender/receiver agreement: DDDecisionMade ordinary record

Original server writer: **0x222B9770**. Client decoder: **0x241FAB10**. Their explicit typed calls match:

| Field index | Original server CALL site | Original client CALL site | Type |
|---:|---|---|---|
| 1 | 0x222B9787 | 0x241FAB1D | float32 |
| 2 | 0x222B9790 | 0x241FAB7C | int32 |
| 3 | 0x222B9799 | 0x241FAB85 | int32 |
| 4 | 0x222B97A2 | 0x241FAB8E | int32 |
| 5 | 0x222B97AB | 0x241FAB99 | float32 |
| 6 | 0x222B97B4 | 0x241FABA4 | float32 |
| 7 | 0x222B97BD | 0x241FABAF | float32 |
| 8 | 0x222B97C6 | 0x241FABBA | float32 |

The writer calls original 0x221E8830 for floats and 0x221E87A0 for integers. The reader calls original 0x240A5630 for floats and 0x240A5600 for integers. These are original instruction targets, not type labels inferred from names alone.

**Variable-length caution:** The first client float is compared against original 0.0 and -1.0 constants and takes special paths before consuming the rest of an ordinary record. The server also sends a short -1.0 notification in a separate routine near 0x222BA830. Do **not** assume every DDDecisionMade packet contains all eight fields. The exact meaning of both sentinels remains to be established.

## DDSuccessModifiers: 10 pairs of two float32 values

The original server sender near **0x222BA9C0** and client decoder **0x241F9770** each perform a **10-iteration loop with two float value operations per iteration**, for 20 float32 values on the normal path.

- Client read CALL sites: 0x241F9782 and 0x241F9798, both to 0x240A5630, loop comparison at 0x241F97AF.
- Server write CALL sites: 0x222BAA15 and 0x222BAA29, both to 0x221E8830, loop comparison at 0x222BAA34.
- Server derives values via original helper 0x222C07A0 with selector arguments 1 and 3. Client passes read values to 0x241FCF80 with selector arguments 1 and 3. What each of the ten indices represents in gameplay remains unproven.

## CDynamicDifficultyAction: exact original member field meanings

The original 2006 server virtual method **0x2215BDD0** reads its own data and prints matching **literal debug labels**. This verifies the following without fabricating names:

| Original class offset | Type | Exact original printed label | x86 member read |
|---|---|---|---|
| +0x2BC | float32 | Time started | 0x2215BE07 |
| +0x2C8 | float32 | Value | 0x2215BE17 |
| +0x2C4 | int32 | Decision | 0x2215BE2A |
| +0x2C0 | int32 | Advisor | 0x2215BE38 |

The actual original strings are "--Time started: %.4f", "--Value: %.4f", "--Decision: %.i" and "--Advisor: %.i". The audit verifies each source instruction AND its format-string pointer. This is stronger than matching a name in .rdata.

## Six advisor data groups

The original client readers for the advisor-channel and advisor-score messages loop through **six groups**. Corresponding original server writer loops are also visible. Neither six item names nor their difficulty weighting formulas have been recovered, so do not invent them from the number of entries.

## Verification

Run the exact original binaries locally:

    py research/tools/audit_sin_difficulty_protocol.py --client "C:\SiN\bin\client.dll" --server "C:\SiN\bin\server.dll" --output "C:\private\sin-difficulty-protocol.json"

The auditor checks exact message bytes, 9 paired registration callsites/sizes, 8 client message hook callbacks, six client reader delegations, eight paired DDDecisionMade typed writer/read operations, 10×2 float loop structures, two client decision sentinels and four server action fields. Original-free synthetic regression tests deliberately mutate call targets, field loads, registry sizes, labels and sentinels.

## Next steps toward all client.dll and server.dll behavior

1. Follow real client DDInfo, advisor and decision handlers into their state fields and UI/progression consumers. Decode remaining seven message types before claiming complete protocol parity.
2. Reverse server nonvirtual dynamic-difficulty manager functions, score/threshold formulas and how actions affect AI, weapons, saving and player experience.
3. Map message bit widths, exact field lifecycle and replication timing, not just the C++ read/write scalar types.
4. Implement the reconstructed game code against **verified native SourceVR ARM64 client/server interfaces**, and compare behavior with original SiN on Windows.
5. Continue the whole-DLL work queue across original weapon, NPC, HUD, poison/antigen, vehicle, scene and save/load systems. The entire 2006 DLLs are not yet recovered.

**Scope boundary:** this is binary-grounded message and member-field reconnaissance. It does not provide an Android executable gameplay module or complete SiN difficulty gameplay formulas.


## Original server action factory: eight named script/action classes

The original 2006 server compiled action-creation dispatcher at x86 0x2215D1A0 matches exactly **eight** original literal action labels in a sequence of compare-and-construct branches:

| Original action label | Label-reference instruction |
|---|---|
| Look Action | 0x2215D1A7 |
| Use Action | 0x2215D21A |
| Weapon Action | 0x2215D280 |
| Map Action | 0x2215D2E0 |
| Move Action | 0x2215D311 |
| Health Action | 0x2215D342 |
| Game Action | 0x2215D36F |
| **Dynamic Difficulty Action** | **0x2215D39C** |

The **Dynamic Difficulty Action** comparison branch allocates a 0x2CC-byte object and tail-jumps at **0x2215D3C4** to original **0x2215CD20**. The constructor initializes its vfptr with the independently RTTI-validated primary vtable pointer **0x224116B8**, directly identifying the class as **CDynamicDifficultyAction** rather than assuming from string similarity. The constructor passes argument 7 to its shared action-base initializer.

These strings establish eight recognized factory labels in this particular original dispatcher; they do not prove every script action is implemented on Quest, or fully reveal the execution semantics of any one action. Dynamic Difficulty Action specifically is a *server-side gameplay action class*, beyond the client network-message protocol described above.


## 2026-11-03 primary-binary correction to this earlier report (supersedes tentative type/sentinel labels)

The original client helper `0x240A5600` pushes bit width `0x10` and calls `0x240A5080` (signed reader); the original server helper `0x221E87A0` calls `0x2225A160`, which pushes `0x10` and forwards to signed writer `0x22259B00`. Therefore the three integer **wire fields are signed 16-bit**, not 32-bit despite their 32-bit source member loads and 32-bit CPU registers. The complete ordinary sequence is **f32, s16, s16, s16, f32, f32, f32, f32** = **208 payload bits** (assuming standard 32-bit float bit serialization). Original client branch instructions `0x241FAB26`/`0x241FAB37` compare against 0 and -1 with x87 FCOMP, TEST AH,5 and JP, meaning **threshold conditions rather than exact value sentinels**: first >= 0 takes the ordinary path, `-1 <= first < 0` zeros client state at offsets +0x9C and +0xA0, and `first < -1` invokes `0x241FA450` on its list at +0xA4. Client unordered NaN follows the ordinary branch under these original flag checks. Earlier wording about equality-based sentinels and i32 wire fields is superseded.
