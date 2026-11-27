# Phase 1614: SiN grenade spit mutant/poison Detonate exports

Authenticated retail server.dll SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. The original PE export table reports:
* ?Detonate@CGrenadeSpitMutant@@UAEXXZ at RVA 003120b0, VA223120b0.
* ?Detonate@CGrenadeSpitPoison@@UAEXXZ at RVA00312b70, VA22312b70.
This is a distinct original projectile family from CNPC_RecombinantMinor's CMutantSpit entity.

Both methods first check actor+94, invoke original virtual+1ac when nonzero then clear +94, draw a discarded random float [90,110], emit "GrenadeSpit.Acid" and "GrenadeSpit.Hit" (0,0), in that order. Both call the inherited entity removal service 22286bf0 last. The poison variant additionally refreshes dirty origin for +b8&0x800, then calls the SiN-specific 222fe1b0 effect with source origin+2bc and original constant arguments (0,10,175,10,-1) before removal. Effects and actual Source entity removal stay in the host, no generic physics or model code reproduced. Native API exposes separate mutant and poison methods and logs effects in a regression fixture with state, sound, timing, poisoned origin and malformed input checks. Retail random draw is discarded, so it is only validated, never used for invented damage scaling.

VA/bytes/SHA256:
223120b0/96/21ebeb684d5da46b7ede3bde7231e4a7dd32f908d41d5726f4e51d83d9d882ca
22312b70/150/eda1e9b8c3e27c035068cdeb9019cc25cfd2d627da751f63bc0f0a30f0dea30b

No original executable bytes committed; CI pending. No claim of complete projectile lifecycle.
