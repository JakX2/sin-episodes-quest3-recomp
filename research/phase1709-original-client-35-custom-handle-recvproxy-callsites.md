# Phase 1709: reconstruct 35 original PE32 custom handle RecvProxy assignments

## Original source proof

The owner's unchanged 2006 PE32 `client.dll` SHA256 is
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

Two native custom RecvProxy methods
`0x241A8DB0` (**30** canonical source assignments) and
`0x241A8DF0` (**5** canonical `moveparent` source assignments)
have **byte-identical complete 49-byte executable bodies** with
SHA256
`a67004960a3f69d9cfbf0662c7a7f2cc5016d0f14d8f277fbd748507314a8067`.
Each ends with original `RET` and 15 original `INT3` padding bytes.
Both decode the source DWORD at original `CRecvProxyData+4`;
both write a single DWORD to the output pointer (original third
stack argument).

The original functions first compare incoming DWORD against
**`0x001FFFFF`**, writing **`0xFFFFFFFF`** for that one
special invalid-handle value. Otherwise they execute:

```text
mov eax, [source+4]
mov ecx, eax
sar ecx, 11        ; signed arithmetic shift right
shl ecx, 12
and eax, 0x7FF     ; original low 11-bit index
or  ecx, eax
mov [output], ecx
```

Modulo native 32-bit arithmetic, the source output is:

```text
if incoming == 0x001FFFFF:
    output = 0xFFFFFFFF
else:
    output = ((incoming << 1) & 0xFFFFF000) | (incoming & 0x7FF)
```

The phase's independent C++17 regression reproduces the original
`SAR` sign extension explicitly using unsigned integers, checks
65,536 low source values, more than 100,000 deterministic high-bit
samples, boundary and invalid sentinel cases. Source-private audit
SHA-pins both actual native 49-byte methods and validates the
source allocation (30 entity handles and 5 moveparent fields).
Their two original PE32 function addresses remain *labels*, not
executable pointers that can be copied into an Android client.

## Bounded owned-object dispatch

`include/sin_compat/original_client_custom_handle_proxy.hpp`
now provides pure host-side `DecodeSourceHandle`. Existing
`original_client_standard_recvproxy_router.hpp` uses that
function only when the original source callback VA is one of
the **two authenticated** handle methods. It extracts the native
DWORD little-endian from the host-owned decoded data `+4`,
checks the original property `+44` offset and 4-byte destination
capacity before writing, and does not invent an entity lookup or
register a live proxy.

On source metadata for all 1,110 static properties:

| Original source callback class | Count | Host reconstruction status |
| --- | ---: | --- |
| Seven source-standard proxy families | 999 | Bounded original source-aware decoder receiver semantics |
| Two authenticated custom handle methods | 35 | Newly reconstructed packed handle/invalid sentinel transform |
| Remaining custom RecvProxy methods | 48 | Explicit unsupported outcome |
| Special type4/array/animation-overlay constructor routes | 28 | Explicit unsupported outcome |

That yields **1,034 source callback assignments with host-owned
behavior**, not 1,034 native retail engine callbacks and not a
complete Source network protocol.

The cross-platform `test_original_client_standard_recvproxy_router.cpp`
now checks both original custom callback identities, native member
offsets and byte-level sentinel/handle remap with neighboring-byte
preservation. It traverses all authentic 1,110 source property slots
and refuses unsupported 48+28 routes, rather than silently falling
back to raw byte-copy. An additional focused C++17
`test_original_client_custom_handle_proxy.cpp` confirms all native
arithmetic and cannot publish retail callback addresses.

Reproducible source audit, with no original DLL published:

```sh
python research/tools/audit_sin_original_client_custom_handle_proxy.py --self-test
python research/tools/audit_sin_original_client_custom_handle_proxy.py --client /private/client.dll
```

Strict native C++17, ASan/UBSan, MSVC Win32 x86, Android ARM64 and
Bionic emulator original-free jobs are wired through the existing
CMake and CI workflows. They do **not** substitute for running an
independent replacement `client.dll` against the user's
actual 2006 retail game.

## Remaining retail ABI work

This phase does not implement 48 other custom receiver behaviors,
the 28 special constructor paths, animation-overlay table/proxy
lifetime, engine-owned entity/temp-event creators, original
receive-table pointer mutability, or real network packet decoder.
The native `VClient011.slot5` remains deliberately guarded and
all `CanExposeToRetailEngine()` remain false. No actual game map
boot or Android/Quest 3 gameplay is established.
Deferred HL2 physics/surfaces/models/props/rendering are unchanged.
