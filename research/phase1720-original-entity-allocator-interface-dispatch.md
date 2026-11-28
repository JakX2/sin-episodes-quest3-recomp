# Phase 1720: original 2006 PE32 allocator interface dispatch

Unchanged owner original SiN Episodes client.dll SHA256:
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

## Exact source functions

The original zero-initializing wrapper from Phase1719 calls
`0x2416F8F0`, a **complete 79-byte original PE32 method**
SHA256 `e4682f7b52bf956d9769e11f7390b3f6b17076d25e36f35951211b5a6e1fe82a`.
It is **not** a normal portable malloc function.

The native method checks bit0 of original writable global `0x2437290C`.
If unset it marks that bit, calls original native lazy context resolver
`0x2416F7F0`, and saves its pointer result at `0x24372908`.
On subsequent calls it reads the cached context pointer.
The original resolver is a distinct complete **90-byte method** SHA256
`c2a27c407aa1fa127dc525282abb96d283e1026826f428c0855df9480123cce0`.
It checks an engine allocator vtable method at byte offset `+0x5C`,
and invokes original Windows imports when creating its context.
If unavailable, it returns NULL.

The original allocator dispatcher dereferences the global engine allocator
interface slot `0x242761CC` and selects one of two virtual methods:

- Null context: original vtable byte offset **+4**, passing allocation size.
- Non-null context: original vtable byte offset **+0**, passing allocation
  size, context pointer, and source flags=0.

This branch structure, native globals, complete 79/90-byte method SHA256
and original x86 call/opcode witnesses are authenticated by the private
source audit. The original owner allocator service, context memory and
Windows functions cannot be used directly on Android or in a replacement
client.dll.

## Safe bounded source-only implementation

`include/sin_compat/original_client_source_allocator_dispatch.hpp`
implements **only an allocation route planner**. It requires an exact
source-authenticated one of 142 entity creators and explicit host engine
binding. If the lazy context is unresolved, it reports a required resolver
rather than guessing. Null initialized context selects original vtable+4.
Only 4-aligned synthetic context tokens in `0x10000000..0x1FFFFFFF`
can select original vtable+0; original PE32 pointers are refused.
Seven special entity creators and 42 temp-event getters remain unsupported.
No actual vtable, original allocator, creator or Windows import is called.

Public original-free audit:
`python research/tools/audit_sin_original_client_allocator_dispatch.py --self-test`

Optional private original-DLL SHA audit:
`python research/tools/audit_sin_original_client_allocator_dispatch.py --client /private/client.dll`

Native C++17 regression:
`tests/test_original_client_source_allocator_dispatch.cpp`.
Tests both source branches, lazy-init request, unbound engine refusal,
unsafe pointer refusal, all 142 source creator sizes and 49 exclusions.
Linux strict C++17/ASan/UBSan, Windows MSVC Win32 and Android CMake
targets are wired to these host-only checks.

## Remaining real blocker

Original engine allocator ownership and lazy context initialization,
native entity constructors and destructors/vtables, receive-proxy ABI
and special arrays/overlay still require reconstruction. Every
host-owned native32 ClientClass +0/+4 creator field remains NULL.
`VClient011.slot5` stays guarded. No actual game startup, Android
or Quest 3 gameplay, physics, prop/model or rendering claims.
