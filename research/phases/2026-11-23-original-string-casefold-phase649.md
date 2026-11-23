# Phase 649: Original 2006 SiN string casefold and locale dispatch

## Scope, exact owner provenance, and priorities

The owner's objective remains recovering missing **SiN Episodes: Emergence gameplay/entity/NPC behavior** from the 2006 original `server.dll` and `client.dll`. The already-functioning Half-Life 2-equivalent physics, surfaces/materials, model/prop reading/loading/rendering are explicitly out of scope. The string comparison below is a **narrow required dependency of the original symbol-registration path**, not an invitation to rebuild Source/CRT generally.

Both original unchanged retail binaries were verified by their complete SHA256:

- `server.dll`: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- `client.dll`: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

**Three complete original native methods, 358 executable bytes**, were authenticated:

| Original function | Complete native size | Original whole-method SHA256 |
|---|---:|---|
| `0x223A5574` mode-switching comparator | 67 | `d108c0f0291d63e422757a9255fa568f383a772601ba2a66a6f2b79ca9fe7f29` |
| `0x223A8410` ASCII fallback comparator | 78 | `9208efe7b20ebf707490aa3334dabf576a42bfabcb93356dfd2505c20dd6d658` |
| `0x223A72DA` character lowercasing/locale transform | 213 | `5b0ce7c127d7a1ed81c122de7929ff525b2dfff8b1855c4d3a4eaaea4f315dba` |

The private auditor checks both whole original DLL identities, all three **entire executable body hashes**, **4 exact original rel32 call edges**, **22 original branch destinations** (including the jump into the ASCII fallback), **76 sparse address-pinned instruction witnesses**, exact original executable section, and post-body boundaries. Not all these CRT-style helper methods have post-RET INT3 padding: `0x223A55B7` and `0x223A845E` are adjacent *valid function entries*, while `0x223A73AF` is INT3. This distinction is authenticated rather than inventing boundaries.

The original proprietary DLLs, complete code bytes and Ghidra exports are **not** committed.

## Native 0x223A5574 control flow

This original method is cdecl `compare(const char* left,const char* right)` and checks the **DWORD at absolute 0x2259F500**:

- If zero, executes an original near **JMP to `0x223A8410`**. This is a tail-jump, not a rel32 CALL.
- If nonzero, loops over unsigned source BYTEs. Calls original `0x223A72DA` on the left byte and again on the right byte, with two original relative calls at `0x223A5591` and `0x223A559D`. Stops when transformed **left** character is 0 or transformed characters differ, then returns the **signed 32-bit subtraction of the transformed values** `left_transformed - right_transformed`. Otherwise it continues.
- The source reads unsigned BYTEs before the transformation; it does not read UTF-8 codepoints or accept arbitrary host `std::locale` abstractions.

A reference avoids signed-overflow UB by performing the x86 wrapping 32-bit subtraction as unsigned bits, then converting to the equivalent signed 32-bit result.

## Native 0x223A8410 zero-mode ASCII comparator

The original fallback is a standalone 78-byte x86 cdecl routine. It compares source bytes directly, and if they differ transforms only **A..Z** to lowercase by adding 0x20. Its return is:

- **−1** if the first differing folded unsigned byte on left is smaller.
- **0** if equal up to the null terminator.
- **+1** if larger.

It loops without a locale callback. Bytes >= 0x80 are ordered as unsigned; for ASCII letters the comparison is case-insensitive. This routine compares the original raw bytes first, avoiding fold operations if equal, just as the retail code does.

## Native 0x223A72DA character transformation

The entire 213-byte body is authenticated. Portable reimplementation models the confirmed branches, state reads and call boundaries, while **not inventing** its remaining locale-dependent native callees.

- Checks `DWORD[0x2259F500]` first. If zero, performs direct ASCII A..Z + 0x20 mapping and otherwise returns the original 32-bit input.
- With a nonzero locale, checks `DWORD[0x2253D1E0]`; when nonzero and input <=127 unsigned, takes the same ASCII fast branch.
- Otherwise if input <256, evaluates the lowercase class bit: signed `DWORD[0x2253D208]` >1 delegates to original `0x223A898A(input,1)`; else reads bit0 of the original unsigned `BYTE[DWORD[0x2253D200]+2*input]`. If the class bit is zero, returns the input unchanged. Character inputs >=256 bypass that class test.
- Checks high-byte lead marker bit7 of `BYTE[ctype_table+2*((input>>8)&255)+1]`. For a marked lead byte, constructs a two-byte character `(input>>8,input&255)`; otherwise constructs a single-byte character.
- Calls original `0x223AC9E7` with locale-mode/options=256, the local byte buffer, byte count 1 or 2, output buffer of size 3, codepage `DWORD[0x2259F510]`, and final option 1. If returned count is zero, uses original input; if 1, returns output byte0; otherwise joins output bytes 0 and 1 in big-endian order as the original registers do.
- The actual internals of original `0x223A898A` and **locale-mapping `0x223AC9E7`** remain typed required host callbacks. Their complexity includes CRT-like classification and external Windows locale settings. No claim that synthetic ASCII behavior implements real international comparison rules.

## C++17 implementation and synthetic verification

Introduced `Casefold::compare` and `Casefold::transform` with a typed `IHost` guest-memory API, original state variable addresses and explicit callbacks. Callback failure, guest-address overflow, bounded iteration and reentrancy are defensive portability safeguards rather than behavior observed in native x86 crash cases.

Local strict C++17 `-Wall -Wextra -Wpedantic -Werror` and independent ASan+UBSan regression suites pass. Tests include:

- ASCII case equality, mismatches, greater/less, prefix ordering, null termination, high unsigned byte 0xFF, wrong guest pointers.
- Nonzero locale mode still taking the fast ASCII path, transformed subtraction (not just {-1,0,1}), original source-iteration counts and left-string termination.
- CType array bit masks, separate classification-helper path, DBCS lead byte high-bit and two-byte requests, wrapped output, mapping zero return, host exceptions/failures and comparator reentrancy.
- Real **Phase648 `0x2228E9B0` original nested-map comparator model → Phase649 `0x223A5574` model** integration using guest string-reference records with block pointer and offset.
- Expanded compiled **Phase638–649** synthetic symbol-registration tree test: the Phase648 nested string comparator host callback now delegates to the actual compiled Phase649 source reference instead of assigning an invented fixed negative result. The outer original comparator/tree insertion still uses 39 invented symbol IDs, previous original wrappers and synthetic host memory. The truly original Phase638 nested lookup `0x2228F7A0` remains a distinct unresolved callback.

Portable Linux CTest, native Windows Win32 x86 CTest, Android NDK ARM64 compilation and real Android x86_64 Bionic `dlopen` smoke are configured in the PR, with results recorded against the exact SHA once CI completes.

### Private owner original audit

`python research/tools/audit_original_string_casefold_phase649.py --server ORIGINAL_SERVER_DLL --client ORIGINAL_CLIENT_DLL`

Public invented-byte mutation regression:

`python research/tools/audit_original_string_casefold_phase649.py --self-test`

## Remaining original SiN-specific dependencies and caveats

The remaining locale calls `0x223A898A` and `0x223AC9E7` are **deliberately opaque host boundaries**, as is the original **`0x2228F7A0` nested symbol lookup**. The latter is the next useful gameplay-registration focus rather than spending more research cycles reconstructing generic CRT locale infrastructure, unless a native test proves that dependency is blocking SiN-level behavior.

This phase recovers the original comparison dispatch and fallback source, **not** original in-game NPC execution, a playable Windows DLL replacement, Android levels or Quest 3 VR gameplay.
