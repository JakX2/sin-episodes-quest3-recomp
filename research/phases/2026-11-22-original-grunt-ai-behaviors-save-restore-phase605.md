# Gameplay Phase605: original Grunt AI_Behaviors save/restore, not live policy selection

## Authentic original 2006 DLLs and corrected interpretation

The user's connected Drive SE1/bin.zip contains original 2006 SiN Episodes: Emergence server.dll (SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650) and client.dll (SHA256 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d). Original executable bytes were directly analyzed without substituting published Source SDK 2013 C++ for the 2006 implementation. The original game binaries, executable code dumps and original assets are not committed.

**Correction to Phase604:** Original Grunt primary vtable 0x2249AAA4 +0xA4 = 0x2216DE40 is a **SAVEGAME RESTORE wrapper**, not evidence for normal in-combat behavior-selection policy. It genuinely writes registered array pointers into owner+0xC18, but it restores the saved active behavior. The paired +0xA0 = 0x2216DDF0 is the **SAVE wrapper**. Phase604's original vtable pointers and pointer-transition method 0x2216E510 remain correct; Phase604's label "candidate behavior selection" must not be interpreted as normal scheduling.

The direct 2006 evidence is the paired Save/Restore wrappers, their exact original E8 callees, serialization literal **AI_Behaviors**, serialized header version 2, per-behavior registered-name matching, active flag recording and a returned restored behavior index.

## Three new whole native methods, 652 bytes

| Function role | Original entry | Complete source bytes | Whole SHA256 |
| --- | --- | ---: | --- |
| Serialize AI behavior group | 0x220CD530 | **244** | a51be7f7295a12b98ed710325c55aa7b3ac97e938f9c8cedcd84cc0793f50c0f |
| Restore AI behavior group | 0x220CD630 | **341** | 737d723103081e9c2474976ee2376d82f151db843c7a1e63988572702a3bd908 |
| Original Grunt Save wrapper | 0x2216DDF0 | **67** | 7d6a0a11e435df9b172cc1aa7cba7dd611d38e8f83b8d1a648611706b67b8b1e |

The exact original method end alignments include 12, 11 and 13 verified INT3 padding bytes respectively. The 104-byte original Grunt Restore wrapper at 0x2216DE40, SHA256 273450c7062ad7ca28aa34edb7faa41c3d5f5d97641e943b32e387c8b06b3ee7, was already counted in Phase604 and is only **re-authenticated**, not counted again.

### Native Grunt virtuals and direct call graph

- Original Grunt primary vtable 0x2249AAA4 +0xA0 -> 0x2216DDF0 (Save wrapper); +0xA4 -> 0x2216DE40 (Restore wrapper).
- 0x2216DDF0: inherited original 0x22043820 called from 0x2216DDFA, then AI_Behaviors saver 0x220CD530 called from 0x2216DE23.
- 0x2216DE40: inherited original 0x22043AD0 called from 0x2216DE4A, then AI_Behaviors restorer 0x220CD630 called from 0x2216DE6C.
- A separate original BlendedNPC specialization calls those shared savers from 0x22375D63 and 0x22375DAC. Its vtable must NOT be conflated with Grunt.

All six original E8 rel32 direct targets are checked at exact code locations.

## What the original source really serializes

### Save helper 0x220CD530

Opens the section **AI_Behaviors** via serialization object virtual+0x1C, writes a header value **2** and the provided registered behavior count. Iterates the registered behavior pointer list. Original behavior virtual+0xD8 supplies a behavior name/class identity. The helper compares against global pointer 0x224E4B38, which points to the exact **CAI_BehaviorBase** string at 0x223F380C. An exact base class name match is skipped. Other behaviors serialize their name, a flag derived from **behavior pointer == current pointer**, and their state through behavior virtual+0x30.

### Restore helper 0x220CD630

Gets the serialized section's name via its virtual+0x18 and compares **13 bytes including terminator** against **AI_Behaviors** at 0x223F3830. Checks stored format version WORD==**2** and the number of saved records. It enumerates saved records and current registered behavior pointers, matches each saved name against the registered behavior virtual+0xD8 identity, and calls matching behavior virtual+0x34 to restore its data. If the saved record's active flag is set and a registered behavior matches, the helper records that registered index; the default no-match index is **-1**.

### Grunt Restore wrapper 0x2216DE40

First invokes the inherited original restore method at 0x22043AD0, and on success calls 0x220CD630 with the actual behavior-vector backing pointer and count. A returned index other than -1 leads to an array element load and **owner+0xC18 pointer store at 0x2216DE8B**. A -1 result stores **NULL at 0x2216DE96**. This is savegame state restoration, not a combat-time schedule/behavior choice.

The exact original serializer interface types and the historical developer member name for +0xC18 have not been recovered from symbols. The observed 2006 dataflow is authenticated; a portable reimplementation is not yet integrated into a playable game.

## Normal runtime behavior selection remains a separate question

The native pointer transition method 0x2216E510 established in Phase604 is real. Original direct E8 byte-pattern references to it include 0x2216E4E5, 0x2216E6DF, 0x2216E8E6, 0x2216E928, 0x2216E968, 0x22182B1C and 0x2234E61E. In particular, 0x2234E61E appears in a separate Grunt-area path and merits original receiver/class verification. These references need enclosing method and input provenance before any can be called the true combat behavior-selection policy.

## Reproducible audit and limitations

Tool: research/tools/audit_sin_original_grunt_behavior_save_restore_phase605.py

    python research/tools/audit_sin_original_grunt_behavior_save_restore_phase605.py --self-test
    python research/tools/audit_sin_original_grunt_behavior_save_restore_phase605.py --bin-zip /private/SE1/bin.zip

The SHA-gated auditor checks the exact original DLL pair, three new method whole SHA256s and INT3 boundaries, previously authenticated Restore wrapper, actual paired Grunt vtable entries, exact original AI_Behaviors and CAI_BehaviorBase literals, global sentinel pointer, six E8 direct calls, and 15 sparse native opcode witnesses. Only addresses, digests, class labels and metadata are exported; no commercial machine code. Synthetic self-test mutates every byte of six E8 call encodings (**30 corrupted branches refused**) and checks wrong-source refusal.

**Three newly recovered complete original methods / 652 original bytes. Phases601–605 total: 22 unique whole methods / 9,786 original native bytes.** This is not 9,786 bytes of reconstructed portable C++, a source-equivalent Windows DLL, Android port or Quest 3 runtime. Generic HL2 physics, surface properties, models, props and rendering remain explicitly out of scope.
