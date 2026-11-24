# Phase 690: native shared combat-character ammo behavior

Fresh owner server.dll SHA256
568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650.
Original RemoveAmmo 0x222aa670 (88 bytes,
723466e888783547ecd920bc5a6bc869ea895e790dd35dcbb81412a0eb7f9e7c);
RemoveAllAmmo 0x222aa700 (51 bytes,
4fa3c21ec590db393138edc4b808ae72866db79a80c74263d755c9187c0b4187);
SetAmmoCount 0x222aa740 (43 bytes,
2c25753d5d4c090b1ea36814f44a455fb124b525bd7a2a321089eab0643070c1);
GetAmmoCount 0x222aa770 (55 bytes,
053f22e7bfcc85269deed6759f4078c330dac5731e46a8e2e04ff9bb04446c9c).
AmmoDef max helper 0x221e6370 (85 bytes,
4e03937cf9d557ea4b751f7816b10bcf4fe7f1e15199bd30b4e7da6ac16d4b37)
returns literal max or converts cached ConVar float toward zero when
the raw max is -1 and a variable exists; invalid definition returns zero.

Owner array begins +0x64c; clear loops exactly 0x80 bytes (32 dwords).
Get index -1 returns 0 before definition lookup. Max == -2 returns 999
without reading storage. Remove nonpositive amount is no-op before
definition lookup, infinite definition does not debit, otherwise native
32-bit subtraction then signed clamp to zero. Set and clear do not check
definition. Changed value calls owner virtual +0x3e4 before store;
equal skips. Clear notifies separately for each changed slot.

New host-based native CombatAmmoApi executes these original operations
without duplicating owner inventory. Live generation and max resolution
are host responsibilities. Port bounds reject unsupported indices
instead of reproducing unchecked original array access. No raw memory
writes, original ABI allocation or guessed ammo definitions.

Restricted synchronous integration: Magnum reserve query and debit,
scattergun owner ammo query and reserve debit, and rifle authorization
may call the same native ammo API for only the matching owner and
permitted operation. Other nested operations/owners and recursive
ammo are refused. Rifle atomic commit remains host-owned (not replaced
by an independent debit). OutputScope preserves outer guard.
Reports delivered writes and uncertainty; partial clear is not replayed
automatically. Tests cover exact callback order, infinite/-1/negative/
wrap semantics, full clear and partial failure, and actual cross-export
Magnum reserve query/debit through the loaded module. No real retail
owner entity or network callback has been installed.
