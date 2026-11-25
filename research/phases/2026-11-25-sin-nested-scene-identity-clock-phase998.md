# Phase 998: retail nested scene identity and timing boundary

Reopened original SE1/bin.zip from owner Drive and extracted server.dll (SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650). Local objdump PE32 disassembly confirms:
- Both scene vtables 2242dd3c and 2242e89c slot +33c = 221a2c20, which reads scene+40c and calls 22114060. The latter scans root+14 actor words for pointer equality up to root+20 count and returns -1 on absence. There is no child traversal or name comparison at this boundary.
- Both +2c4 = 221a2b60: type 11 only, obtains child through 2210b520, checks 221120e0 finished, then simulates child using scene+3e0 directly, not parent time minus child start.
- Scripted +290 = 221a9aa0 writes incoming delta to +3e0; instanced +290 = 221a44e0 increments +3dc using incoming delta without writing +3e0. Do not impose a guessed nested clock correction.
- Child start 221adc70 resets the linked graph; original cache lookup on a separately parsed same-named child actor still misses.

Owner-only archive check: scenes.zip SHA256 b3bc833210dabb7fb30ffde46be1ca7136f40e09ae97b42713f48b5b22e5739e; all 261 VCD members scanned using line-level event declaration search, zero subscene declarations. This does not rule out dynamically supplied scenes or other asset archives.

The named manager fixture now creates a same-named actor in the child graph, verifies distinct PE32 actor object identities and native root lookup miss, observes the missing-actor callback exactly once and no extra emission, and checks that instanced +3e0 and the nested simulation time remain zero. These are original-derived regression expectations, not retail runtime execution. CI pending; no original assets or disassembly bytes committed.
