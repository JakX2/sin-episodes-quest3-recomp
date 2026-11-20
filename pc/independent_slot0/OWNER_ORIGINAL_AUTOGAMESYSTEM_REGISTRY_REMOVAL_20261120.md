# Original 2006 AutoGameSystem destructor registry removal

## Why this advances real game-system lifecycle support

The previous original C_SoundscapeSystem deleting-destructor
analysis identified a call to the base Source game-system
destructor at 0x24156820. This stage traces **that actual
base destructor** and the registry-removal method it invokes,
rather than assuming an engine global system can simply be freed.

Original client.dll from the user's private bin.zip is
authenticated with SHA256:

6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.

| Original method | VA | Complete bytes | SHA256 |
| --- | --- | ---: | --- |
| AutoGameSystem base destructor | 0x24156820 | 26 | cecc59ef0571ba365a6ce38669ea8c6a229ce3669a8f9f00bc27d06b3e892b8a |
| Global registry first-match removal | 0x241565E0 | 90 | f71baaca312397a0a87e72799b2b20bb69214751462124cd73e3b4e19b1f0807 |

The original base destructor:

1. Writes original base game-system vtable
   **0x24293B1C** to object offset +0.
2. Takes the address of a local variable containing
   this object's pointer.
3. Calls 0x241565E0 on original global registry
   **0x242FBFA8**, with that pointer address.
4. Returns. There is **no owner operator delete**
   in this 26-byte method. Optional object deallocation
   occurs in the previously audited derived vtable slot11.

The original global registry uses an array pointer at
registry+0x00 and active count at registry+0x0C,
corresponding to original global count address
**0x242FBFB4**.

The 90-byte removal method:

1. If count<=0, return with no mutation.
2. Iterate array entries, comparing 32-bit object
   pointer identity to the caller-supplied object pointer.
3. If the pointer is not found, return without changing count.
4. On the **first** matching pointer, compute number of
   trailing entries and call original memmove helper
   **0x24248B50** for four bytes per trailing element.
   This shifts later entries down and **preserves order**.
5. Decrement the active count by exactly one.
   It does NOT clear the unused trailing raw array slot.
   If there are duplicate identical pointers, the first
   matching occurrence is removed; others remain.

The source private auditor
audit_sin_original_autogamesystem_registry_remove.py
validates the complete original retail DLL SHA and both
complete method SHAs, plus selected exact opcode windows
and the two direct call targets (registry removal and
memmove). The GitHub CI self-test fabricates opcodes
and rejects corrupted windows; no original proprietary
code bytes are committed.

## Independently owned implementation

client_original_autogamesystem_registry_remove.h
implements a bounded **independent-only** registry that
uses synthetic 32-bit identity tokens rather than engine
addresses. It scans for first pointer-identity match,
shifts subsequent tokens one entry left while retaining
order, decrements count once and preserves the untouched
trailing backing slot just like the original method.

The normal registry capacity and invalid-input guard
are *owned* safety policies. No original 2006 memory,
Source allocator or actual global registry is accessed.

The typed owned base destructor writes original
base-vtable metadata first, invokes synthetic first-match
registry removal using the owned object identity and
then records a completed independent teardown. A
repeat owned destructor refuses rather than simulating
a dangerous second destruction; the original 26-byte
method does not establish this double-call guard.

Native Win32 fixture
test_client_original_autogamesystem_registry_remove.cpp
checks empty/not-found removal, multiple duplicate
identities, stable ordering and memmove count,
unchanged stale trailing slot, base-vtable transition
even when the object was not registered, repeated
owned teardown refusal and bounded staging safety.

## What remains

This source-derived registry removal is a necessary
lifecycle piece, **not** a working engine registry.
The original registry list-registration path, method
ordering across all client game-system instances,
Source global object ownership and actual
VClient011::Init registration are still incomplete.
The original soundscape base/destroy methods are not
activated in normal Windows gameplay.

Last owner-observed actual retail Windows checkpoint
remains 0x53494E74; optional soundscape
0x53494E76/77 and CVar owner probes remain untested
in the owner's real engine process. The Android NDK
and emulator CI tests are build/loader smoke tests
only, not a Quest 3 port or running game.
