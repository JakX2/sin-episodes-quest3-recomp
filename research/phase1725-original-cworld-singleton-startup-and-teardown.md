# Phase 1725: source-authenticate original CWorld singleton startup and teardown

Unchanged user-owned 2006 PE32 client.dll whole SHA256:
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

## Critical ClientClass creator vs actual world allocation distinction

The source original **CWorld ClientClass creator** is the already
authenticated 40-byte method at `0x240FC920`. It does NOT allocate
an entity. It reads original client singleton global `0x2433F3F4`,
calls the object's primary vtable byte-offset `+0x94` method with
two network arguments, then returns an original interior networkable
pointer at base +8. It dereferences the global before the later
null comparison, so calling it while uninitialized would be unsafe.

An entirely separate client initialization routine **does allocate
the world singleton before that factory is used**. The original
client startup at `0x240FE590` calls original `0x240FCDD0`, a
complete **74-byte** native function, SHA256:
`8a9a82e9e7a8bbd9eb61c80802270817f00c8053bff3b21ba941e9c46311d289`.

It requests **0x4B0 = 1,200 bytes** from the original zeroing
allocator wrapper `0x240C4FF0`, invokes original native CWorld
constructor `0x240CCE30`, installs source RTTI/vtables at
object offsets +0, +4, +8, +12 and stores the original object
base pointer to global `0x2433F3F4`. On allocation failure,
it stores zero to that global.

Original client shutdown at `0x240FF3D4` calls original
`0x240FC900`, a complete **27-byte** native method, SHA256:
`71721cdd2db6d9210f6b335faca52209973d792a0aa0b802c0c7306241303e88`.

It tests the original world singleton pointer, invokes its
primary vtable's byte+0 virtual method with destruction flag=1
when non-null, and then clears original global `0x2433F3F4`.
The independent original getter at `0x240FCD40` is a six-byte
`MOV EAX,[0x2433F3F4]; RET`, SHA256:
`b07f64b7fbb5a63cfd4816fcc0481ce9f61be29ae5eae77d80f758bd319a0d06`.

All four complete native original methods and their RET/INT3
padding are SHA-authenticated. Original startup/shutdown E8
callsites are separately verified against the unchanged source.

## CWorld native vtable distinction

The CWorld bootstrap installs original RTTI vtables at:
`0x2429632C`, `0x242962C4`, `0x2429628C`, `0x24296274`.

Its primary virtual at byte+0x94 resolves to **0x240FCB10**.
The target is a **complete 44-byte original function**, SHA256
`305044e8173fc81a50952318532ec1a5638e4c956dc056285e048ef92448b35d`,
followed by four INT3 bytes. It first clears owner field
`+0x480`, calls source original helpers `0x24085750`
and `0x24129480`, then calls the shared 75-byte network
initializer `0x240C8FE0` and returns with `RET 8`.
This establishes a world-specific initialization preamble
followed by the common network initialization sequence.

This is NOT a direct reference to the shared 75-byte method `0x240C8FE0` used
by the 50 generic inline-vtable entity creators in Phase1723.
The world primary byte+0x44 points to `0x24054E80`.
Its SECONDARY byte+0 virtual points to the same original
`0x24054EC0` four-byte interface-adjustor thunk that
returns interface-this minus four.

This matters for porting: a fabricated World vtable copied
from a generic entity would invoke the wrong network
initializer. Both native source targets require their own
portable implementation and correct 2006 object layout.

## Source-only host preflight, no executable original objects

`include/sin_compat/original_client_world_singleton_lifecycle.hpp`
provides original source identity and a strict host-only
World lifetime dependency plan. It refuses to publish native
creator pointers, original old-global addresses, null
world instances or unverified synthetic object tokens.
The plan reports the need for the original constructor,
network initialization, and native virtual destructor rather
than invoking any unsupported engine code.

Public original-free source audit:
`python research/tools/audit_sin_original_client_world_singleton_lifecycle.py --self-test`

Optional private unchanged owner original-code SHA audit:
`python research/tools/audit_sin_original_client_world_singleton_lifecycle.py --client /private/client.dll`

Test:
`tests/test_original_client_world_singleton_lifecycle.cpp`.
Tests original class ordinal/VA/source global, separate
1,200-byte startup allocation, distinct CWorld virtual+0x94
method, unbound/invalid original PE32 pointer refusal,
and all native32 ClientClass executable creator fields NULL.
Linux C++17 and ASan/UBSan, Windows Win32 and portable Android
CMake tests are wired.

## Remaining real boot barrier

Source CWorld startup/shutdown lifecycle is mapped but cannot
run until the actual engine allocator service, CWorld 2006
constructor/destructor, real RTTI/vtables, network receivers
and dependent interfaces are reconstructed. No retail original
game files were overwritten. VClient011.slot5 remains guarded;
there is no actual Windows game boot or Android/Quest 3 gameplay,
models, renderer or physics claim.
