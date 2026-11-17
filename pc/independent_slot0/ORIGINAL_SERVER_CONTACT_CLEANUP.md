# Original-guided SiN server contact cleanup (Phase 319)

## Evidence

Read-only inspection of the SHA-authenticated original x86 server.dll
(SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650)
at RVAs 0x21FFD0 and 0x220190 confirms both cleanup paths save the
successor pointer before invoking partner notification and freeing
the current node. Each resolves the contact handle through the
generation-checked registry described in Phase 318. Both decrement
their respective allocation counter before releasing a node and
recheck the data-object presence bit before final destruction.

The touch path walks link +8, checks bit 1 of the entity data-object
flags at +0x390, suppresses automatic data-object cleanup temporarily,
restores its previous setting after traversal, and resets the entity's
touch stamp at +0x40 even when no touch list exists.
The ground path walks link +4 and checks bit 0 of the flags word;
it leaves the touch stamp and suppression flag unchanged.

The owner's Source SDK 2004 archive,
src_mod/game_shared/physics_main_shared.cpp, provides the corresponding
PhysicsRemoveTouchedList, PhysicsRemoveGroundList, and partner
notification functions. Their control flow corroborates these observations.
The original layout differences must not be replaced with an assumed SDK
struct layout.

## Independent implementation

server_contact_cleanup_core.h uses independent node storage, a supplied
registry view, separate counters, explicit partner notification, node
release, and root-destruction hooks. It saves each successor before invoking
hooks and preserves the observed touch suppression and stamp behavior.
Missing root/data-object state takes the original no-list path.
Before mutation it rejects incomplete hooks, absent owner, and chains
that exceed the available node count, including off-sentinel cycles.
These latter checks are independent safeguards rather than original
game instructions. Callers must supply valid owned nodes and must not
destroy pending successors during callbacks. The hooks remain responsible
for reciprocal links, gameplay EndTouch/EndGroundContact behavior, and
actual allocator/data-object lifecycle.

The synthetic test exercises both traversal types, actual heap node
deletion, stale/invalid handles, callback order, presence rechecking,
counter accounting, missing hooks, and malformed-list refusal.

This is not an exported retail server interface or a full physics/contact
subsystem. FreeContainingEntity's real cleanup callbacks and the
ServerGameEnts001 export are still incomplete. No original executable
code or commercial DLL is committed.
