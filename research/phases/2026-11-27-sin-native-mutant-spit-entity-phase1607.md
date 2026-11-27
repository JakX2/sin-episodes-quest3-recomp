# Phase 1607: original CMutantSpit projectile lifecycle

Reopened original private server.dll SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. RTTI 22537790 ".?AVCMutantSpit@@" -> COL 224d2b10 -> primary vtable 224a1914. +64 spawn2237bfa0, +164 contact2237d440, +1c0 damage setter2237d690. Original normal projectile factory 2237c040 creates "gibspit" at index-1, type-checks, dispatches spawn, assigns damage+494, relocates to supplied muzzle, refreshes dirty owner origin then copies owner+354 angles to projectile, sets velocity and owner virtual +50. Spawn invokes precache, movement(4,0), name "spit_mutant" via 22138160, solid2, original model name, bounds [-2,+2] and group19, clears damage. All model/particle/physics operations are owner callbacks, not model loading or rendering reconstruction.

Touch checks other collision flags virtual +fc/+30 bit8 or group19 and returns without RNG/removal on reject. Otherwise consumes discarded float [90,110], plays NPC_BigMomma.SpitTouch1, integer[0,1] chooses SpitHit1/2. If other+94 zero: refresh velocity if dirty then origin twice if dirty, owner trace/decal request with origin and velocity, scale10, mask0x200400b, group0, "MutantSpit". If damageable: damage from +494 type2, projectile used as both damage-info actor arguments, origin then velocity refresh, owner damage-info force via BUCK ammo scale1; apply to other. Remove projectile after either accepted path. No generic collision, damage engine, or model implementation. Fail closed on missing/wrong typed factory result rather than reproducing original null dereference. Separate normal-only API; alternate projectile still inherited. Tests exercise full spawn and factory callback sequence, rejected contact, both accepted branches. Note that callback failure may leave partial host side effects.

VA/bytes/SHA256:
2237c040/161/629c5d3bc092e85a14ed5b47bcd050dcceb9f0d0d7aa9cc6aaf3b8324dc7563f
2237bfa0/150/5ce27cca35156244829d74cce8a963b122c55c9ef6996b6e77c44d5a1f03e157
2237d440/465/0380f62eb0061e0370713dcc133ce9fa364d8e9a9e8e28947ece0a495ca82552

CI pending. No original executable bytes committed.
