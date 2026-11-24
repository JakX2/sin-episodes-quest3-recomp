# Phase 691: original shared client/server ammo definition registry

Fresh owner-private exact client/server DLL SHA256 checks; original
server registration 0x222d54b0..0x222d57fb, 844 bytes, SHA256
e622b24aa7423468578a37bd03a17d5795cbabfb07346b65645a42d20b6e4e31.
Original client counterpart 0x24212ec0..0x2421320b, 844 bytes,
SHA256 feb0e28932f1ab27eb44c56f2bca165eb4ff7324c86262dcad5f17acd5ca9056.
Direct local decoding of both push/call sequences resolves all 17
ordered registrations with 12 arguments each, identical after string
addresses are resolved. Calls reach server 0x221e61d0 and client
0x24089ab0. Server constructor 0x221e6270 starts index at 1 and
allocates 32 slots; name lookup 0x221e5d50 begins at index 1.
The older owner SDK2004 CAmmoDef uses case-insensitive name lookup;
original wrapper and field stores establish the extended record.
Player damage at table base + index*60 +0x2c, NPC float at +0x30,
max carry at +0x34; original max helper 0x221e6370 verified in
Phase690. Preserve impulse and extra float as bit fields; no claim
of an exact source-level name for the extra float. All original
registrations are finite literal maxima.

New independently authored identical source compiled into actual
client.dll and server.dll, versioned read-only metadata API; index,
definition, max and count. Native server's shared ammo operations
use the original max table when host provides no max override.
Caller-owned live inventory and original engine notification
remain host responsibilities. No engine CAmmoDef object installed.
Actual Win32 pair test loads both modules and compares all 17 rows,
exact order/max, case-insensitive lookup and invalid-index behavior.
Linux service test uses original default max. Owner-only audit
reproduces cross-DLL verification without emitting code or assets.
Not full networking/prediction or retail startup.
