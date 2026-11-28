# Phase 1713: original Team player_array_element callback with bounded synthetic pointer

## Source identity and exact original 2006 PE32 evidence

Unchanged owner `client.dll` SHA256:
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

Original native `0x240F7240` is a complete **24-byte** function
SHA256
`491da928d45037adb4f7934e27130a1e5924a1013c447c2a12b30b22ad8ea859`,
followed by eight original `INT3` padding bytes before
`0x240F7260`. One and only one of the source's
**1,110 statically declared** properties uses this callback:
`DT_Team.player_array_element` (source table ordinal **81**,
property slot **3**, native type **0**, native member-offset +44 **0**).
The authentic original property constructor at `0x240F776A`
passes `0x240F7240` as its callback argument via a preceding
literal source `PUSH`.

The complete 2006 native routine has four essential reads/writes:

| Native instruction | Original source effect |
| --- | --- |
| `MOV ECX,[EAX+0x14]` | Network `CRecvProxyData` element index at **+20** |
| `MOV EDX,[EDX+0x480]` | Load an array pointer from the callback's third-argument base +**0x480** |
| `MOV EAX,[EAX+4]` | Read a decoded incoming DWORD from proxy data +**4** |
| `MOV [EDX+ECX*4],EAX` | Store the decoded value at array index times four |

The **third callback argument** is treated as an object-like base
by the original function; this phase does not prove complete
source `CRecvProxyData`/owner lifecycle or the source engine's
actual third-argument allocation contract.

The related `DT_Team` **native type4 outer descriptor**,
slot **4**, has independently authenticated source element count
**32**, original stride zero, and separate original callback
`0x240F76B0`. The latter remains **unported**. Recovering a
single inner callback does not implement the outer type4 array
deserializer or its allocation and ownership semantics.

## Portable original-free reference, with NO real pointers

`include/sin_compat/original_client_custom_team_array_element_proxy.hpp`
implements just the original indexed DWORD store, using:

- A bounded **32-element** element index; 32 and above are refused.
- The decoded original `CRecvProxyData+0x14` index as an
  **explicit host scalar**, not a silently widened 16-byte source
  record or fabricated complete Source ABI.
- An explicitly simulated 32-bit base `0x10000000`, with synthetic
  host-owned Team-array region beginning at virtual offset
  **0x1400** (purely a safety reserve, not an original 2006
  object-size claim).
- An owned host buffer containing the synthetic pointer DWORD
  at base +`0x480` and the complete 128-byte array region.
  No original Win32 pointer is ever dereferenced or published.

The adapter checks object-pointer field bounds, original array index,
synthetic token lower bound, four-byte alignment, bounded token
subtraction and the entire 128-byte array span **before modifying
any array byte**. It refuses tokens that alias object storage,
point outside the host vector, look like original PE32 addresses,
or refer to a short/incomplete allocated array.

`original_client_standard_recvproxy_router.hpp` now recognizes
this source identity and accepts an additional
`originalSourceElementIndex` decoded scalar in its independent
host input record. All existing 16-byte primitive receive-data
fields remain unchanged. The synthetic token is only consumed for
**DT_Team slot 3**. The special native type4 outer property at
**DT_Team slot 4** stays unsupported and does not fall back to
standard array `memcpy`.

## Regression testing and coverage

`tests/test_original_client_custom_team_array_element_proxy.cpp`
checks indices 0, 17 and 31, preserving adjacent bytes, rejecting
indices 32 and `0xFFFFFFFF`, wrong source table/slot/VA, stale
x86 pointer values, object-field aliasing, token misalignment and
short buffers. It explicitly verifies that the type4 outer
array route remains unsupported. A clean host synthetic token is
installed at the one original Team element route during the
full **1,110/1,110** source-catalog dispatch classification test.

Updated host-reference coverage:

| Original source callback class | Host-bounded paths |
| --- | ---: |
| Standard Source proxy identities | 999 |
| Handle and moveparent callbacks | 35 |
| Color byte reorder | 4 |
| Decrement integer | 5 |
| Movement/collision/default FOV | 3 |
| Plasma/FireSmoke x87 scale | 2 |
| **Team element index** | **1** |
| Remaining custom methods | **33** |
| Original special constructors (including Team outer array) | **28** |
| Total original source properties | **1,110** |

**1,049/1,110** original property callback *routes* now have
bounded host-side reference behavior, not a fully functioning retail
network receiver registry or 2006 game-native callback ABI.

The self-contained original-byte audit has two modes:

```sh
python research/tools/audit_sin_original_client_custom_team_array_element_proxy.py --self-test
python research/tools/audit_sin_original_client_custom_team_array_element_proxy.py --client /private/client.dll
```

The optional private mode verifies the original DLL SHA, complete
24-byte native callback hash, original `RET`/INT3 boundary,
four key indexing/storage opcodes and source constructor callback
literal. Public mode and Linux C++17/ASan/UBSan, Windows x86 and
Android CMake builds require **no proprietary executable bytes**.

## Actual runtime restrictions

`VClient011.slot5` remains deliberately guarded and all
`CanExposeToRetailEngine()` / `CanPublishRetailProxyPointers()`
methods remain false. No actual commercial Windows DLL is swapped,
and there is no real Source networking packet decoding, Team
allocator, player-array proxy ownership, entity creation, Android
APK gameplay, Quest 3 VR frame or newly implemented physics,
surfaces, models or renderer work.
