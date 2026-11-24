# Phase 693: native paired inventory scalar contract and capture

Fresh owner-original SHA256 verification in Phase692 and direct
constructor disassembly of unchanged original server/client.
DT_LocalPlayerExclusive slot5 m_iAmmo server 0x22249a9d,
32 DWORDs +0x64c, unsigned 10-bit element SendPropInt at
0x22249a82; client 0x240d39bd, +0xc00, 32 DWORDs.
DT_BCCLocalPlayerExclusive slot0 m_flNextAttack server
0x222a631e (+0x60c, NOSCALE float), client 0x240c37a1 (+0xbf8).
Slot1 m_hMyWeapons server 0x222a635f (+0x6cc, 48 handles),
client 0x240c37ea (+0xc80, 48 handles). Original array
constructors 0x2225efc0 and 0x24126e90 create array-derived
tables; this port models only semantic array values, not flattened
wire graph. DT_BaseCombatCharacter slot2 m_hActiveWeapon server
0x222a524f (+0x78c), client 0x240c389b (+0xd40).
Both use Phase692's original 21-bit handle proxy.

Different recipient rules: DT_BasePlayer localdata uses
0x22258250 (call at 0x22249fa9): clears 8 words and selects
object ID minus one. DT_BaseCombatCharacter bcc_localdata
uses 0x222a50f0 (call at 0x222a522f): clears all recipients,
then tests direct object virtual +0x120, gets engine slot18
client number using +0x16c. If nonplayer, tries virtual
+0x130 then returned object's +8(0), tests returned
object's +0x120, resolves that player and sets its client
bit. Do not guess the concrete indirect object type. Always
returns supplied data pointer even with empty recipient set.
Host supplies the two independently observed one-based
recipient slots (0 = none) and full live character identity.

Shared module InventoryApi: exact scalar metadata, separate
player-ammo/combat-local/public masks, raw ammo count array,
handle conversion reused from Phase692, next attack float.
10-bit count validation, invalid handles, reserved sentinel
and nonfinite refusal are explicit port guards, not claimed
original encoder overflow behavior. Capture only reads
authorized groups, not private values of other clients.
Raw inventory must be used, not GetAmmoCount's infinite
999 sentinel, because original SendProp uses the actual
array field. Live handle resolution is external.
Native service regression uses the SAME authoritative ammo
fixture through Phase690 removal and Phase693 capture; decoded
client state observes the changed count. Also tests separate
recipient slots, nonowner read suppression, partial failures
without output publication and stale level rejection.
Win32 pair checks actual client/server module conversions and
array boundaries. Phase692 Release test helper include moved
after NDEBUG undef to retain assertions. No real Source wire
transport, game-class registration or native prediction claim.
