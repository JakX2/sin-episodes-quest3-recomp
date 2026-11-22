# Gameplay Phase602: five complete original Grunt behavior activity virtual methods

## Retail original-binary authenticity

Direct original 2006 SiN Episodes: Emergence binaries from the owner's connected Drive SE1/bin.zip:
- server SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650
- client SHA256 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d

Phase535 already established eight embedded Grunt behavior class RTTI/vtable identities. Phase598 confirmed their eight original native +0x7C virtual activity targets, which reduce to **five distinct source method addresses**. Phase601 established that original owner+0xC18 is used by the event gate and Grunt activity remapper, but the developer-declared member name is still a structural hypothesis. This pass directly authenticates and inspects all five actual activity method bodies.

## Five new complete original 2006 native methods, 792 bytes

| Actual original method | Start VA | Complete native bytes | Original whole-method SHA256 |
| --- | --- | ---: | --- |
| Common activity backbridge, shared by Rappel/Beserk/ActBusy/Assault | 0x220CD400 | **8** | b7bd9eccad64400217c35647d73def30cb590070fdf477ff01b2d56b7eb0ccf4 |
| CAI_FuncTankBehavior | 0x22392700 | **19** | 7dc9cc2cb138916a0ffa128954a400c587c0e07c6a3c6d16c371b08d18e846c9 |
| CAI_TakecoverBehavior | 0x22398670 | **457** | 286e96bf50d40a21365ba40578c3154317bcd646cbe4bbcd3827491a2ed44851 |
| CGruntStandoffBehavior | 0x220E1EE0 | **142** | 28d47fabade3850134945b8e988f988e42370608920efd8b709e0955c3ad4133 |
| CAI_FollowBehavior | 0x220D33B0 | **166** | 415e92b3d358ef558a9bfcc0dd61acd4877eded4df38a457b48b1db5f98946c2 |

All five full span SHA256 checks pass on the exact original 2006 DLL. Every corresponding native behavior vtable +0x7C matches. There are **four direct original native calls/tailjump edges** to the shared routine and **20** sparse opcode/field witnesses, all checked by the auditor.

### Correcting the Standoff boundary

Original Standoff +0x7C method ends at exclusive **0x220E1F6E**. The following **two INT3 bytes** at 0x220E1F6E/0x220E1F6F are followed by a **different original method starting at 0x220E1F70**. A naive search for a longer INT3 run might misattribute the next method to Standoff, reporting a false 738-byte body. The proper whole-method digest applies to exactly **142 bytes**.

This is the second actual adjacent-code boundary correction in two phases, after Phase601 fixed Follow event method's 1,868-byte body.

## Native source semantics newly recovered

### Shared original activity bridge (0x220CD400)

The actual eight-byte function accesses the pointer at behavior+0x0C, loads that object's vtable and tail-dispatches to its **virtual +0x0C**. This is not a literal activity-ID passthrough. It is a host/bridge delegate, consistent with but independently verified beyond the newer Valve Source SDK comparison.

Exactly four registered Grunt behavior vtables use this target directly at +0x7C: Rappel, Beserk, ActBusy and Assault.

### FuncTank behavior (0x22392700)

If its own byte **+0x18 is nonzero**, return native numeric activity **0x171 (369)**. If it is zero, tailjump to original 0x220CD400 and delegate activity mapping to the owner bridge. No invented symbolic ACT name for ID 369 is assigned until the original registration has been checked.

### Takecover behavior (0x22398670)

The original routine branches on the **input activity**, uses the Grunt owner's entity handle field **+0xA18** with original low-12 index/high-serial checks against the original global entity table at 0x224F5620, and conditionally remaps:

| Input raw activity | Original specific condition | Native return |
| --- | --- | --- |
| 0x10 (16) | Handle resolves; target's original WORD+0x3A4 equals 0x65 | **0x12 (18)** |
| 0x10 (16) | Handle resolves; target WORD+0x3A4 equals 0x6B | **0xE8 (232)** |
| 0x10 (16) | Handle path valid and target recovered through another native call, WORD+0x3A4 equals 0x6C | **0xE9 (233)** |
| 0x42 (66) | Valid original serial-checked owner+0xA18 handle | **0x43 (67)** |
| 0x01 (1) or 0x48 (72) | Valid original serial-checked owner+0xA18 handle | **0x2D (45)** |

The other paths call the common bridge at 0x220CD400, including when the handle is invalid or no special condition qualifies. Unsafe 2006 native pointer-dereference behavior is not portrayed as safe portable C++ semantics. The audit pins original sparse comparison/return witnesses and actual delegate edge; not every possible null or malformed native memory access has been reverse-engineered.

### GruntStandoff behavior (0x220E1EE0)

Reads owner's original serial-checked +0xA18 handle and selectively calls owner virtual **+0x714**, then delegates the resulting activity along with an original Standoff state at **behavior+0x40** through original helper **0x220E1E80**. If this helper returns native -1, the handler delegates to common 0x220CD400. An original state transition from **+0x40 = 1 to 2** occurs when the earlier raw return code equals **5** in the verified branch. The precise design-level name of this state field remains unknown.

### Follow behavior (0x220D33B0)

Input activity **1** is treated specially when native predicate 0x220D23F0 succeeds; then the original +0xA18 encoded entity handle is resolved and extra target data is tested, potentially forwarding via owner virtual **+0x70C**. Other cases and unsuccessful gates delegate to original 0x220CD400. This does not establish the exact result of the extra owner virtual, and the portable model should preserve it as an unresolved callback.

## Source verification and tests

New auditor: research/tools/audit_sin_original_grunt_behavior_activities_phase602.py

    python research/tools/audit_sin_original_grunt_behavior_activities_phase602.py --self-test
    python research/tools/audit_sin_original_grunt_behavior_activities_phase602.py --bin-zip /private/SE1/bin.zip

It reuses the exact SHA-gated PE32 mapping from Phase601, validates all eight native +0x7C vtable slot addresses, five whole-method digests, real 2–8 byte INT3 boundary alignments, original adjacent Standoff function, four exact native E8/E9 branch targets to the shared activity bridge, and 20 sparse native instruction witnesses.

**Authentic owner retail audit: PASS.** Original-free self-test checks all four direct call/jump encodings and rejects five different one-byte corruptions per edge (**20 negative tests**). Wrong original DLL SHA rejected. Added as synthetic CTest; public CI never includes the original DLLs.

**Five newly authenticated complete native methods, 792 bytes.** Combined with Phase601: **14 original functions / 8,717 bytes** recovered across the exact event and activity targets of the eight registered Grunt behaviors. This does not produce a ready Windows native server.dll, prove every AI animation emitter asset or run Android/Quest. Generic HL2-equivalent physics/materials/surface/model/prop work is excluded by owner priority.
