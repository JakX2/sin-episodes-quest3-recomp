# Phase 329: original SiN shared user-message catalog

## Evidence

Both owner-private game DLLs were SHA-256 checked locally:

- server.dll: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650
- client.dll: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d

The original server registration routine spans RVA 0x2CFC20 through
0x2CFEDE (return). The original client equivalent spans 0x20F670
through 0x20F92E (return). Each contains precisely 39 ordered calls
to its local registration helper, with identical named message and
signed payload-size arguments. The server helper is RVA 0x229A0;
the client helper is RVA 0x1D0280. -1 denotes variable-length data.

The original server registration helper inserts a freshly allocated
record through the tree insertion helper at 0x22830. Its allocator
at 0x21930 assigns the next sequential index when the free list is
empty. This agrees with the owner's SDK 2004 CUtlDict and CUtlRBTree
implementations. The registration path starts with a fresh catalog
and contains no removal calls; the ordered records define the IDs.

The original ServerGameDLL003 slot 28 at RVA 0x1E7C80 tests message
index validity, copies its name with the bounded string-copy helper,
and returns the corresponding size through an output reference.
It returns false without modifying outputs for an invalid index.
The SDK 2004 GetUserMessageInfo signature corroborates the native
four-argument ABI and callee stack cleanup of 16 bytes.

## Independent implementation

sin_user_message_catalog.h contains the recovered factual protocol
metadata, not original code or packets. It is reusable by both
independent DLLs. The actual server slot 28 now performs the lookup,
including positive-capacity truncation and NUL termination. Null and
nonpositive-capacity buffers are explicitly refused by the independent
implementation as additional defensive checks.

The native DLL test reads all 39 records through the returned vtable,
tests invalid IDs, output preservation, short buffers, and invokes slot
28 directly with x86 stack and return-value checks.

Inspect-OwnerOriginalUserMessages.py verifies both SHA pins, exact
registration receivers and call targets, signed sizes, complete ranges,
and equality against the independent catalog. Its public synthetic
self-test uses only invented registration records.

Notable game-specific fields include ArenaMode, ArenaUpdate,
DynamicDecision, HUDCamera, GamePlayVarValue, Progression, DDInfo,
DDAdvisorScores and PoliceRadioMessage. Original order is essential:
substituting stock HL2 registration would disagree with the retail
client. The catalog does not implement serialization, handlers, dynamic
difficulty decisions, or actual network transmission. The independent
DLLs still deliberately refuse full initialization. No original DLL or
commercial implementation bodies are included.
