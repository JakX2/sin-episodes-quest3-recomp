# Phase 277: actual x86 virtual dispatch through an independently built DLL

## Evidence gap

Before this phase the independently built Win32 `CreateInterface` DLL
returned 8-byte **plain-data sentinels**. The Windows loader smoke verified
export names, return status, object identity and 32 load/unload cycles,
but it never invoked even a synthetic C++ virtual function through a vtable.

This phase adds a **separate** independent synthetic PE32 DLL and a native
MSVC x86 executable. Both use a deliberately synthetic,
`SIN_RECON_VTABLE_TEST001` interface. The DLL returns a C++ object with
three pure virtual methods in an exact known test order:

- `read_value()`: read mutable per-object state;
- `add_value(int)`: mutate and return that state, testing one x86 stack
  argument while MSVC supplies `this` in ECX;
- `signature()`: identify the same instance.

The smoke loads the DLL via `LoadLibraryA`, resolves the undecorated
`CreateInterface` export, and checks that the object's vtable and all
three vtable method pointers reside inside the independent loaded module.
It invokes all three methods through C++ virtual dispatch, verifies
initial state and positive/negative arguments, and validates object
identity across a secondary reference-counted load. Sixteen unload
and reload cycles detect stale/static-object initialization errors.

The test DLL **rejects** `VClient011`, null names and all other retail
interfaces. No methods from any original game DLL are called or forwarded.

## GitHub Windows x86 validation

The existing `independent-factory-smoke.yml` Windows x86 job compiles
the new module and caller using MSVC x86, runs the native executable,
checks PE32 x86 machine type and undecorated factory export, and applies
both independent PE import audits (including delay import and forwarders).
No original game DLL bytes are needed or published.

This is a runtime/ABI-mechanics milestone only, NOT original SiN x86
interface layout validation. A successful synthetic call does not prove
any method position, object size, calling convention, `Init()` contract
or bootstrap behavior for `VClient011`, `ServerGameDLL003` or another
retail interface. An independent playable Windows build remains unproven.

## Next step

Obtain actual owner-original 2006 interface-object/vtable evidence via
Phase 274/275, verify specific original MSVC vtable slots and method
signatures, then replace the synthetic-only interface with a separately
reviewed implementation and test under the real engine. Original code
and pseudocode remain owner-private.
