# Phase 1707: source-correct bounded standard STRING/DATATABLE RecvProxy behavior

## What the actual 2006 PE32 methods do

The owner-original `client.dll` is SHA256 pinned to
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

Two original 2006 executable native methods were re-audited in the
unchanged DLL and reconstructed as **original-free, host-owned
C++17 behavioral equivalents**. Neither original method is
copied into this repository as executable bytes.

| Original callback | Native bytes | Original SHA256 | Actual narrow effect |
| --- | ---: | --- | --- |
| `0x24126CA0` standard string proxy | **59** | `ce29a1a4701a85ae6174f1410b4fb592d58312adf6fc39a24b8ef508ebaf28d5` | Read destination buffer size from `RecvProp+12`, source string pointer from `CRecvProxyData+4`, copy until NUL or buffer size, always terminate output at `size-1` |
| `0x24126CE0` datatable proxy | **11** | `016529308820e753867bd2c2dc5176a2ef2d947df9772b7b58425b0192893ec1` | Store the third stack argument DWORD into storage pointed to by the second stack argument |

The source method boundaries and field accesses are checked by
`research/tools/audit_sin_original_client_standard_string_datatable_proxies.py`
against original method SHA256 and individual instruction witnesses.
Importantly, original `0x24126CA0` **does not reset intermediate
destination bytes after an early NUL**. It always forcibly NUL-terminates
the destination's final native buffer byte, preserving the rest.
The original native signed string-size check skips all writes if
`RecvProp+12 <= 0`.

## Independent portable implementation

`include/sin_compat/original_client_standard_recvproxy_host.hpp`
implements these exact observable effects without original pointers:

- `ApplyString(sourceBytes,destination,nativeStringBufferSize)`
  works with explicit owned source bytes and a byte-vector destination.
  The source slice must explicitly include bytes that the original
  receiver would have read. It refuses a truncated non-NUL-terminated
  source or undersized destination **before writing anything**;
  no unbounded `strlen`, no overrun, no guessing missing memory.
- A zero/negative native buffer size returns a genuine source no-op.
  Successful calls preserve intermediate destination bytes after an
  early terminator and force a zero byte at `capacity-1`.
- `ApplyDataTableToken(value,outToken)` reproduces the observed
  narrow 32-bit DWORD assignment, but does **not** dereference an
  original PE32 pointer or claim real entity lookup/lifetime support.

Phase251 already independently reconstructs the other **five standard
primitive copy proxies** (byte, short, int32, float, vector3). This new
phase covers the last two of the **seven source-standard proxy families**
identified in Phase1706. Such semantic coverage does **not** imply
complete networking, custom callbacks or engine ABI registration.

## Tested boundary and reproducibility

`tests/test_original_client_standard_recvproxy_host.cpp`
covers typical NUL-terminated strings, truncation, nonterminating
source bytes, native capacity 1, nonpositive capacities,
intermediate-byte preservation, source/destination shortage refusal,
embedded NUL, and datatable DWORD tokens including zero and
high-bit values. The test also requires
`CanPublishRetailProxyPointers()==false`.

New native CTest target is built by CMake on Win32 and portable builds.
The focused original receive-table workflow compiles and runs the
standalone test under strict C++17 and ASan/UBSan, runs the
original-free Python method audit and keeps existing original network
schema tests. Optional original-owner read-only verification:

```sh
python research/tools/audit_sin_original_client_standard_string_datatable_proxies.py --self-test
python research/tools/audit_sin_original_client_standard_string_datatable_proxies.py --client /private/client.dll
```

## Explicitly still unresolved

These callbacks do not implement decoded packet wire data, class factory
creators, client prediction/entity lifecycle, native RecvTable memory
ownership, 83 game-specific custom RecvProxies, the 28 specialized
constructor paths, original animation-overlay callback/child table, or
an executable `VClient011.slot5` result. Actual retail startup is
still guarded and independent game DLLs were not installed in the
user's game. Physics, surfaces, props/models and rendering remain
excluded per the user's project priority.
