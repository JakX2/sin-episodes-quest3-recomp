# Phase 1429: Native map output action construction

Both original PE32 DLLs extracted from the user Drive SE1/bin.zip and SHA256 verified: server 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650, client 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.

Original datamap custom slot +0x10 resolves via global 224f363c -> object 224f38b0 -> vtable 2240ed10 -> 22154bd0. That routine allocates 0x1c from pool 22559a00, constructs via 22154830, then prepends to output+0x14. 22154990 performs the same operation on a direct output pointer. Constructor increments the shared action counter at 225599f8, clears target/input/parameter/delay/next, initializes count -1, then reads five comma-separated fields via 22285ea0. Empty input defaults to literal Use at 2240a758, empty target and parameter remain zero, atof delay, atoi count (zero becomes -1). No trimming, escaping or sixth field interpretation. Row offsets 0/4/8/c/10/14/18 match the existing native void-output traversal (Phase 1334).

Authenticated byte ranges (end exclusive):
- 22154830..22154987: a5a20abed82ad3e76974249dac535013b8600bd56d075cd6800051b22d81f28c
- 22154990..221549cb: 6ba7d4032e2acbb8a317475fa5967b524c5a3b3079d8413be06fef0af5de6dd1
- 22154bd0..22154c13: 6d4dcbc8dcb86045d2e69d98b3823bdbd196c898ad3739a2a7ed248ee5e910e7

Opt-in native output leaf uses explicit shared ID storage and pool allocate/release, reuses KeyBindings intern. Tests loaded replacement DLL direct and actual map parser, duplicate output order, omitted fields, zero count, finite count, failure and bounds. Owner must provide stable disjoint 32-bit pool rows and the same token/pool domain to existing output firing. Bounds reject >255-byte field, count overflow, short output extent, invalid allocation; failed interning releases unlinked row and consumes ID. No attempt to emulate original unsafe buffer overrun or null allocation crash. Prevalidates fields before allocation, unlike retail; no real engine heap publication, no claim of actual map-triggered delivery regression yet. Legacy output callback retained when native binding absent. CI pending.
