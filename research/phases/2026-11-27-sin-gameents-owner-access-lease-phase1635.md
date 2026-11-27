# Phase 1635: Owner access lease across original GameEnts ABI calls

Stacked on fully verified Phase 1634 PR #1642 (commit `9534fba16466ceaafb128662300e82aa9a710a2d`). Original `ServerGameEnts001` still has a seven-entry x86 virtual table. This phase changes only the diagnostic bounded owner contract and its native Win32 regression. No retail `CreateInterface` registration.

## Problem

The Phase 1634 diagnostic wrapper checks owner-authorized edicts, unknown interfaces, entities, touch storage and transmit buffers before use. Those checks do **not** by themselves ensure that storage and owner callbacks stay valid between the check and its later field reads, virtual calls and writes. Entity removal itself intentionally retires one member of the leased object set.

## New bounded owner integration

- `GameEntsBoundedOwner` now requires `begin_access(context,slot,&token)` and `end_access(context,slot,token)` as well as prior pointer and extent authorizers. Missing callbacks make the seven-slot object `Ready()==false`.
- After checking the per-instance reentrancy guard but before validating or reading any engine-owned pointer, every operational virtual method must acquire an access lease. A failed acquire returns `GameEntsDispatchResult::LeaseDenied` without executing the operation.
- On a successful acquire, **all** paths release the exact opaque lease token once, after the operational callbacks have returned. Reentrancy remains refused through the end callback, then the active guard clears.
- Host-side contract: a successful lease must keep the original-layout edict/unknown/entity pointers, touch graph, transmission buffers and callbacks stable for the **entire** dynamic dispatch; a failed acquire must not leave a lease held. This is a protocol between trusted runtime integration and the independent diagnostic object, not a proof of live Source engine ownership.
- Diagnostic report records attempts, acquisitions, releases and denials without altering the six original method signatures or their x86 calling convention. The destructor is not leased because its owner must guarantee that the diagnostic instance itself outlives every invocation.

## Regression

Extend the existing native MSVC x86 virtual-table test with a lease-bearing owner fixture, pointer and extent validators that reject unleased access, missing begin/end callback readiness, denial before debug setter/touch/release/transmission side effects, nested interface calls during both lease acquire and release (must be refused), nonzero opaque lease identity match on release, and exact acquire/release balancing across all existing success and failure branches. Existing direct six-slot x86 vtable call/ESP checks, touch→transmit→release chain and forged pointer tests remain.

## Remaining blocker

A retail integration must independently prove a real owner lease or equivalent single-threaded engine lifecycle guarantees, not simply return `true` from `begin_access`. Real edict generation, networkable object ownership, contact allocator and bitset lifetime are still not authenticated. This code does not boot the original engine, expose a retail interface, or implement shared Source physics/surfaces/models/props.

Verification: commit-pending; check native Windows 239/239 CTest, PE32 authentication, Win32 factory compatibility and C++ reference/Android CI on the **final SHA**.
