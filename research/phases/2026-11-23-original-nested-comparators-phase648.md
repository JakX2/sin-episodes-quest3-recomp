# Phase 648: Original SiN nested symbol-map comparators

## Project scope and exact owner retail evidence

Recover missing original *SiN Episodes: Emergence* (2006) gameplay/entity symbol-registration dependencies. Owner priority is **not** to repeat working HL2-equivalent physics, surfaces/materials, props, models, or rendering. No such work was done.

The unchanged owner's original retail DLLs were directly inspected:

- `server.dll` whole SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- `client.dll` whole SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

Two **complete original methods** in original retail `server.dll` authenticated by entire x86 method SHA256, method ending INT3 padding, .text provenance, **16 real branch targets**, one rel32 call to `0x223A5574`, and **97 independent address-pinned opcode witnesses**:

| Method | Exact original length | Entire original SHA256 |
|---|---:|---|
| `0x2228E910` inline bytewise comparator | **157 bytes** | `8285c595abf524eb7a9878b5d83e7712974bda42b8662529e22e44e330dacf76` |
| `0x2228E9B0` delegated string comparator | **113 bytes** | `44b4473790bc069cc4184ffff3deba0220146b714df135f3cac25e63b5e4738e` |

**Total: 270 verified original executable bytes.** Retail original binary contents and complete DLLs are **not** committed; audit against original DLL is owner-private. Public `--self-test` mutates invented method bytes only.

## Shared original comparator ABI and string reference resolution

Phase647 recovered nested map constructor `0x2228F360`, which installs comparator `0x2228E910` when selector low byte equals 0 and `0x2228E9B0` otherwise. Phase646 calls this constructor with selector=1, so the **actual nested map uses 0x2228E9B0**.

Both comparison functions use **cdecl(record_ptr_A, record_ptr_B, map_ptr)**, returning **EAX=0 or EAX=1**. Neither is an ordinary comparison of two direct char pointers. Instead, each input is a **four-byte string reference** consisting of two unsigned 16-bit fields:

- WORD `record+0`: block ID.
- WORD `record+2`: byte offset within block.

**Special sentinel rule:** only when **both** 16-bit fields are `FFFF`, the actual string pointer is the **DWORD stored at `map+0x30`**. A single FFFF by itself is *not* the special case.

**Normal string rule:** read the DWORD pointer at `map+0x1C` to the block-pointer table, index that table at `block_id × 4`, read its DWORD block pointer, then form:

`string_guest_va = block_pointer + uint16(offset) + 8`.

The additional **+8** comes directly from original native LEA instructions. Both methods resolve the first record and then the second in this fashion, preserving the separate fallback branches.

These pointer blocks are **not** the Phase647 nested slot-array itself. Phase647's separate map strings/blocks structure still needs population from the original string-registration routines.

## Original `0x2228E910`: inline unsigned bytewise comparison

The 157-byte method reads each resolved zero-terminated byte string directly:

- Compares unsigned BYTE from first pointer against BYTE from second.
- Returns **EAX=1** iff the first unequal unsigned byte of the first string is **less** than the second.
- Returns **EAX=0** on greater or equal strings.
- Checks zero byte terminators, and uses an original **two-byte-per-iteration loop**, without reading a byte after a terminating first byte.
- Has no external comparison call.

The portable source reproduces this comparison and byte-read order. The original x86 `CMP DL,[ESI]`, `SBB EAX,EAX`, `SBB EAX,-1`, `TEST EAX,EAX`, `SETL CL`, `MOV AL,CL` yield the unsigned less-than Boolean.

## Original `0x2228E9B0`: delegated comparison, not guessed strcmp

The 113-byte method resolves the same string references, then at original `0x2228EA0E` executes a native rel32 **CALL `0x223A5574`**, pushing (left_string_pointer, right_string_pointer). It cleans 8 stack bytes and does `TEST EAX,EAX`; `SETL CL`; `MOV AL,CL`. So output is **1 exactly when the comparison's 32-bit signed EAX is negative**, otherwise 0.

The original internal comparator `0x223A5574` has additional runtime state at **`0x2259F500`**. The beginning of its original disassembly checks this DWORD; when zero it jumps to `0x223A8410`, and the other path performs repeated calls to original char-transforming `0x223A72DA`. **This is not modeled as standard C `strcmp` or guaranteed ASCII case-folding.** Its exact full implementation remains a **mandatory typed `compare_original_223a5574` callback** and the next likely original reverse-engineering target.

A test-only invented callback implements a simple ASCII case-insensitive comparator to establish that Phase648 calls the delegate and uses its returned signed result. This synthetic comparator is not claimed to reproduce the real original engine string comparison.

## Code and tests

The new `Comparators` C++17 reference has explicit guest-memory host reads and direct/indirect paths. It records resolved string VAs, sentinel use, bytes compared, delegated return value, and separate failure stages. Null-pointer refusal, arithmetic overflow checks, 65536-iteration cap, host callback exception guards, and same-instance reentrancy refusal are portability-only measures, **not** original retail x86 error semantics.

Local standalone strict C++17 `-Wall -Wextra -Wpedantic -Werror` tests and AddressSanitizer/UndefinedBehaviorSanitizer tests **pass**. Test cases cover odd/even-length strings, equal strings, prefix ordering, unsigned `0xFF` vs `0x01`, null termination, normal block+offset, FFFF/FFFF fallback, rejection of lone FFFF as a sentinel, delegated signed results including INT32_MIN, injected comparison failures/exception/reentrancy, map initialization selecting both comparators, and malformed guest memory.

A new **compiled cross-phase Phase638–648** regression chains original Phase646 outer table construction, Phase647 nested map construction, Phase648 comparator invocation, and the existing 39 synthetic symbol tree insertions and rebalancing. It asserts that Phase647 selects the original **0x2228E9B0** and Phase648 translates guest block/offset references into expected string pointers for the required typed delegated callback. The original `0x2228F7A0` lookup used by Phase638 still remains a synthetic callback: the test does **not** claim real original nested lookup has been recovered.

Portable Linux C++ CTest, native Windows MSVC Win32 x86 tests, and Android NDK build compatibility are in this PR and verified separately through its exact-head CI. A passing `dlopen` smoke is a test of Android module compatibility, not a playable Android/Quest 3 game.

### Private owner audit command

`python research/tools/audit_original_nested_comparators_phase648.py --server /path/to/original/server.dll --client /path/to/original/client.dll`

Public original-free mutation test:

`python research/tools/audit_original_nested_comparators_phase648.py --self-test`

## Next original-specific gaps

1. **`0x223A5574`** original delegated string comparison with runtime state `0x2259F500`, auxiliary `0x223A8410`, and char-transform helper `0x223A72DA`.
2. **`0x2228F7A0`** real nested lookup/registration path, which currently still uses synthetic forwarding callbacks.
3. Actual in-process original retail game verification and original engine memory manager/Windows→Android integration, only after gameplay source dependencies are authenticated.

No claim that original retail Windows gameplay, actual NPC AI or Quest 3 VR gameplay is yet working.
