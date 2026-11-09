# Phase 135: original grunt hostility-mode relationship and condition gathering

## Evidence provenance

Based on the preserved Phase 17 static-analysis report for the owner-supplied 2006 Windows x86 server.dll, SHA-256:

568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650

The original DLL was not re-materialized or re-disassembled in this pass. No original game code was executed. Original VAs and object offsets apply only to that exact PE32 build.

## Slot 240: relationship override

CNPC_GruntBase slot 240 points to 0x223585B0; the CAI_BaseHumanoid parent slot points to 0x222A7F50.

Observed behavior:
1. Call the inherited humanoid relationship method first.
2. Read the byte at grunt this+0xDB1.
3. If zero, return the inherited disposition unchanged.
4. If nonzero, call target virtual slot 61 (+0xF4, high-confidence IsNPC match).
5. Only when false, call target virtual slot 72 (+0x120, high-confidence IsPlayer match).
6. Return D_HT (1) for NPC or player targets; otherwise D_NU (4).

Phase 18 subsequently identified +0xDB1 as the active byte of an embedded CAI_BeserkBehavior beginning at grunt +0xDA0. Its constructor initializes the byte to zero and its +0x14 deadline from current time. An activation-like routine sets the deadline to current time plus 10 seconds, but no static direct caller was found, so the real trigger and runtime reachability remain unverified. This PR's portable API takes a boolean representing that active byte and never enables it by default.

## Slot 316: condition-gather subset

CNPC_GruntBase slot 316 points to 0x22358600; the parent slot points to 0x22039D30.

- Mode byte zero delegates the whole operation to the parent.
- Mode byte nonzero iterates the host-resolved collection observed at this+0x898 in engine order.
- For each target, IsPlayer sets shared condition COND_SEE_PLAYER (0x1F).
- The relationship override is then called. Neutral D_NU targets skip most remaining processing.
- If a hostile target equals GetEnemy, set COND_SEE_ENEMY (0x0A).
- IRelationPriority (high-confidence slot 241 / byte offset +0x3C4) maps:
  - negative -> COND_SEE_DISLIKE (0x09);
  - 0 through 10 -> COND_SEE_HATE (0x07);
  - greater than 10 -> COND_SEE_NEMESIS (0x20).

The original loop also performs entity-flag checks and calls a further grunt virtual at +0x678. Those effects are not reconstructed. The C++ host contract explicitly defers this per-target tail.

## Implementation and verification

- Adds portable relationship and condition-gather C++.
- Keeps shared condition IDs separate from grunt-local IDs.
- Host supplies an ordered resolved view of the unidentified +0x898 collection.
- Preserves NPC-before-player short-circuit and player-condition-before-relation ordering.
- Regression tests cover parent delegation, dispositions, target order, enemy marking, priority bands, neutral skipping and unresolved-tail deferral.
- Desktop CTest, Android NDK ARM64 compilation, and Android Bionic x86_64 mock-host execution are distinct checks. None is SourceVR or Quest gameplay.

## Remaining work

Resolve the +0xDB1 flag name/activation, identify the +0x898 collection and +0x678 virtual, integrate with actual engine entities, and validate against original gameplay.

No proprietary DLL, APK, original ELF, BSP, model, map, script or game asset is included.
