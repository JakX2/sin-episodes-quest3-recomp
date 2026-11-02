# Original 2006 SiN CGrenadeAR detonation: effects, damage dispatch, decals and timing

**Provenance:** the owner's original Windows PE32 SiN Episodes: Emergence server.dll, SHA-256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. This phase does NOT build gameplay for Android or load BSP maps. All original x86 virtual addresses and object offsets are reference-only.

## Original method identification

Earlier phase identified original CGrenadeAR : CBaseGrenade virtual slot **250** as **Detonate** at **0x22314570**, corroborated by four independently named grenade subclass Detonate exports at the corresponding virtual slot. Touch handler is 0x223142F0 and periodic think handler is 0x22314360. This work traces their specific code evidence.

**Original Detonate function 0x22314570**:
- Checks byte field **+0x795** before proceeding. If zero, jumps directly to its return path; if nonzero, clears it as the detonation begins. This acts as a gated/one-shot effect flag; additional state paths remain.
- Interacts with linked entity handles, dispatches an effect through an engine virtual at +0x4C, and reads original object fields **+0x798** and **+0x7AC** into effect/damage-related inputs. Their source-level field names and exact roles have **not been proved**.
- Builds a trace near the projectile origin **+0x2BC**, using vector data at **+0x250** scaled by the exact original **60.0** float at x86 rdata 0x223DDDCC. Original trace helper is **0x22014CC0**, callsite **0x223147F5**.
- Based on trace hit context, selects exact original decal names **Scorch** (string 0x22407CB0) or **SmallScorch** (0x22477BD4) and calls **0x2228B320** at 0x2231483D.
- Calls original **0x22288A90** at 0x22314873 with origin and immediate scalar arguments **25.0 amplitude, 150.0 frequency, 1.0 duration, 750.0 radius**, followed by command/airshake constants. Its original machine code clamps amplitude at **16.0** (also verified at 0x22288A95 and 0x22288AA2) and iterates over the original player list. The call shape and clamp closely match Valve Source SDK **UTIL_ScreenShake**, so that is a high-confidence identification rather than a blind guess. Secondary reference: https://github.com/ValveSoftware/source-sdk-2013/blob/master/src/game/server/util.cpp . Note that the original SiN code, not the later Source SDK, supplied the actual numbers.
- Resolves a possible attacker/owner using **0x222B2730** at 0x223148E7, constructs damage-related state with **0x221F89C0** at 0x223148F5, invokes a shared damage/effect dispatcher **0x222A5E10** at 0x223148FB (which itself calls engine virtual +0xB8), and finally calls removal **0x22286BF0** at 0x22314901. The precise C++ API name, damage radius units and ownership rules of the shared dispatch remain unverified.

**Do not confuse these direct call targets with a reconstructed complete explosion**. The engine virtual callbacks, damage-info structure layout, networking, collision masks and trace interfaces must be implemented using compatible ARM64 SourceVR game interfaces.

## Touch and think conditions

- Touch callback **0x223142F0** filters incoming collision/trace input and, after related contact checks, invokes the object virtual at **+0x3E8**, i.e. slot **250**, when the flag at **+0x795** is set.
- Think callback **0x22314360** invokes the same virtual slot 250 at **0x22314444** under additional time/physics conditions.
- A recurring think is scheduled at game time plus **0.05 seconds**, original float at **0x223D9CB0**. In this routine, a separate time comparison uses **0.1 seconds**, original float at **0x224771F8**. Original projectile initialization also scheduled an initial think at time+0.1 in the prior investigation. These are **different uses of timing constants**.
- The think callback reads **+0x7C4**, compares it with original float **300.0** at **0x224771FC**, and conditionally adds original double **15.0** at **0x22467448**. This field is not yet proven to represent a physical radius, timer or damage quantity. Do not assign it a C++ field name yet.

## Exact machine-code evidence summary

| Relevant code address | Verified original evidence |
|---|---|
| 0x22314579 | Reads +0x795 and tests it against zero |
| 0x223145A5 | Clears +0x795 during detonation |
| 0x2231466F / 0x22314675 | Reads +0x7AC and +0x798 respectively |
| 0x22314743 | Multiplies vector component by float 60.0 |
| 0x223147F5 | Direct call to trace helper 0x22014CC0 |
| 0x22314817 / 0x22314833 | Original Scorch / SmallScorch literal pointers |
| 0x2231483D | Direct call 0x2228B320 to emit selected decal |
| 0x22314873 | Direct call 0x22288A90, screen-shake-like player feedback |
| 0x223148E7 / 0x223148F5 / 0x223148FB | Owner resolver, damage-info-like state, engine effect/damage dispatch |
| 0x22314901 | Final removal call 0x22286BF0 |
| 0x2231434A / 0x22314444 | Original touch/think virtual calls at vtable +0x3E8 |
| 0x22314450 / 0x22314463 | Compares +0x7C4 to 300.0, adds 15.0 |

A new strict SHA-256-pinned standard-library tool, research/tools/audit_sin_ar_detonation.py, checks **23 original x86 instruction witnesses**, **12 rel32 direct callsite-to-callee edges**, both original decal strings and original float/double constants. It refuses a different server.dll build rather than silently applying addresses to other releases.

## Reproduction

Run with the user's original private game binary (not HL2's server.dll):

    py research/tools/audit_sin_ar_detonation.py --server "C:\SiN\bin\server.dll" --output "sin-ar-detonation.json"

The accompanying research/tests/test_sin_ar_detonation.py supplies an original-free synthetic PE fixture and mutation tests. Original commercial DLL bytes and content are not committed.

## What the ARM64 gameplay port needs next

1. Map exact server-side damage-info fields and dispatch call signatures against original byte/argument evidence, including the effect of +0x798 and +0x7AC. Do not merely infer semantics from the stored numeric defaults.
2. Reproduce touch filtering, physics trace/collision response and the one-shot +0x795 gate in a host-compatible entity.
3. Handle Source's screen shake carefully in VR; flat-screen camera displacement should not blindly displace the headset's tracked camera. Effects and decals need separate Android renderer compatibility validation.
4. Reconcile projectile replication and owner/attacker identity with original client.dll and server.dll Send/Recv tables, even if no dedicated DT_GrenadeAR literal exists.
5. Compare real Windows SiN behavior to the independently implemented source-compatible ARM64 game code.

**This is an evidence-backed original-game code inventory, not a finished gameplay module.**
