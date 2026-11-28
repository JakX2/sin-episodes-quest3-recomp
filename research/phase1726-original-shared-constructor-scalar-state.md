# Phase 1726: first executable scalar projection of the high-use constructor

The original owner client.dll was extracted from the Drive SE1/bin.zip and
SHA-256 verified as 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.
Disassembly with GNU objdump confirms the complete 206-byte body at
0x24092BD0 (SHA-256 28e8f086a532a606b78304522eaaeb571f63922feaee80aaaeeacb7e8a03659a,
followed by two INT3 padding bytes). It calls 0x240BD7C0, which
in turn calls 0x240CCE30 (also used by CWorld). The latter base
constructor and the intermediate constructor are NOT implemented here.

After the base call and installation of four original vtable labels,
the high-use constructor writes scalar state: six 32-bit all-ones
fields, four zero 32-bit fields, four floating-point bit patterns
(65.0 twice, 1024.0 twice), one zero byte, and a 16-bit all-ones value.
The last value comes from 0x241E6ED0, a 5-byte helper whose AX result
is always 0xffff (SHA-256 8d7d77a27d5c4e65bce023f11124befd05322e6a0e3cfa9f0fb439f8baa2d9ea).
The highest written byte is +0x81f. Conditional stores are represented
only by their final scalar byte state.

The new C++17 function executes these scalar writes on caller-owned
synthetic storage, after checking the exact source creator ordinal,
source VA, constructor family, allocation size and minimum extent.
It deliberately does not install vtables, run base constructors or
expose a native entity. Tests cover all 34 directly mapped canonical
users (33 inline + one 60-byte creator), rejected classes, wrong VAs,
undersized and oversized buffers, unchanged unrelated bytes and
idempotence. Existing canonical CI target executes the regression.

The 68 distinct 60B constructor targets and six distinct 86B targets
overlap at 0x24092BD0, 0x240BD7C0 and 0x240C3940. Their union has
71, not 74, unique targets. This correction does not change the
118 creator body count.

Original PE32 code bytes and game data are not included. Native
ClientClass +0/+4 callbacks and VClient011.slot5 remain guarded.
No claim of a completed constructor or retail runtime integration.
