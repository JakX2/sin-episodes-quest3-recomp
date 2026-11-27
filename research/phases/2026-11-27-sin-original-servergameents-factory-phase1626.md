# Phase 1626: Original `ServerGameEnts001` factory and seven-entry vtable evidence

## Scope and proof

Direct bounded read-only PE32 analysis of the owner's original `server.dll`, SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650` (original image base `0x22000000`). This is original code and ABI **identification**, not a functional engine-side implementation.

The retail image contains the exact C string `ServerGameEnts001` at original VA `0x22443314`. Its sole direct pointer reference in the image is in static-registration thunk `0x223bf210`: it pushes this name and factory function pointer `0x221e82c0`, selects static registration object `0x225723e4`, then calls shared factory-registration helper `0x221fb5d0`. The registration method is 21 bytes (`6c46b07d21b9fa3c4a216b491ec0719e94c4b1dd6d2a72bbf9b13bb9df79209a`). The factory function at `0x221e82c0` is six bytes (`81f3c5629ce13db9b52fddf5328c9d8301d28f7d02a510d2108ede5d9d3ea532`) and returns a fixed original singleton address, `0x22510654`, without allocating anything or handling a requested version itself. The singleton's first word is vtable pointer `0x22443030`.

The vtable is located in `.rdata`, immediately after an RTTI/completion pointer at `0x2244302c`. Seven function pointers occupy bytes `0x22443030` through `0x2244304b`; following `0x2244304c` is zero, then a different RTTI record and `ServerGameClients003` vtable at `0x22443054`. SHA-256 of exactly those 28 bytes is `802c3d045ace1464abe8259852c6654c2c4a2327b56be820085cca8e609038ae`. This rules out accidentally conflating the two adjacent interfaces.

## Complete virtual entry inventory

| Slot | Original function | Bytes | Full method SHA-256 | Verified behavior at this proof level |
| --- | --- | ---: | --- | --- |
| `+0x00` | `0x221e9360` | 31 | `6d003e7b41562fcfcf5f3741ad6888bd72f881fac026b5ea76dd8f976d115e32` | Deleting destructor: resets vptr to `0x2244281c`, optionally invokes deletion helper `0x2226e800`, returns this |
| `+0x04` | `0x221e82d0` | 12 | `dca33cf1628220c76b1919029a5e882dfe903aa93934aee599ef63f66e59c78f` | Stores its sole argument at global `0x225721e8` and returns with `ret 4` |
| `+0x08` | `0x221e9380` | 299 | `8352791af429bd1cf0d040a0765b42ec959181b34d67f98f818a8c039389f1c1` | Resolves two arguments through `argument+0xc` and their virtual `+0x14` methods, rejects absent entities, refreshes dirty origins, averages two 3D positions using source constant `0.5f` at `0x223d9778`, then calls owner helper `0x22221080`; complete helper semantics unresolved |
| `+0x0c` | `0x221e82e0` | 14 | `0b0caf62afd15971ebdd8778c492e03519e45a2c93c03b42aceec070630cb0ac` | Passes the sole argument into delegated `0x22266e60`; ignores returned value; `ret 4` |
| `+0x10` | `0x221e94b0` | 22 | `c702a6d4ebf9c7ec08865a2dfa7b1010364af129604919721528675c3c3fac53` | Null argument returns zero; non-null returns 32-bit field at `argument+0x16c`; `ret 4` |
| `+0x14` | `0x221e82f0` | 28 | `189b466407658510bb27d05b63609600b2e338a8c920d5b5997cb2002869bbe3` | Null argument, or null nested `argument+0xc`, returns zero; otherwise invokes nested object's virtual `+0x14` and returns it; `ret 4` |
| `+0x18` | `0x221e94d0` | 354 | `e069400e844edfe9c5e756bfd828abfc6764df7b37ce13dfca374f94c25ad176` | Iterates a supplied collection of 16-bit indices, checks per-entity state and engine-global conditions, invokes further class virtuals and inherited helper methods; complete method logic and ownership remain unresolved; `ret 12` |

Full-method hashes authenticate byte ranges and original function boundaries, not completed readable C++ for the larger entry bodies. At least `+0x08` and `+0x18` still require deeper original call-tree recovery before implementing an engine-facing interface.

## Relationship to the actual retail startup experiment

Original factory identity is now proved. The owner previously observed retail-engine startup request `ServerGameEnts001`, rejected by the independent diagnostic factory, with the engine then continuing into client `VClient011::Init`. The request's rejection was *not proven fatal*; likewise, this original vtable inventory does not make the independent `server.dll` safe to advertise `ServerGameEnts001`. The original embedded object, vtable, deletion semantics, entity pointer conversion and owner helpers cannot be replaced by a dummy pointer or a partial method table. Keep `CreateInterface` fail-closed until all seven ABI slots and relevant storage contracts are safely implemented and tested.

The next useful work is to reconstruct the two large originals at `0x221e9380` and `0x221e94d0` and the delegated helper `0x22221080`, while preserving existing Source engine services and checking all registration/Init call orders. No shared physics, surfaces, props, model handling or rendering is reimplemented here.

No proprietary executable bytes or complete commercial disassembly are included. No new in-engine runtime test was performed, and neither Windows retail startup nor Android/Quest gameplay is claimed.

## Reproduction and audit status

The seven complete virtual method bodies cover **760 authenticated original instruction bytes**. Together with the six-byte factory thunk and 21-byte static registration, the phase inventories **787 distinct original code bytes**, plus 28 vtable data bytes. This counts authenticated full bodies, *not* full semantic decompilation.

Run `python research/tools/audit_sin_original_servergameents_factory.py --self-test` for the original-free synthetic PE32 parser, vtable pointer, method digest and tampering regressions. With the owner-local authenticated original, use `python research/tools/audit_sin_original_servergameents_factory.py --server /path/to/server.dll --self-test` for full SHA-256, exact registration order, thunk, singleton, seven virtual entries, whole-method hashes and targeted original-image corruption tests. The original-free self-test is wired into `sin-original-decomp-tests.yml`; no commercial binaries are included in GitHub CI.
