# Gameplay Phase603: original 2006 Grunt CAI_BehaviorHost RTTI and pointer-layout constructor recovery

## Original native source provenance

The user's connected Google Drive SE1/bin.zip is now directly available for original game source investigation. The untouched original 2006 SiN Episodes binaries match:
- server.dll SHA256: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650
- client.dll SHA256: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d

Phases 597–602 established owner+0xC18 as a suspected current AI behavior pointer using the original Grunt vtable, behavior registration order, original event gate and comparative published Source SDK. **Phase603 now finds the actual original 2006 template class RTTI and its complete host constructor in the commercial retail executable**, removing the prior uncertainty about the host class identity itself.

## Confirmed original MSVC RTTI types

The actual original 2006 PE32 MSVC RTTI Complete Object Locator (pointer at native vtable minus four) and TypeDescriptor recover **exact** C++ original class names:

| Original vtable address | Original native MSVC RTTI TypeDescriptor |
| --- | --- |
| **0x224190A0** | **IBehaviorBackBridge** |
| **0x224188B8** | **CAI_BehaviorHost<CAI_BaseNPC>** |
| **0x2249543C** | **CNPC_GruntBase** |

These names are **not** post-hoc Source SDK 2013 guesses. The owner-private 2006 DLL was directly checked for the exact encoded RTTI string and native vtable-to-COL-to-TypeDescriptor linkage at each address.

The original 338-byte Grunt base constructor 0x22360D80, independently complete-SHA-authenticated in Phase535, **directly calls 0x2216EDE0 at 0x22360D86** before constructing the eight embedded behaviors (FuncTank, Rappel, Takecover, Beserk, Assault, Standoff, Follow and ActBusy). The original constructor installs a specialized secondary interface vtable at owner+0xC14. The 2006 CAI_BehaviorHost native class identity is now proved.

## One newly complete native function: host constructor 86 bytes

| New original method | Native range | Complete length | Whole-method SHA256 |
| --- | --- | --- | --- |
| Behavior-host base constructor | 0x2216EDE0–0x2216EE35 | **86 bytes** | **2d403029787e1848e3804b4cdcfddd20a8eae329bb262171c2f0859a3bda48e1** |

This complete native method ends before ten authentic INT3 padding bytes, with the next function beginning at 0x2216EE40. The previously verified Grunt base constructor 0x22360D80 is also rechecked in the same audit: **338 bytes**, original whole-SHA256 20ba9979f88e567a350b3378178dd03eef942667b891a83d4510fd539059356b.

Original method 0x2216EDE0 calls an inherited original base constructor at **0x22043DE0** (native E8 at 0x2216EDE3). It installs original interface vtable addresses at owner+0xC14 and initializes **owner+0xC18 to NULL** and each native behavior-collection field to zero:

| Original owner member | Verified initialization |
| --- | --- |
| **+0xC14** | RTTI-backed secondary IBehaviorBackBridge / CAI_BehaviorHost vtable |
| **+0xC18** | Zero/null pointer, before embedded behavior construction |
| **+0xC1C** | Zero, initial collection pointer/storage |
| **+0xC20** | Zero |
| **+0xC24** | Zero |
| **+0xC28** | Zero, previously Phase535 identified behavior vector count |
| **+0xC2C** | Zero |

Combined with Phase535's later insertion of eight source-verified embedded behavior pointers into +0xC1C and Phase587/592's two virtual calls through +0xC18, this is **strong original-source proof of the AI behavior-host layout**. The historical C++ name for field +0xC18 (the SDK uses m_pCurBehavior) remains not directly identified by original symbols, and a native behavior-selection store **after construction** has not yet been proved. Do not claim that the active pointer can never refer to a ninth type.

## Three important false positives eliminated from Phase595

The original Phase595 byte-pattern scanner reported 245 +0xC18 sites, including 51 possible assignments. Early Phase596 triage had marked near-Grunt sites as promising constructor or pointer-store candidates. Direct actual x86 instruction verification in the 2006 DLL now proves:

| Candidate original VA | Real original operation | Conclusion |
| --- | --- | --- |
| **0x223527AC** | Store literal integer **1** into another object's +0xC18 field | NOT an allocated behavior pointer |
| **0x22353C3C** | Store literal integer **3** into another object's +0xC18 field | NOT an allocated behavior pointer |
| **0x22353D4C** | Store literal integer **2** into another object's +0xC18 field | NOT an allocated behavior pointer |
| **0x2216EDF4** | Store zero from EAX into genuine behavior-host owner +0xC18 | **Confirmed original constructor initializer** |

These same-offset instruction matches belong to distinct native object layouts. Phase596's earlier assignment candidates were correctly marked unresolved; the next research should exclude the integer literal 1/2/3 stores from any current-behavior pointer assignment graph.

The previously identified additional E8 calls at **0x2208C00E** and **0x2208C054** are also visible as actual original direct calls to event gate 0x2202D930 in a separate native event method, rather than merely accidental opcode patterns. These are shared behavior-host usage leads, not evidence of Grunt constructor assignments.

## Exact source validation and remaining work

New read-only audit: research/tools/audit_sin_original_grunt_behavior_host_ctor_phase603.py

    python research/tools/audit_sin_original_grunt_behavior_host_ctor_phase603.py --self-test
    python research/tools/audit_sin_original_grunt_behavior_host_ctor_phase603.py --bin-zip /private/SE1/bin.zip

It strictly checks full original DLL SHA256 pair, Grunt and behavior-host constructor whole-method SHA256s and boundaries, the exact three original RTTI vtables/COL TypeDescriptors, the two authentic E8 constructor edges, Grunt secondary vtable field, real null/vector initializer instructions, and all three unrelated integer +0xC18 stores. Source-specific synthetic regression flips each of the five bytes of both constructor edges: **10 invalid mutations correctly rejected**. Wrong-DLL SHA is refused; public CI does not contain original game bytes.

**Local source authentication: PASS.** One newly recovered 86-byte complete native method plus cross-confirmation of earlier 338-byte Grunt ctor. Combined Phases 601–603: **15 distinct complete original gameplay methods, 8,803 bytes** (7,925 event + 792 activity + 86 constructor).

The remaining native unknown is the exact runtime assignment/selection path setting owner+0xC18 to an embedded AI behavior instance, plus complete behavior special-event source translations. No generic Half-Life 2 physics, surface-properties, models, props or rendering work is performed. This is original gameplay decompilation evidence, **not** a playable Windows replacement DLL, Android port or Quest 3 runtime.
