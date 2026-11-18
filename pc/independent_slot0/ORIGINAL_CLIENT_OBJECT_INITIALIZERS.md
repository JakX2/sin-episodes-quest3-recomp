# Phase 346: client deferred object initializer lifecycle

Owner original client.dll SHA256:
6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.
Read-only disassembly and the owner SDK 2004 cl_dll/initializer.h/cpp
corroborate the deferred-object registry immediately after console
registration in VClient011::Init.

| Operation | RVA |
| --- | --- |
| Four-pointer record constructor/prepend | 0x1640C0 |
| FreeAllObjects | 0x1640F0 |
| InitializeAllObjects | 0x164120 |
| Call from Init | 0xFE39B |
| Failure cleanup call | 0x16414D |
| Shutdown cleanup call | 0xFF422 |
| List head (zero-filled original image storage) | 0x369374 |

An original 16-byte record contains destination (void**), creation
cdecl callback, deletion cdecl callback and next pointer at offsets
0,4,8,12. Initialization walks head to tail. A successful creation
writes the returned pointer into its destination; a function-only
entry has a null destination and returns null. A required creation
returning null triggers cleanup of all entries. Cleanup is also
head-to-tail, and invokes a required entry's deleter even when the
current value is null, then clears that destination. This includes
unvisited required entries on failure. The original constructor
prepends records. No direct rel32 call/jump or absolute constructor
address reference was found in the mapped raw text/rdata/data
sections, and the list head starts zero-filled. This does NOT prove
the live retail registry is empty or that no indirect constructor
exists.

client_object_initializer_core.h independently implements these
behaviors on caller-owned records, exact x86 offsets and cdecl
callbacks. It prevalidates a bounded list, unique destinations and
initially null storage, rejects reentry and malformed chains, and
refuses a non-null result from a function-only entry rather than
dereferencing a null destination. No original addresses are invoked.
The test exercises prepend order, required and optional callbacks,
success, failure with null-slot cleanup, explicit cleanup, duplicate
and cyclic inputs, already populated destinations, empty list and
malformed optional result. The original metadata inspector authenticates
the call edges and reports direct constructor-reference counts.

This is a native owned implementation, not yet integrated into
the live client startup path. The original console registry,
particle manager (next call at 0xFE3B9), VGUI and game systems
remain incomplete. Do not report complete Init. No original
DLL or commercial implementation bodies are committed.
