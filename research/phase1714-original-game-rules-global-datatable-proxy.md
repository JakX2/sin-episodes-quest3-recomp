# Phase 1714: original 2006 PE32 GameRules global-to-output receiver

## Source-authenticated native callback

The user's unmodified retail SiN Episodes `client.dll` SHA256:
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

The original source **`DT_SE1Proxy.se1_gamerules_data`**
(table 64, property slot 1, native type5, member offset +44 zero)
uses a **single** original native callback at `0x24212E20`.
It is a complete **13-byte executable** function, full SHA256
`1b9fa5d21adeab9069a2d97339fed5cfa6e42055c079877dbe5fe7d6a2461043`.
It terminates with `RET`, followed by three `INT3` padding bytes,
then a different method begins at `0x24212E30`.

Its complete behavior is four source x86 instructions:

```asm
mov eax, [esp + 8]             ; original callback output DWORD*
mov ecx, [0x2433F3F8]         ; original client global singleton POINTER
mov [eax], ecx                 ; return the current global pointer value
ret
```

The `0x2433F3F8` address identifies the **original PE32 global
slot**, not an executable method or an address valid inside a new
client. The actual global's runtime VALUE is not known merely from
the source DLL's static data. The original constructor callback
argument was independently checked at `0x24212E75`, 14 bytes before
its property name argument at `0x24212E83`.

## Correct portable separation

`include/sin_compat/original_client_custom_gamerules_datatable_proxy.hpp`
retains the original source callback identity but **does not**
dereference the original global, copy any real Win32 process
pointer, or pretend to provide a complete engine-owned singleton.

Instead the host must supply an explicit authenticated binding:
a boolean `hasAuthenticatedGameRulesSingleton` and an opaque
`ownedGameRulesSingletonToken`. In this safe experimental ABI,
the non-null token must be 4-byte aligned and lie in the explicitly
**synthetic** host token range `0x10000000..0x1FFFFFFF`.
A null token is allowed because the original global could be null.

Without an authenticated binding, the router returns
`UnsupportedSourceCallback`. With a non-null original PE32
address, unaligned token, or out-of-range value, it returns
`RefusedBadOwnedMemory` without altering host state.

This host source-equivalent routine simply assigns the
authenticated synthetic token to its bounded output DWORD.
It neither creates a GameRules singleton nor allocates a real
2006 `CSE1GameRules` object.

## Audit, tests and source classification

Read-only optional owner-private SHA audit:

```sh
python research/tools/audit_sin_original_client_custom_gamerules_datatable_proxy.py --self-test
python research/tools/audit_sin_original_client_custom_gamerules_datatable_proxy.py --client /private/client.dll
```

The original-private mode validates the whole client SHA, the exact
13-byte function SHA, its full three `MOV`/one `RET` native opcode
witnesses, function/padding boundary, and original property
constructor `PUSH` callback identity. The public mode never
commits or reads proprietary code bytes.

`tests/test_original_client_custom_gamerules_datatable_proxy.cpp`
covers unbound refusal, explicit 4-byte-aligned synthetic pointer
admission, null output, unsafe original pointers, wrong original
table/slot/callback and object immutability. The all-1110 source
route census now checks the one host-bound original GameRules
callback separately:

| Original source callback group | Host reference coverage |
| --- | ---: |
| Standard Source identities | 999 |
| Handle and moveparent callbacks | 35 |
| Color byte reorder | 4 |
| Decrement integer | 5 |
| Movement/collision/default FOV | 3 |
| Plasma/FireSmoke x87 scale | 2 |
| Team indexed array element | 1 |
| **GameRules host-bound singleton output** | **1** |
| Remaining custom receivers | **32** |
| Specialized constructors | **28** |
| Total original static properties | **1,110** |

This produces **1,050/1,110** portable, narrowly
source-reconstructed **host callback routes**, not a working
commercial game networking engine. The original client engine's
`VClient011.slot5` remains deliberately guarded and all
real pointer-exposure functions remain false. Actual original
runtime singleton allocation/lifetime, native creator callbacks,
array/overlay special descriptors, actual packets, Windows
gameplay, Android/Quest 3 startup and physics/rendering remain
outside this phase.

Strict C++17 CTest, Linux ASan/UBSan, MSVC Win32/Android CMake
build and focused CI are wired; no commercial original binaries
are uploaded to GitHub.
