# Stats owner catalog setup in original 2006 client

Original client SHA256:
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

`C_StatsSystem::Init` calls `0x241FC5A0`, which has a **complete
292-byte SHA256**:
`18bc538a9afd444683914b08aa6bf7d73b2d8dd403e15f6b07be0b32bb2ea6cd`.

The first part of this 2006 helper takes the subobject at
`this+0x84`, frees previously registered entries, reserves
`0x42=66` pointer slots, then loops 66 times allocating a
separate `0x0C=12`-byte record. Each successful allocation
writes DWORDs `[+0]=0`, `[+4]=0`, `[+8]=-1`. Failed individual
allocations leave a null slot in the pointer vector, rather
than reducing its logical length.

The original function then copies a nine-DWORD table at
`0x242AE0DC`, initializes resource enumeration via
`0x24164C60` and `0x24165010`, and iterates results through
`0x24164BC0`/`0x24164BE0`, forwarding each item to
`0x241FBD20` before releasing the enumeration handle.
The true resource provider and item formats are not yet known, so
our new `BeginOwnerSetup` leaves that stage as an explicitly
injected fail-closed callback, **not an invented Source filesystem
API**.

`client_stats_catalog_init.h` reconstructs the entire owned
66-record setup stage with repeat/reset and nullable-allocation
semantics, nonmovable ownership and a bounded resource continuation.
The MSVC x86 synthetic fixture tests record sizes/values, allocation
refusal, cleanup/reinitialization and outer AutoGameSystem failure.
This does not prove a normal retail client Init or a finished Stats
subsystem. Last owner-real checkpoint remains `0x53494E63`.
