# Phase 1622: Original mutant idle/locomotion sound precache

Direct authenticated read-only recovery from original PE32 `server.dll` (SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`). The Major and Minor each override virtual +0x180 to make ordered calls to the inherited `0x221b6a80` sound precache service. Original engine asset system and all existing registration API layouts remain delegated.

Original Major +0x180 entry `0x22377070`, 39 bytes, SHA-256 `56f79788d273357d82b978175fd72407c39c2fc7d33b4d11ddcb96501d92c477`. It submits the strings in this exact order: `NPC_Leperkin.Idle`, `NPC_Leperkin.Run`, `NPC_Leperkin.WalkFast`.

Original Minor +0x180 entry `0x2237b3e0`, 51 bytes, SHA-256 `19a90f5a8e59095f9e53e115554c228e3aa25baecdf4dd707c0c2a113441c139`. It submits `NPC_gib_hopper.Idle`, `NPC_gib_hopper.Walk`, `NPC_gib_hopper.WalkBackwards`, `NPC_gib_hopper.Run`.

The new `SiN_GetServerRecombinantSoundPrecacheApi` is a separate server-only versioned export. It verifies actor class identity and forwards the ordered class-specific names to a caller-supplied host callback. The retail precache routine does not use a returned failure status; the independent native wrapper deliberately fails closed if a host callback rejects and reports attempted/successful requests without rollback. Existing `SiN_GetServerRecombinantRegistrationApi` is unchanged.

The loaded Windows regression checks both exact name sequences, no client export, size/version negotiation, failed callback accounting, early invalid input rejection and wrong-class rejection.

These functions are sound **precaching**, not an NPC transformation mechanism. The distinct `NPC_Leperkin.MorphToHopper` and `NPC_gib_hopper.MorphToLeperkin` names appear in each class's larger earlier precache body; the compact +0x180 methods do not submit those names. Nothing in this phase claims functional morphing.

No proprietary executable bytes committed; no generic Source physics, models, props, material systems, audio backend or game startup fabricated. CI pending.
