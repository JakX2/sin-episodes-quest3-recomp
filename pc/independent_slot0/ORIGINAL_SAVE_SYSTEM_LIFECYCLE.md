# Phase 338: original save-block and game-system lifecycle

## Evidence

Owner-private server.dll was locally SHA-256 verified as
568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650.
Direct PE32 inspection establishes the original post-console startup
sequence at RVA 0x1E7861..0x1E7940. The SDK 2004 gameinterface and
isaverestore/igamesystem headers corroborate the interface roles.
The original has five save-block getters in the same order as that
reference:

| Kind | Original getter RVA | Singleton RVA |
| --- | --- | --- |
| Entity | 0x273BE0 | 0x51BA84 |
| Physics | 0x22ECA0 | 0x575288 |
| AI | 0x09DF20 | 0x4E250C |
| Templates | 0x1C2F90 | 0x50967C |
| Default response system | 0x091790 | 0x4E1E4C |

Each result is passed to save-block set slot 9 (byte offset 0x24).
The original set has 12 slots, including the nine block-handler
operations and slots 9..11 for add, remove and specific restore.
The original shutdown path at 0x1E79B0 removes the same five
identities in reverse order through slot 10 (byte offset 0x28).

Six explicit game-system getters are invoked in the following order
(the SDK reference has only five; do not assume identical lists):

| Order | Original getter | Singleton |
| --- | --- | --- |
| 1 | 0x1F3DB0 | 0x572AE0 |
| 2 | 0x20D900 | 0x513AE0 |
| 3 | 0x2F0FA0 | 0x595F58 |
| 4 | 0x2B67D0 | 0x521864 |
| 5 | 0x25F8E0 | 0x57AE9C |
| 6 | 0x1B4E50 | 0x507558 |

The first two and last two correspond to string, physics, HLTV and
sound systems; the third/fourth are retained as original identities
until their full functionality is established. All six are appended
by the helper at 0x1F56E0. Original DLLInit then calls event-manager
slot 1 with resource/ModEvents.res, then checks the boolean results
of InitAllSystems (0x1F5700), scene cache init (0x1AD8A0) and model
sounds cache init (0x149B60).

The original 0x1F5700 manager appends the auto-system linked list
(head global 0x572CC0; next pointer +4). A duplicate pointer is
skipped with a diagnostic warning. It clears the auto head and
marks initialized before calling individual system Init slot 0.
The loop rereads the count after each call, so a system appended
during initialization is visited. The first false result terminates
without automatic rollback. Forward/reverse dispatch functions
0x1F5330/0x1F5380 enter and leave the engine-cache critical section
through slots 5 and 6 around each system callback.

## Independent implementation

server_save_system_lifecycle.h supplies typed native 9/12-slot
save interfaces, five-handler registration/removal, and native
12-slot game-system interface with Init first, destructor last.
It implements bounded owned ordered lists, auto deduplication,
late append during Init, failure propagation and reverse
cache-wrapped shutdown. It refuses invalid inputs, cyclic chains
and reentry; it only allows ordinary shutdown after successful
initialization, rather than guessing partial-failure cleanup.
Owners keep all supplied objects alive. No original memory is
overlaid and no empty stub is exported to the retail engine.

The native test verifies five registration/removal pairs, six
explicit starts, two unique auto systems, a duplicate auto,
a late-appended system, exact forward/reverse order and locks,
cyclic preflight, failure at the third system and non-retry.
Inspect-OwnerOriginalServerPE.py authenticates the getter edges,
singleton identities, add/remove slots and late initialization
edges locally. This is not the concrete save serializer or full
game-system implementation. Phase 337 is the next retail diagnostic.
No game binaries or original implementation bodies are committed.
