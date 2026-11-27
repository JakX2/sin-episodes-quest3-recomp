# Phase 1602: Minor private animation-event registration and dispatch

Direct objdump of original server.dll SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. Five ordered event registrations at 2237e150..2237e1d2. Original +2c0 2237d310 matches live IDs, not fixed animation event numbers. AE_LEAP_TELEGRAPH checks enemy, invokes 2237af80 (two additional enemy queries, +f4 special -> +1c4 target point else +214 center plus10 Z, +120 result discarded), caches vector+c18 and sets+c40. AE_LEAP_ATTACK calls 2237b1e0: attached+c41 suppresses; enemy and cached point consume+c40, otherwise query target point, no enemy uses zero vector and mode1 retaining cache. Probability+c44 >.5 resets .5, otherwise subtracts .1f. Actual +76c launch trajectory remains callback. AE_POUNCE_SLASH resolves+c30, invokes +774, draws twice [5,10], passes angular vector (second,first,0) to 22139c10. AE_SPIT and AE_POUNCE_OFF dispatch to separate unrecovered class-specific helpers 2237c360 and 2237d990. Unrecognized event passes original record to parent. Native event dictionary provides stable private registration/generation checks. No generic animation or collision code recreated. Test covers branch and callback order, stale handle, cache and RNG, unknown event. Owner callbacks are not complete attacks.

VA/bytes/SHA256:
2237d310/290/01b0903b022b3a579f1a935f966bd0880b31b8b354c2d1f9692a6db62c8a71a8
2237af80/199/ed86c19d181e52dbb85badaeabc45a025af4620d7c76d5113586728a10d69a0b
2237b1e0/187/f0fa556da470384831c87937dba4ab47e86bba874ce7a89f675c5535df48ebf3
2237e150/130/a7996522cf08a7113bb07b66355e15fbcaa2a6a6e02d138e1a74045342b41fcb

CI pending.
