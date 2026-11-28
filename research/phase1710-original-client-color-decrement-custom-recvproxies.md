# Phase 1710: nine more source-authenticated custom RecvProxy assignments

## Exact original 2006 PE32 methods

The unchanged owner-original SiN Episodes `client.dll` SHA256 is
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

Source original native method `0x241A8BF0` is a complete **38-byte**
routine SHA256
`d60b249c0c261174c934cd38641d46656fd299741af2b43b3bc4cd7e944a5a1b`.
It reads the original decoded received DWORD at `CRecvProxyData+4`,
then stores four output bytes in this specific order:
`raw>>24`, `raw>>16`, `raw>>8`, `raw&0xff`.
For input `0x11223344`, destination bytes become
`11 22 33 44`. Source original 2006 constructor callsites use
this function on **four** property fields:
`m_clrRender` twice, `m_Color1`, `m_Color2`.

Source original native method `0x241A8C20` is a complete **17-byte**
routine SHA256
`bbea8c9b7a7fce6219d595fb3305ce6b6137f5186e171df2ab93d3fe23323582`.
It reads the incoming DWORD at `CRecvProxyData+4`, executes
`SUB ECX,1` and stores a DWORD to the output: unsigned32
`raw-1` with natural wraparound `0 => 0xFFFFFFFF`.
The original source 2006 callsites use this function on **five**
properties: two `m_nEnginePitch` fields, `m_iMaterialModel`,
`m_iClip1`, and `m_iClip2`.

The exact original byte boundaries are separately authenticated:
the color method is followed by 10 INT3 padding bytes before
the next function at `0x241A8C20`; the decrement method has
15 INT3 padding bytes before independent native function
`0x241A8C40`. Original source addresses are labels and are
**never executable in the independent Win32/Android runtime**.

## Portable original-free implementation

`include/sin_compat/original_client_custom_color_integer_proxies.hpp`
retains both source-authenticated original method identities and implements
their narrow effects with pure host C++17 `SourceColorBytes` and
`SourceDecrementDword`. No original PE32 binary bytes are committed.

Existing `original_client_standard_recvproxy_router.hpp` now dispatches
only those source-exact custom identifiers alongside the Phase1709
handle methods and the Phase1708 standard methods. It extracts
decoded-value DWORD from a host-owned `CRecvProxyData` shaped
16-byte record, checks real source `+44` member offset and
4-byte destination bounds, and modifies only the correct original
host-object bytes.

| Original callback source categories | Count | Portable status |
| --- | ---: | --- |
| Seven source-standard receiver families | 999 | Original behavior in owned memory |
| Two source custom handle/moveparent methods | 35 | Original packed handle transform |
| Color byte reorder | 4 | Source-proven byte order |
| Integer decrement | 5 | Source-proven native32 subtraction |
| Remaining custom methods | 39 | Explicit unsupported outcome |
| Native special type4/array/overlay routes | 28 | Explicit unsupported outcome |

Total host-supported source callback assignments are now
**1,043 of 1,110**. This is a *decoded-value host reference* count,
not the number of engine-callable native pointers, complete receive
tables or actual wire serialization routines. No custom callback
silently falls back to default `memcpy`.

The original-free standalone CTest
`test_original_client_custom_color_integer_proxies.cpp`
checks exact output byte order, modulo32 wrapping and all nine
source identifier references; the complete original-source dispatcher
test checks both callback families at their *original source member
offsets*, preserving neighboring bytes, and verifies all 1,110
actual property slots produce **1,043 Applied / 39 custom unsupported /
28 special unsupported**.

Optional private owner original DLL audit:

```sh
python research/tools/audit_sin_original_client_custom_color_integer_proxies.py --self-test
python research/tools/audit_sin_original_client_custom_color_integer_proxies.py --client /private/client.dll
```

It authenticates the unchanged original whole DLL, exact native
38/17-byte function bodies, original method/padding boundaries, complete
instruction sequences and four/five source callback site counts.
Strict C++17, ASan/UBSan, Windows MSVC Win32 and portable Android
NDK/Bionic build paths use the existing CI infrastructure.

## Real retail boundary

This phase does **not** make original PE32 executable callbacks callable
from the new client DLL. **`VClient011.slot5` remains deliberately guarded**,
all exposure checks remain false, and no commercial game DLL was swapped
in the owner's installation. Runtime entity/temp-event creators,
animation overlay specialization, native callback ownership and
actual Source network packets are still unresolved. Physics/surfaces,
models/props and rendering remain excluded per project priority.
