# Original 2006 SoundscapeSystem vtable slots 2 and 11

## Source evidence and class ownership

This stage follows the original constructor/12-slot vtable (PR #499)
and its 223-byte auxiliary cleanup helper (PR #506). The source was
read directly from the owner's SHA-verified SiN Episodes 2006
client.dll in the connected bin.zip. The original whole client SHA256:

6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d

| Original virtual entry | VA | Complete body | SHA256 |
| --- | --- | ---: | --- |
| vtable slot 2 | 0x240F3DF0 | 22 bytes | f585618e3c7645a7729ed7b27d96f10ede980d57f439eb6672308c1b1db21331 |
| vtable slot 11 | 0x240F3E20 | 76 bytes | a46d83727243a32da487b7794d25b7fee70ef1ac8969a3d266dc9042e878e189 |

Both entries are authenticated at original vtable
0x24293C34+slot*4, including original code SHAs and selected
x86 instruction/call-target witnesses.

### Virtual slot 2: not a destructor

The 22-byte body at 0x240F3DF0 executes:
1. Virtual CALL through this->vtable at byte +0x04 (slot 1).
2. Virtual CALL through this->vtable at byte +0x00 (slot 0).
3. Tail JMP into original helper at 0x240F37D0.
 
The original vtable identifies slot 1 as the soundscape Shutdown
method and slot 0 as Init. The wrapper executes them in that order
and then forwards this to a separate method, which probes the
current engine interface and can traverse registered soundscapes.
It does not directly delete the object.

Do not substitute direct static calls for the two virtual
dispatches or assume this method can be safely invoked during
the independent DLL's currently incomplete normal startup.

### Virtual slot 11: deleting-destructor shape

The source at 0x240F3E20:
1. Assigns the derived original vtable 0x24293C34 to this+0.
2. Invokes subobject cleanup function 0x240F3AD0 with this+0xB4.
3. Invokes subobject cleanup function 0x240F3A20 with this+0xA0.
4. Invokes subobject cleanup function 0x240F3970 with this+0x1C.
5. Invokes the SAME subobject cleanup with this+0x08.
6. Invokes base destructor at 0x24156820, which changes the
   base vtable and manipulates the original game-system registry.
7. Only if the low bit of the passed flags is set, invokes
   original operator delete at 0x2416FD60 with this.
8. Returns this and cleans up its 4-byte explicit argument
   via ret 4.

The child cleanup function at 0x240F3AD0 is *distinct*
from PR #506's auxiliary helper 0x240F3D10. The separate
child functions use original allocator 0x2416F940 for
their subarrays; their full behavior must be established
before any real memory management is activated.

These function identities and branch targets are full-byte
SHA-checked privately with
audit_sin_original_soundscape_vtable_lifecycle.py --archive.
The public original-free --self-test fabricates the selected
opcodes and refuses mutated wrappers, and never uploads game
DLL bytes into GitHub CI.

## Owned source-order lifecycle model

client_soundscape_original_vtable_lifecycle_owned.h models
the source call ordering as typed, bounded, non-pointer
events. It cannot call original virtual functions, mutate
original game objects or free retail engine memory.

For slot 2, it generates the exact virtual
slot1 -> virtual slot0 -> tail helper three-event chain.

For slot 11 it generates the exact vtable-reset, reverse
subobject cleanup, base cleanup, optional original delete,
return-this seven- or eight-event chain. Deallocation is
selected by flags bit0 only; unrelated flag bits do not
trigger deletion.

A two-phase plan contains the owned state revision, source
event identities and optional-delete bit. State commits
require byte-for-byte matching source-order events; modified
plans, invalid operation enums, impossible slot2 delete
flags and stale plans are rejected. Reset deliberately
increments the generation to prevent a queued plan from a
previous synthetic object's lifetime being reused.

The native Win32 source-free fixture covers call/cleanup
order, both deleting-destructor branches, vtable writes,
child order, stale and forged plans, double destruction
refusal and cross-lifetime reset safety.

## What this does NOT mean

The independent model is not a native engine-compatible
C_SoundscapeSystem allocation or implementation. No real
Source allocator is called, no original HWND/ConVar/KeyValues
pointer is used, and no genuine virtual call is executed.
The original engine object size, base game-system registration,
complete child array destructors and per-frame soundscape
update remain incomplete.

Normal VClient011::Init still fails closed. The last
user-observed retail checkpoint remains 0x53494E74.
Prepared diagnostics 0x53494E76/77 and the optional
read-only ConVar lookup have not been observed in a
real owner retail process in this session. This work
does not claim gameplay, sound playback or Quest 3 boot.
