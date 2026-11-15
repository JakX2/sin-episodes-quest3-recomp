# Phase 243: original client PVS map comparator, global dispatch and typed key adapter

15 November 2026. Stacked draft on Phase242 PR #297. This work is grounded in the owner-supplied original **SiN Episodes: Emergence (2006)** retail PE32 Windows DLLs and the previously reviewed original client entity-list constructor (Phase228). **No original retail binaries, complete proprietary native function bodies, disassembly dumps or game assets are committed.**

## Dual-original-DLL provenance, required before reviewed-code credit

| Original module | Whole file bytes | Whole original SHA256 |
| --- | ---: | --- |
| client.dll | 3,592,192 | 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d |
| server.dll | 6,049,792 | 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650 |

## Four newly authenticated complete original native x86 methods

| Original client VA | Purpose | Whole native x86 code bytes | Complete original code SHA256 | Original exit, INT3 |
| --- | --- | ---: | --- | --- |
| **0x241056C0** | Compare first 32-bit key field with unsigned less-than | **17** | **77b87ba9017ac5403ea41c488944263dd6475f82ddf3b8f28b36155b9e8f15f4** | RET at +0x10, 15 INT3 |
| **0x24105CF0** | Tail-JMP through global comparator target address | **6** | **1c204ed9a7470ce6d00c305cb9c6fe5aede30d0cf03616fc31363d357cb974da** | JMP DWORD PTR [0x2433FDB8], 10 INT3 |
| **0x24105760** | Nonzero setter / zero getter of same global pointer | **20** | **fe8da08323e50b2b4b05c2b6d87f4798721dce4a1b6d207a000b208a4d792b60** | RET at +0x0D/+0x13, 12 INT3 |
| **0x24108800** | Compose key DWORD+WORD, optionally publish comparator and insert into map | **54** | **90ed1a6166e503620e701e59198499e531b64dec6be6f665edb9fffa6c9bf9d3** | RET8 at +0x33, 10 INT3 |

**Reviewed original-function ledger 349 → 353: server 147, client 202 → 206. Four new complete native x86 bodies, 97 bytes.** This count reflects independent entire-method SHA reviews, **not a decompilation percentage**, source/compiler exactness or an independently bootable Windows gameplay DLL.

The auditor gates original-method credit on both entire retail DLL hashes, all four complete native method hashes, **26 sparse native opcode witnesses**, native return and following INT3 boundaries, five external owner-construction/global-store witnesses, the exact E8 at 0x2410882B, and **eight real PE32 HIGHLOW relocation records** for the absolute function/global pointers. Owner original binary hashes and those byte/relocation checks were independently verified locally.

## New direct runtime ABI finding

Original constructor **0x24108190** at native code sites:

- **0x2410822F** installs native thunk **0x24105CF0** into the entity-list parent's embedded map at parent **+0x18060** (map first DWORD).
- **0x24108273** installs original comparator **0x241056C0** into the same embedded map at parent **+0x1807C** (map +0x1C). This is the global-candidate field.
- The thunk **0x24105CF0** executes an **indirect tail-JMP** through process-global function-pointer DWORD **0x2433FDB8**, whose target must be established before comparisons.
- The function at **0x24105760** stores a nonzero caller pointer in global 0x2433FDB8 and returns it; with zero it **reads** the existing global without modifying it.
- At **0x2410881F**, the original map insertion adapter copies its nonzero map+0x1C candidate into global 0x2433FDB8 before calling map insertion **0x24108750** from native E8 site **0x2410882B**.
- The previously SHA-reviewed original PVS registration and unregister functions similarly copy that candidate at native sites **0x241088EA** and **0x2410894E**. They are not counted a second time.

### Exact original comparison

Native comparator at **0x241056C0** reads one DWORD from each key pointer. It performs an x86 unsigned **CMP**, **SBB**, **NEG** sequence and returns **0** or **1** for \`first_key_DWORD < second_key_DWORD\`. The other **four bytes of each stored eight-byte key do not participate**. As a result, two full keys with different upper bytes can compare equal, even if their stored payloads differ.

This matters for the Phase236 map lookup and Phase237 insertion placement. It supersedes any generic eight-byte comparator assumption for this specific original map owner. The portable C++17 implementation uses bitwise byte assembly of the first DWORD and an unsigned comparison; it is not invoking the retail DLL.

### Native stack-key initialization nuance

The original **0x24108800** adapter reserves **8 stack bytes** as a key, but writes **only the first four bytes** from its first caller-provided pointer and **two bytes** from its second pointer. The last **two bytes are NOT initialized** by this method. It then calls original map insertion 0x24108750 with pointer-to-key and a zero second argument, returning via RET8.

The host reference takes the high two bytes **as explicit injected values**, instead of inventing zero initialization or pretending they are stable. This also avoids falsely claiming that the stored eight-byte payload has six-byte canonical identity. Only the initial four-byte key field affects the verified comparator. The original native function does not add stack validation or initialize the unspecified bytes.

### Original HIGHLOW relocation sites authenticated

Eight PE32 HIGHLOW relocations on absolute target/global addresses:

| Native relocated operand VA | Meaning |
| --- | --- |
| 0x24108235 | Constructor map+0 thunk 0x24105CF0 |
| 0x24108279 | Constructor map+0x1C comparator 0x241056C0 |
| 0x24105CF2 | Thunk's global pointer 0x2433FDB8 |
| 0x24105769 | Global setter operand |
| 0x2410576F | Global getter operand |
| 0x24108820 | Typed insertion adapter's global store |
| 0x241088EB | Existing PVS registration global store |
| 0x2410894F | Existing PVS unregistration global store |

## New code and tests

- \`research/tools/audit_sin_original_client_pvs_comparator_dispatch.py\`: original retail dual-DLL SHA gates, four complete method SHA256s, 26 sparse native witnesses, return/padding checks, five owner/global dispatch code witnesses, exact original map insertion E8 and eight PE32 relocation sites.
- \`research/tests/test_sin_original_client_pvs_comparator_dispatch.py\`: synthetic-only PE mutation tests, original method/return/INT3/external witness/PE base/text-section guards; **synthetic evidence receives zero original recovery credit**.
- \`include/sin_compat/original_client_pvs_comparator_dispatch.hpp\`: clean-room C++17 unsigned first-DWORD comparator, injected native global pointer token, typed thunk adapter and caller-supplied unknown high key bytes; reuses Phase236 insertion API.
- \`tests/test_original_client_pvs_comparator_dispatch.cpp\`: unsigned ordering including values >= 0x80000000, first-DWORD equality despite unequal high bytes, original pointer get/set behavior, fail-closed unknown target, and **Phase236–240 map insert/lookup/red-black integration** using the real original comparator's comparison rule.
- Root Linux and native MSVC Win32 x86 CTest registrations, plus revised SHA-reviewed method catalog.

Owner-private retail original binary audit:

    python3 research/tools/audit_sin_original_client_pvs_comparator_dispatch.py --archive /private/bin.zip

Synthetic-only mutation suite:

    python3 -m unittest discover -s research/tests -p test_sin_original_client_pvs_comparator_dispatch.py -v

## Remaining original Windows runtime gaps

This recovers the concrete **comparator ABI and dispatch path** for the PVS map, but not the original engine-owned containing object, its full C++ vtable implementations, actual runtime allocator, or an independent rebuilt client/server DLL proven loaded in the retail Source engine. The global indirection could point to a different native function; the host explicitly refuses unknown targets instead of making up their behavior.

Priority remains **original Windows PC independent gameplay DLL boot first → Android ARM64 second → Quest 3 VR last**. CI host references, proxy forwarding and ABI smoke do not prove gameplay replacement.
