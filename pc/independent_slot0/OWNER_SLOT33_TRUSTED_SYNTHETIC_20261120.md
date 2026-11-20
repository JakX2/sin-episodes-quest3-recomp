# Phase 479: compiled VClient011 slot33, synthetic-only reader gate

## Why this matters

Prior phases established a bounded owned Stats decoder/dispatcher and
the original client bit-reader layout, but tests had not yet passed
messages into the compiled independent DLL's actual slot33 vtable entry.

This phase adds a **third, separate, test-only DLL build** from the
same slot0.cpp, with compile definition SIN_SYNTHETIC_SLOT33_OWNED.
It is output as sin_slot33_owned_fixture.dll under a fixture directory,
not as client.dll and not under the game owner's runtime deploy folder.

The regular independent Windows PE32 client.dll and server.dll builds
remain unchanged in their observed external behavior.

## Exactly what changes in the fixture

1. The separate synthetic fixture DLL exposes test-only exports
   to initialize its own Stats system and install a **known synthetic
   24-byte reader object** along with a caller-supplied trusted byte span.
2. Its slot33 first checks whether the bf_read* argument equals the
   address of its **own installed reader copy**. All other pointers,
   including arbitrary unknown pointers, are refused *without a read*.
3. The trusted candidate is then checked by the Phase 478 adapter.
   Malformed sizes, pointer mismatch, bit positions and overflow refuse
   without publishing value mutations.
4. The Phase 477 dispatcher handles only the owned StaticStat=27,
   DynamicStat=28 and ResetHUD=5 messages, returning fixture success
   only for accepted owned messages.
5. Calling the compiled fixture's VClient011 slot1 Shutdown disables
   this owned-only session. A later explicit test initializer can
   recreate it without pretending the game Client Init succeeded.

## Native Win32 acceptance tests

test_client_slot33_synthetic_reader.cpp loads **both** distinct DLLs:

- sin_slot33_owned_fixture.dll (special test-only build);
- client.dll (standard independent fail-closed production build).

It checks correct CreateInterface(VClient011) factory objects and
that slot33 really belongs to each loaded module. A manually
assembled MSVC x86 thiscall invocation checks ESP balance after
callee ret 8, not merely the C++ method signature.

Synthetic test messages exercise bit-unaligned StaticStat ID2,
DynamicStat ID55 with independently named record, and one-byte
ResetHUD; validate value changes and recreation; reject unknown
IDs, arbitrary bf_read* addresses, truncated fields and mismatched
trusted storage; check that installer copies the 24-byte reader.
The production client.dll is tested alongside the fixture and must
continue returning false for slot33 even when handed the same
well-formed owned test reader. The standard client.dll must not
export any test-only registration entrypoints.

## Strict boundary and next retail milestone

**No original engine packet was received in this phase.** The test
enters a real PE32 vtable slot, but in a separate explicitly controlled
synthetic process, using a reader installed by the fixture itself.
That does not validate safe dereferencing of arbitrary game memory.

The regular VClient011::Init still returns 0. The actual owner's
latest independent retail checkpoint remains 0x53494E74, and the
real game has not booted with independent DLLs. The original
CUserMessages hook lifecycle, missing HUD/secondary Stats reset
collection and complete Init/shutdown sequencing remain unimplemented.

The next step toward a real engine message is to establish an
engine-owned reader lifetime/buffer trust boundary, prove it with a
disposable retail process plus verified restoration, and ensure all
necessary registered callbacks and game systems are initialized.
Do not move this test-only pointer installer into a production DLL.
