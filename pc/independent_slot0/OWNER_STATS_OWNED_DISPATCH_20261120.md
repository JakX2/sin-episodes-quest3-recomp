# Phase 477: owned CUserMessages dispatch boundary (no retail transport)

## Previous proven position

PR #476 head 3d0b169577a8213c9b95aea20bc20a9ea12392ac reconstructs
an independent 55-static / named 11-dynamic Stats value runtime and
the SHA-authenticated ResetHUD cleanup/recreation shape. Its Windows
PE32 original-free suite passed 90/90 CTests. The owner-proven **real**
Windows engine checkpoint remains 0x53494E74. That exit came from
the file-system/owned HookMessage registration diagnostic and does
not demonstrate VClient011::Init returning success or dispatching
a single genuine network message.

## New independent adapter

The private SHA-authenticated original client disassembly identified:
- The VClient011 entry 33 at 0x240FDCD0, which delegates to
  CUserMessages::DispatchUserMessage at 0x241D0000.
- The latter invokes each registered handler with an **independent
  copy of the incoming six-dword (24-byte) bf_read reader**.
- The three Stats hooks are StaticStat=27, DynamicStat=28,
  ResetHUD=5. Their values, names and independent ResetHUD owned
  lifecycle were reconstructed separately in prior phases.

The new client_stats_owned_dispatcher.h has a bounded observer
registry and a delivery method that:

1. Initializes the previous independently owned Stats runtime;
2. accepts only those three known message IDs and a valid **owned**
   stats_bitbuf::Cursor, never a retail engine reader pointer;
3. submits a cursor copy to the stat value mutation/reset runtime;
4. **refuses** bad headers or values without notifying observers;
5. invokes each independently authored observer with its own fresh
   cursor copy, isolating consuming listeners from each other;
6. limits registrations to four owned observers for each tracked
   message, and clears them on Disable or Initialize.

The test constructs original-shape synthetic bitstreams with an
unaligned starting bit, dispatches StaticStat/DynamicStat/ResetHUD,
checks mutation, dynamic name creation, reset and reinitialization,
verifies observer ordering and independent read cursor positions,
tests negative/malformed frames and capacity bounds.

The 4-observer cap, refusal before observer fanout, and owned-session
Enable/Disable policy are **conservative independent design decisions**.
They must not be mistaken for the original CUserMessages registry's
capacity, error handling or exact callback return semantics.

## Intentional non-integration

The independent Windows client.dll's **actual** VClient011
slot 33 remains the fail-closed bool __thiscall(int,void*) implementation.
It does NOT reinterpret, copy, or invoke a caller's opaque retail
bf_read*. No unknown engine memory layout is guessed; no proprietary
original callback is executed. The user-message system has not been
wired into a normal successful client Init; the HUD and secondary
ResetHUD collection remain unresolved.

## Gates to leave this phase

- Authenticate the **24-byte retail bf_read layout and cursor
  invariants** against the real original DLL and SDK 2004 declarations,
  including bit position, limits and overflow.
- Implement a separately tested, **strictly bounded** reader adapter
  with full malformed-input and bit-unaligned trials; verify its
  Win32 calling convention at the actual interface boundary.
- Recover original CUserMessages callback registration lifecycle,
  the unimplemented client Init stages, and shutdown path.
- Only then enable original-engine message delivery and run a
  disposable-owner retail Windows checkpoint with original DLL
  restoration guaranteed. Maintain separate pass criteria for
  actual game process execution and synthetic CI.

This phase contains original-free authored code, derived metadata,
tests and docs only. Neither game DLLs nor retail packets are added.
