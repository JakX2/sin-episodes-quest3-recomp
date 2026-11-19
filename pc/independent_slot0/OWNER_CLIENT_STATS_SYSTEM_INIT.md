# Original C_StatsSystem::Init: seven ordered native operations

The owner's original 2006 client.dll SHA256
6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
contains the original C_StatsSystem::Init at 0x241FCAA0.
The entire **114-byte** method SHA256 is
8cfdc286dfe3dafc355884aab427fb53eaa04c2db66e444b64fd1816fbc6b6d5.

The original method initializes subordinate memory at this+0x04 and
this+0x70, calls another owner setup helper on this, asks a helper
to process `scripts/stats/stats_display_layout.txt` at this+0x98,
then makes three ordered native callback registry insertions:

- `StaticStat` with original callback 0x241FB7A0
- `DynamicStat` with original callback 0x241FB7E0
- `ResetHUD` with original callback 0x241FCA60

Every operation is checked by original CALL-rel32 instruction addresses
and the literal names in the owner-private SHA-verified DLL. All seven
calls are unconditional, and the original function concludes with
`mov al,1; ret`. The three callback pointers are opaque native
addresses, not executable functions in our independent client.

`client_stats_system_init.h` is a **correctly ordered orchestration
model** using callback-injected implementations for all seven steps.
The native test checks call identity, ordering, repeat behavior,
and the surrounding AutoGameSystem first-failure boundary. The actual
four helper bodies, registry symbol store and callback functions remain
unimplemented. Accordingly this does **not** mark C_StatsSystem's
complete Init method as reconstructed; the last live owner engine
milestone remains 0x53494E61. Do not invoke original pointers as
independent DLL addresses.

Further work: independently reconstruct 0x241FF420, 0x241FF8E0,
0x241FC5A0, 0x24204F30, then the registry callbacks.
