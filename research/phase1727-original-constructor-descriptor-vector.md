# Phase 1727: shared constructor descriptor insertion and growth

Source: owner client.dll SHA-256
6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.

Original complete method identities, independently read from the owner
binary using GNU objdump and SHA-256:

| Original source VA | Bytes | SHA-256 |
| --- | ---: | --- |
| 0x24061220 | 121 | 3e3868e86685fa86f4bcbb796a35de8b8277cddb8b69e17f203a673757edb948 |
| 0x24060870 | 115 | 2dc1e61743a119f0043dfc270e07363ff9d3d7946a27e68855145a1fca9407a9 |
| 0x240BD7C0 | 861 | 194b3ef35fa66ab726f06f1ce37539c6901ced7a3d22995a228f00aa04e62db0 |
| 0x240CCE30 | 573 | 63fc9f1bbd6c8fe318f7faa1cd4aef448dd19553a6dbdc0d182b768855b34eca |

The base constructor 0x240CCE30 invokes insertion at 0x240CCFBE
and 0x240CCFE4 for its embedded vector at +0x10. The intermediate
constructor 0x240BD7C0 calls it at 0x240BDA0E, 0x240BDA3A and
0x240BDA60 for its embedded vector at +0x480. This is a shared
constructor dependency of the 34 direct routes from Phase1726, plus
other direct and transitive base users, including the CWorld route.

The original vector uses storage, capacity, growth setting, count,
and cached storage at +0, +4, +8, +0xC, +0x10 respectively.
Insertion grows as needed, increments count, refreshes the cache,
moves the suffix by one 12-byte record, copies three opaque DWORDs,
and returns the insertion index. Growth starts at three records,
then doubles or adds a positive growth step; -1 prohibits growth.
The old 24-byte descriptor model starts at two and must not be used.

The portable C++17 implementation owns records with std::vector
and models source logical capacity separately from host capacity.
It rejects invalid indices, unsafe size arithmetic and insufficient
external capacity rather than reproducing invalid source accesses.
It snapshots descriptor input by value; native alias-dependent
behavior and original allocator callbacks are not implemented.

Local strict GCC C++17 test and AddressSanitizer/UndefinedBehaviorSanitizer
test passed. CMake and the existing focused CI workflow are wired.
No original code bytes, game files, or native pointers are published.
Base constructors, virtual functions, original allocator, slot5 and
ClientClass callback publication remain incomplete.
