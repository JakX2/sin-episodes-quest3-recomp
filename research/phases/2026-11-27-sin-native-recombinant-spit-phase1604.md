# Phase 1604: original Minor spit orchestration

Authentic original server SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. 2237c360 checks enemy, reads Acid_spit attachment, reacquires enemy, refreshes dirty origin +b8&800, target +1d4 with actor+2bc and mode1, difference from muzzle. Live gravity cvar 22572dd8+28 clamped to at least1, reacquire enemy and +1c4 target, subtract10 Z, original ballistic helper 2204c850(muzzle,target,gravity,1). Normalize direct aim difference; add original float draws [-.05,.05] twice and [-.05,0] third. Live cvar 2259d568+2c selects normal projectile 2237c040 (direct spread vector*900) or alternate 22311e20 (ballistic vector, zero angular), both use live damage 2259d520+28. Post-projectile effects 2237c59b..2237c7bc remain owner callback with muzzle, ballistic and original +c48/+c4c tokens; no invented particle API. Draw [1.5,2], write +c3c=now+draw. Original x87 AH&5 parity branch: prior+.1 if prior>=.5, otherwise reset .5. Unknown/nonfinite owner input fails closed; no enemy early success without effects or time updates. Shared projectile/ballistic/attachment services owner-controlled; no generic physics or animation rebuilt. Separate callable helper; original AE_SPIT dispatch still uses explicit callback, future owner session can connect this API. Loaded fixture verifies both paths and RNG/callback order.

VA/bytes/SHA256:
2237c360/1238/6acd7c3f945834046782eed0d2d4d9cbb8b49988aa273c7b1828b30a3d2868f6
2237c040/336/5f10b8ddc0c41194095e965ce80972f773607c43d7cfd1cea89352359e3f6e32
2204c850/320/e83d6ef82bc624499aaf2160a9f12e73a2b581c7cfa7b6b19d4c2478ae861f96

CI pending. No original binary bytes committed.
