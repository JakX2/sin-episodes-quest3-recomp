# Phase 721: native SiN DNA scanner

Original pinned server.dll SHA matched. CDNAScanner factory 222f83d0
allocates 0x4b8 and installs primary vtable 2246af4c; no distinct
client/server network class. Original item_dna_scanner spawn 222f81c0
uses models/items/dna_scanner.mdl, idle animation, raw +494=0,
capability flags +498=0x20, installs touch callback 222f80f0. Base
capability slot35 222f8440 ORs +498 with base result. Precache 222f8060
also registers SuitRecharge.Deny. Native spawn-state API only owns the
two proven fields; model/base/collision/touch installation is host work.
Datamap getter 222f8050 -> 22528944, lazy init 222f8240 defines five
descriptors including ScannerTouch, m_flSoundTime +49c and m_Scan /
OnScan +4a0. Constructor explicitly initializes output subfields but
does not explicitly write +49c in the inspected constructor body;
native spawn-state does not invent a timer reset.

Touch 222f80f0 compares exact classname literals (including NUL)
prop_ragdoll (2243cffc), player (223d9954). Ragdoll fires OnScan
at +4a0 with scanner itself as both activator and caller, delay zero,
independent of the deny timer. It does not remove ragdoll or gate
by timer. Player invokes deny path, other names ignored. Use slot87
222f8090 checks nonnull activator and player vslot72, not classname,
then identical deny path. FCOM deadline against gpGlobals+0xc,
TEST AH,41 / JP skips greater/unordered, accepts less OR equal.
Stores now+0.62 (f64 constant 2246aef8) as f32 BEFORE emitting
SuitRecharge.Deny. Touch and Use share deadline. No replay on failed
host sound; explicit reconciliation. Exact equality, unordered old
deadline, repeated ragdoll and case-sensitive class gates tested.
Loaded native server fixture exercises both input paths, output
identity, spawn leaf and capability OR. No new rendering, models,
physics, or copied original code.
