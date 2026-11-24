# Phase 727: shared SiN container state and leak termination

Fresh SHA256 matches owner pins: server 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650, client 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.
CPropContainer constructor 222ffb80 and default constructor 222ffaa0,
subclass constructors 222fffc0 (antigen), 22300410 (large oxygen),
223004e0 (small oxygen), 223005b0 (acetylene), 22300690 (barrel).
All verified factory sizes 0x6a0. Model/sound strings resolved from
original rdata, including original misspelling acetyline_tank.mdl in
acetylene subclass vs acetylene_tank.mdl generic. Values passed into
base constructor preserved as raw fields, no inferred physical units.

| Class | +62c flags | +634 | +638 | +63c | +648 raw |
|---|---:|---:|---:|---:|---:|
| Generic | 3 | 10 | 1 | 0 | 100 |
| Antigen | 9 | 0 | 1 | 0 | 0 |
| Oxygen large | 92 hex | 10 | 1 | 1000 | 100 |
| Oxygen small | 92 hex | 20 | 1 | 3000 | 100 |
| Acetylene | a2 hex | RandomFloat(3,4) | 0 | 3000 | 100 |
| Barrel | e0 hex | RandomFloat(1,2) | 0 | 0 | 100 |

Original common +628=100.0, +64c=0, +668 and +69c invalid;
+630 model pointer, +640 loop, +644 null. Native defaults do not
construct base/output subobjects. AI radius helper 222ff670 returns
256 if flags&60, otherwise 125 if flags&0c, otherwise zero. Bit 100
branches in original are redundant with bit 4 in the final predicate.

Leak termination 222ffc30: no-op unless bit 100 set. Clears 102,
stops optional configured loop via 221b6a80. Checks serial/live
+668 steam handle, resolves again, removes and invalidates only if
initial live check succeeds. Decrements signed DWORD +638; if <=0,
zeros +63c and +628. If remaining flags bit20, tail-jumps virtual
slot188. Native reproduces mutation and ordering, requires host
subclass dispatch (without reentry into same control). Host
retains actual effects, game entity lookup and physics. Explicit
failure block reconciled through existing CanisterApi acknowledge.
Loaded tests cover all defaults, random inputs, radius, inactive
and active termination, stale handles, count and optional break,
failure ordering. Leak start, force and base break are not yet
reconstructed here. No assets/binary bytes or HL2 physics added.
