# Phase 478: authenticated retail bf_read offsets and a trusted owned snapshot

## Evidence sources and trust boundary

Private **read-only** inspection of the user's connected SiN original
SE1/bin.zip (never published to GitHub) verified both original game DLLs:

- client.dll SHA256 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
- server.dll SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650

A different loose Client.dll found in a different connected Drive
folder did **not** match the authenticated SHA and was excluded.
The SHA-pinned retail client PE32 has image base 0x24000000.
Four exact method bodies were checked by full SHA256 against the private
owner original, and their selected original field accesses audited.

## What is now actually authenticated

| Proven original 2006 method | VA | Derived evidence |
| --- | --- | --- |
| bit-word reading core | 0x240A4760 | +0: data pointer, +8: bit limit, +12: cursor position, +16: overflow flag |
| signed-long conversion helper | 0x240A5080 | 16/32-bit reader helper route |
| stat ID's 16-bit wrapper | 0x240A5600 | pushes 16 and calls signed-long conversion helper |
| CUserMessages dispatcher | 0x241D0000 | copies DWORDs 0,4,8,12,16,20 into each listener's stack-local reader; ret 8 |

These facts derive from the **authenticated original retail code**, not
from Source SDK assumptions. +0x04 byte count, +0x11 overflow-assert
flag and +0x14 debug-name pointer remain compatible **candidate**
fields following public SDK 2004 tier1/bitbuf.h. Total 24-byte
Win32 size is independently supported by the retail dispatcher's six
DWORD copy.

**Important signedness nuance:** The 16-bit original stat-ID wrapper
calls a helper whose signed-long path can sign-extend high-bit values.
This is immaterial for the 66 defined stat IDs 0..65, but the safe
independent parser deliberately refuses IDs >=66 rather than copying
unverified original behavior on malformed packets.

This establishes the data representation used by the original bit
reader and dispatcher. It does NOT prove that the replacement can
safely dereference an arbitrary engine-supplied bf_read pointer.

## Added implementation

client_bf_read_candidate_layout.h defines an x86 candidate layout
with sizeof=24 and compile-time offset checks. Its Inspect operation
takes a **trusted independently allocated byte span** and a
preexisting candidate struct. It verifies:

- the source pointer equals the independently trusted span's base;
- DWORD alignment, nonnegative sizes and cursor;
- byte capacity capped at 256 KiB and contained within trusted memory;
- bit extent does not exceed declared bytes and cursor does not exceed bits;
- reader has not already overflowed;
- refuses all malformed states without exposing a partially initialized
  owned cursor, reading arbitrary source pointers, or modifying source.

DispatchOwnedCandidate composes that snapshot with Phase 477's
independently owned message dispatcher. It supports synthetic
StaticStat=27, DynamicStat=28 and ResetHUD=5. Its native Win32 fixture
tests unaligned initial bit cursor, two independent listeners, static
mutation, malformed snapshot rejection, truncated payload rollback,
dynamic name/float update, reset/cleanup and bounded dispatch.

audit_sin_original_bf_read_candidate_layout.py has a private SHA
audit path (--archive or --client) and a synthetic original-free
--self-test path for public CI. It records method SHA expectations
and checks the selected known accessor patterns and copied DWORDs.
Public CI will **never** require, download or publish original DLLs.

## What was deliberately not changed

The production client.dll still has no successful normal
VClient011::Init path, and slot 33 remains a fail-closed method
returning false. This candidate adapter is **not wired** into actual
game transport and does not reinterpret the engine's void* as
arbitrary memory. No actual Source network packet or HUD update has
been observed.

The next meaningful verification gate is an independent Win32
**test-only** slot33 trampoline that uses *known trusted test memory*,
exercises this exact candidate layout through a compiled interface
calling convention, and never enters the production DLL artifact.
Only after an original engine read boundary can be bounded and proven
should retail packet delivery be contemplated.

Owner-retail executed checkpoint remains **0x53494E74** (three
independently owned Stats hook IDs verified via genuine engine file
system). Phase 478's local original binary research and public native
synthetic tests are separate evidence categories.
