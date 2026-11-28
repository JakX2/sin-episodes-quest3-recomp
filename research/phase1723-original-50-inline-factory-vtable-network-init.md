# Phase 1723: 50 original inline entity factories share network-init virtual method

Original unchanged SiN Episodes (2006) PE32 client.dll SHA256:
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

## All 50 original primary and secondary vtable dependencies

Phase1722 recovered 50 complete 86-byte original entity creator methods.
Each writes four original 32-bit vtable pointer labels to the allocated
entity at offsets +0, +4, +8, +12 before calling a first-vtable virtual
method through **byte offset +0x94**. The first vtable address differs
for each class, but **all 50 primary vtable +0x94 entries resolve to
the same original source method `0x240C8FE0`**.

The native method is a complete **75-byte PE32 executable function**
SHA256 `fc701a745f638b46ebb9bacae97369d2a46545dd9bef4351e500ecc8d20bbe92`.
It ends with original `RET 8` and is followed by 5 `INT3` bytes.
Unlike a copied integer, it performs actual engine-dependent
initialization and cannot be modeled as a pointer-free host callback.

The original 75-byte method source code obtains an interface vtable
from object +4 and dispatches its first method with both network
arguments. It reads engine global `0x242FA738`, calls helper
`0x241280A0`, runs helper `0x24119570` on the owner subobject
at +0x16C, invokes the first vtable's method at byte offset +0x44,
passes that result to source helper `0x240C8E90`, stores the first
network input at object field +0x50, and returns `AL=1`.
These are original observed instructions and dependencies, not yet
usable native replacements.

Each second original vtable at object +4 has a first method
resolving to the same **4-byte source method `0x24054EC0`**, whose
complete instructions are `LEA EAX,[ECX-4]; RET`, followed by
12 `INT3` bytes. Complete SHA256:
`b8bcfc02fb79817e275fdfb8a7c0c73dc6dff41e2b950288a2668b7d0ff6e92d`.
This proves a native adjustor from an interior interface at
object+4 back to the owner base. It is not a generally valid
host pointer adjustment.

The primary vtable byte+0x44 targets split among exactly three
original methods:

| Source method VA | Number of 86-byte entity factories |
| --- | ---: |
| `0x24062070` | 34 |
| `0x24054E80` | 13 |
| `0x240982D0` | 3 |

SHA256 of all 50 source primary/secondary vtable-pointer DWORD pairs
in original class-linked order:
`42db228cc0ea0cb4c213d46e1140656d0dea885e0cd8d56994ff472dbc4dc02e`.

SHA256 of all 50 virtual target triples (primary+0x94,
secondary+0x0, primary+0x44):
`ccfbb162a58b55ea5be781e55cef7b386a06327ab06b8b4950707a9d4b725413`.

## Portable source-only dependency preflight

`include/sin_compat/original_client_inline_factory_vtable_init.hpp`
maps the exact 50 source classes, original creator identities,
three primary virtual+0x44 targets, shared network init method
and secondary adjustor identity. It reports source object offsets
(+4 for secondary interface, +8 returned networkable interior
pointer, +0x50 mutated owner field) without executing any of them.
`CanInvokeRetailVirtualMethods()` remains false.

`research/tools/audit_sin_original_client_inline_factory_vtable_init.py`
has public --self-test mode verifying all original-free source
constructor and routing metadata against Phase1722. Owner-private
--client authenticates the unchanged original DLL SHA256, complete
75/4-byte original functions, sparse instructions, source helper
calls, original RTTI vtable address slots and original pair/target SHA.

Public check:
`python research/tools/audit_sin_original_client_inline_factory_vtable_init.py --self-test`

Optional private original-DLL check:
`python research/tools/audit_sin_original_client_inline_factory_vtable_init.py --client /private/client.dll`

`tests/test_original_client_inline_factory_vtable_init.cpp`
cross-checks all 50 source classes and the three method groups,
141 excluded original classes, exact original object interface
offsets, correct source identity rejection, and NULL native
creator pointers in the owned 20-byte ClientClass image.
Strict C++17, Linux ASan/UBSan, Windows MSVC Win32 and
Android CMake builds include it.

## Why this matters for real boot

The original Source virtual `+0x94` is a real multi-call engine
network initialization path. To supply executable replacements
the port will need host/native interface implementations at the
correct object offsets, real callbacks for the original +0x44
groups, and valid object lifetime. Simply returning the fake
source method address would crash the original engine.

Retail VClient011.slot5 remains guarded. No actual 2006 PE32
executable bytes are committed, and no commercial game DLL
was modified. This is not a Windows game boot, real packet
decode, Android/Quest 3 gameplay, renderer or physics milestone.
