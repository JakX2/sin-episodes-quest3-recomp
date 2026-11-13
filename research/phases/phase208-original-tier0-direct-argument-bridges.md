# Phase 208: original tier0 allocator direct-argument bridges

13 November 2026. **Stacked draft on Phase207 PR #261**. The owner's original 2006 SiN Episodes: Emergence i386 PE32 `server.dll` and `client.dll` were privately inspected, with complete original-file SHA256s verified. No commercial DLL bytes, full native function bodies, extracted game content or raw disassembly are committed.

This phase extends allocator investigation beyond lazy-context allocation and free: native three- and four-argument virtual forwarding and the **exact selection and order of stack arguments**.

## Source provenance and distinct newly reviewed functions

Original `server.dll` (6,049,792 bytes, original PE image base `0x22000000`) SHA256:
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.

Original `client.dll` (3,592,192 bytes, original PE image base `0x24000000`) SHA256:
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

| Complete original function pair | Server VA | Client VA | Original bytes each | New verified functions |
| --- | --- | --- | ---: | ---: |
| Tier0 virtual `+0x00`, three forwarded DWORDs | `0x2226E720` | `0x2416FC80` | 27 | 2 |
| Tier0 virtual `+0x10`, three forwarded DWORDs | `0x2226E740` | `0x2416FCA0` | 28 | 2 |
| Tier0 virtual `+0x08`, four forwarded DWORDs | `0x2226E760` | `0x2416FCC0` | 33 | 2 |
| Shared cached allocation alias A | `0x2226E790` | `0x2416FCF0` | 79 | 1 (server) |
| Tier0 virtual `+0x00`, skips second input DWORD | `0x2226E7E0` | `0x2416FD40` | 27 | 1 (server) |
| Unconditional tier0 virtual `+0x14` free alias | `0x2226E800` | `0x2416FD60` | 18 | 1 (server) |
| Shared cached allocation alias B | `0x2226E820` | `0x2416FD80` | 79 | 1 (server) |

Seven complete client/server method pairs comprise **14 original function bodies, 582 complete original native bytes rechecked**. However, four client functions at `0x2416FCF0`, `0x2416FD40`, `0x2416FD60`, `0x2416FD80` were **already SHA verified and cataloged by Phase185**. They are not counted again. **Newly recovered: 10 unique original entrypoints, 379 original x86 bytes** (7 server, 3 client). This brings the atlas from 221 to **231 reviewed functions (127 server, 104 client)**, not a playable-game percentage.

### Complete original native method SHA256

| Method | Server original body SHA256 | Client original body SHA256 |
| --- | --- | --- |
| Three args virtual `+0` | `59032b061ad657a4b65e535c30bf8597ed1fe4c28ba3cbae8b6dc0aec80dbd9c` | `6cc143aece17495514ffc1e1c9e7f58b04b2391670ea68fe66b475813979cefb` |
| Three args virtual `+0x10` | `7dd353c922692127a061e2b9dcbb9c4a75021744bdee3ad0d5e98cf6f125bb83` | `a870764ec093252a4945e3eb8bc80af4f9cc5472c189e3db7ee1c1002579cc0e` |
| Four args virtual `+0x08` | `028af88c5784da167afc6bcd718877bb57ff3a21e528f35a930ba78501677c8d` | `1e32b0610e9e25e09c3e8d12cc4c57f199e81b27f74f4c6964e4efb8279eedaa` |
| Cached allocation alias A | `1ba57fe89ebae2110c154315e1371c8481cc0bd1cf8363a4c83689219ff9714f` | `84fffc04db883f146d64edf1f474b61b5fe03cb36e95b5fcd717ece17e793acf` |
| Three args virtual `+0`, skipped input | `f903cbb82919679192b60918d3ee9f9087444206f9cfc37eb4ee09ac607e5565` | `06b28fa8c3b5d5701c214e37a09179591199cf5de9364c3f37e0349fbafc7b17` |
| Unconditional `+0x14` free alias | `4d45603c1544ce399c6783bb8fd0019698929a223663d256d112ce57d66868fc` | `c597a60f0eca41a3544aef8a22df99bd70535de676d5bf7f73477f0865daea73` |
| Cached allocation alias B | `f0b61188dfa38e2eea163e724678c5ec46319874062766bbe9b78dd29dcf1884` | `44318a97f116b1b629cb2fe1794e14c28ed58b61a7606b7784a4e7b785c20d08` |

## Precisely observed original native semantics

**Direct imported allocator interface.** All seven pairs use original PE-imported `tier0.dll!g_pMemAlloc` through the server IAT slot `0x223D2124` or client IAT slot `0x242761CC`. The direct wrappers dereference the allocator object, get its original vtable and call these byte offsets:

- `+0x00`, three DWORDs, original first → second → third, corresponding to the known Phase185 debug allocation virtual slot. This is not evidence about the underlying tier0 allocator's actual implementation.
- `+0x10`, three DWORDs, same forwarding order. **Do not claim a method name or actual allocator semantics** for this slot without inspecting the real original tier0 module.
- `+0x08`, four DWORDs, original first → second → third → fourth, no interpretation of data ownership.
- **A second `+0x00` bridge** at server `0x2226E7E0` / client `0x2416FD40` reads stack offsets in a different order. After accounting for each `PUSH` adjusting ESP, the original virtual call receives source arguments **first, third, fourth**, and does **not** forward its original second DWORD. The existing Phase185 client name calls this an explicit 3-argument debug forwarder; the new portable function takes four caller-side input DWORDs and discards the second to match the actual native register/stack reads. This is not proof that a formal C++ source signature actually contained four parameters. Callers must be investigated before claiming their source-level prototype.
- The `+0x14` wrapper unconditionally forwards its pointer (including null) with **no lazy owner context or null guard**. In both modules its entire original 18-byte function body is byte-identical to the already reviewed original Phase202/205 free helper.
- Two further **79-byte allocation aliases** per module use the same primary owner-pointer/flag addresses as Phase202/205/207. They OR bit zero of the flag **before** the resolver call and cache a zero result without retrying. The original allocator memory pointer is shared within a module, but each DLL has its own original global state.

**Exact full-body relocation normalization:** the five direct virtual wrappers have **only one** module-specific absolute DWORD operand (original IAT reference at code offset `+1`), and full server/client method bodies become byte-identical after zeroing exactly that operand. Each cached allocation alias has **six** exact module-specific static/IAT DWORDs (at `+0x07,+0x0F,+0x19,+0x20,+0x29,+0x3C`); pair bodies become byte-identical after these six operands are normalized. Normalizing the **one** original owner-context `CALL rel32` displacement at `+0x14` as well proves these aliases are complete-code-identical to the Phase207 six-entrypoint allocation family (full normalized SHA256 `976227759a29da6c6e7521a12fec6c5796d2575d7c4636dbaa62cf70e406a2a3`).

The new audit pins **14 entire original method SHA256s**, seven whole pair-normalized SHA256s, **74 sparse opcode witnesses, 34 exact original static/IAT DWORD references, four original direct context-helper CALL instructions**, all 14 method `ret`/INT3 boundaries, exact `tier0.dll!g_pMemAlloc` import identity in both original PE import tables, and byte-for-byte duplicate free helper identity. The expected ten-new-function, 379-byte count is an explicit invariant in the verifier.

## Independently authored implementation and tests

- `include/sin_compat/original_tier0_allocator_bridges.hpp`: host-neutral C++17 direct vtable `+0,+0x10,+0x08` and skip-middle forwarding; lazy aliases share the existing Phase205/207 16-byte per-module owner cache; direct free reuses the original model; no fabricated Source engine ABI or external tier0 vtable implementation.
- `tests/test_original_tier0_allocator_bridges.cpp`: both original module configurations, nontrivial unsigned DWORD argument order, all four virtual forwarding shapes including intentionally ignored second caller input, cached-zero/no-retry after feature toggle, unrelated flag bits, true owner-tagged allocation, independent client/server global caches, and unconditional null free.
- `research/tools/audit_sin_original_tier0_allocator_bridges.py`: private-input original whole-DLL and whole-method authentication with exact call/operand/section evidence and cross-phase duplicate prevention.
- `research/tests/test_sin_original_tier0_allocator_bridges.py`: **11 synthetic-only mutation test groups** touching each opcode witness, each original absolute operand, original direct calls, method boundaries, prior free-helper identity, PE sections/IAT identities and attempted fake-original hash claims. Synthetic fixtures never claim original recovery.
- Root Linux CTest and native MSVC Win32 x86 CTest target registered. Hosted CI must be checked on the exact Phase208 head before claiming success.

## Current engineering gate

This is verifiable original-code reverse engineering, but the external original `tier0.dll` method implementations and memory lifecycle are still not reconstructed. The **retail Windows executable loading and initializing real reconstructed SiN game modules** has not been demonstrated. The game still is **not playable on Windows/Android/Quest 3** using these reconstructed DLLs.

The next high-value research target is the original **caller graph** of the newly mapped direct allocator virtual wrappers, and the owner-only Win32 game process startup trace from Phase203. Preserve the Windows-first priority; Android then Quest 3.
