# Phase 206: paired original SiN zero-fill allocation and opaque tier0 vtable calls

13 November 2026. **Draft stacked on Phase205 PR #259.** The owner's original 2006 32-bit Windows `server.dll` and `client.dll` were analyzed privately, each matching the established whole-file SHA256. No commercial original DLLs, assets, full machine-code functions or original game maps are committed. Windows PC game-engine execution remains the primary goal, ahead of Android and Quest 3.

## Six NEW SHA-pinned original native entrypoints

| Owner-original function | Original VA | Entire code bytes | Entire original code SHA256 |
| --- | --- | ---: | --- |
| server zero-initializing allocation | `0x2226E460` | 107 | `c9a369e37d6bf6a269cf66f1dcab52f38a95b8c73384671d44ad500251da0861` |
| client zero-initializing allocation | `0x2416F9C0` | 107 | `12b19a35967f33db88219a0ca0abf348b2ecf8091651a418107f0f19ae6eb512` |
| server tier0 virtual `+0x58` tail jump | `0x2226E5A0` | 12 | `be248372e1ae80572616db55e092bcfcbcbbed484a1e139b15752a334b7d8d57` |
| client tier0 virtual `+0x58` tail jump | `0x2416FB00` | 12 | `ea5a906b1f443fc6c98e936b39d629d3b492e994551a8281a11031b5619ff856` |
| server tier0 virtual `+0x20` forwarding call | `0x2226E5C0` | 18 | `895cd1c5f870dd7ff343225897583863df86dcbe96010ecf12a3c3760a52e041` |
| client tier0 virtual `+0x20` forwarding call | `0x2416FB20` | 18 | `3407dec8c90462f67a8084470e5d362fc741ed9ae0bf24befeb54b269ee70dc1` |

**Six distinct newly reviewed original methods totaling 274 bytes** (three in each module). Other allocator methods in Phases186/202/205 remain counted only once. Full-file SHA256s used for every definitive original function claim:

- server original: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- client original: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

Each same-role pair (server/client) is **byte-identical across its entire native body after zeroing only exact module-specific absolute DWORD operands**. The two zero-allocators each contain six such addresses, and the +0x58/+0x20 wrappers one each. Complete normalized function SHA256s:

- 107-byte zero allocation: `4500ad93361a49d8c8bd267b336464671dda03c58e2e2f9b15c3c7f7b358a99a`
- 12-byte +0x58 tail jump: `02df19bf8bf17c957a3d7e95234ca58058edd92624a02815b1d476e5550f7e4c`
- 18-byte +0x20 wrapper: `0dc8d5feacd828692663970ee9ecfab1f1fc2f5044bb2c5321c85566cbd92111`

## Original x86 semantics, without guessing the opaque interface

### 107-byte zero-fill allocation

The original function receives **two 32-bit stack operands** (element count and element size). It multiplies them into ESI with original x86 `IMUL`, retaining the low 32 bits. It then lazily reads/initializes the **same primary cached debug-owner context** as the original single-size allocator, via `CALL rel32` to native server `0x2226E290` / client `0x2416F7F0`. The exact original internal CALL site is `+0x1E` in both functions.

After obtaining an allocation from imported `tier0.dll!g_pMemAlloc` (vtable byte offset **+0x04** plain or **+0x00** owner-tagged), it runs `REP STOSD` for `byte_size >> 2` 32-bit words, then `REP STOSB` for `byte_size & 3` remaining bytes, with EDI = returned allocation pointer. It returns that pointer in EAX. Native code **does not check** whether the multiplication overflowed or whether the allocator returned null. The host model expressly fails closed on overflow and nonzero-size null allocations rather than writing invalid memory. Both original routines end with plain `ret`, followed by five `int3` alignment bytes.

A previous module's zero owner/initialized bit is **not shared with the other DLL**. As Phase205 verified, the stack-based zero allocator and stack/register basic allocation within one DLL share its primary cache fields.

### Original +0x58 tail jump

The exact 12-byte routine loads the original module-specific imported `tier0.dll!g_pMemAlloc` pointer, dereferences the allocator interface vtable, and tail-**JMP**s to byte offset `+0x58`. There is **no call or local RET**, and the function does not read/push an explicit argument. This is an externally implemented *opaque method*, not proof of its formal C++ class/member name, return type or side effects. Four original INT3 alignment bytes follow the tail jump.

### Original +0x20 forwarding call

The 18-byte routine loads the same imported `g_pMemAlloc` object, reads its first stack DWORD, pushes that value and calls the allocator vtable's byte offset `+0x20` before returning with `ret`. Do **not** infer a particular tier0 API name or lifetime action from the raw vtable offset alone. It is a separate function from the later adjacent duplicate same-body wrapper at server `0x2226E5E0` / client `0x2416FB40`, which is **not** newly cataloged in this phase. Fourteen original INT3 padding bytes follow the chosen function.

## Implementation and regression

- `research/tools/audit_sin_original_tier0_zero_allocation.py`: original PE32 section checks, full dual-DLL and all six whole-function SHA256 hashes; full-body normalized twin digests; **16 exact absolute IAT/module-static address operands**, **26 sparse opcode witnesses**, **two exact original context-helper CALL rel32s**, all six INT3 method boundaries, and verified original `tier0.dll!g_pMemAlloc` named import and module-specific IAT addresses.
- `research/tests/test_sin_original_tier0_zero_allocation.py`: ten original-free synthetic mutation groups, including every absolute operand and opcode witness, native callsites, original boundaries, imported tier0 name/address and fake full-code/whole-file hash rejection. Fixtures never count as original.
- `include/sin_compat/original_tier0_zero_allocation.hpp`: C++17 portable model that composes Phase205's exact **per-DLL 16-byte lazy context state** with zero-initialization and separately named **opaque** +0x58/+0x20 host callbacks. Uses synthetic 32-bit pointer tokens, host-owned zeroing and overflow/invalid-memory checks, none of which are native ABI equivalents.
- `tests/test_original_tier0_zero_allocation.cpp`: tests non-DWORD-aligned 21-byte and aligned 16-byte zeroing, zero-sized allocation, no repeated lazy initialization after zero context, valid tagged allocation, overflow/null pointer refusal, separately held server/client state, and opaque dispatch argument and return propagation.
- Root CTest and genuine 32-bit Windows MSVC CTest added. **Hosted CI on the exact draft branch must pass before claiming validation.**

## What this does NOT establish

No complete original `tier0.dll` allocator implementation is available in this phase. The meanings of virtual methods `+0x58` and `+0x20`, and ownership/lifetime across native modules, remain open research questions. These portable C++ tests are not the original 32-bit Windows game launching, rendering, accepting gameplay commands or entering the real SiN campaign. A full original-engine loader/factory and DLLInit trace using the owner's retail installation is still the highest-value Windows PC integration milestone.

**Verified reviewed-function atlas: 207 → 213 (116 server / 97 client)**. This is independently verified function inventory, not decompilation-completion percentage.
