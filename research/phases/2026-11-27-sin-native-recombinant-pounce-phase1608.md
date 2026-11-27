# Phase 1608: original Minor pounce-off and slash

Original server.dll SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. Minor +770 2237d990 sets movement(3,0), checks enemy. No enemy -> forward*350 then native phase1606 launch (same owner basis). Live enemy -> +c24 previous launch position minus +c18 cached target, then copy +c24 into +c18, 22027c30 normalizes XY only, preserving Z until overwritten by random [0.5,1]. Second draw [-1,1] adds signed perpendicular (-y,x)*draw to XY. Original float stores retained. Owner vector normalization, scale250; yaw helper 2228b220 then subtract180, motor yaw(heading,-1). Install detached contact marker2237ca30, clear ground, now+.5 contact deadline, velocity, conditional origin refresh, snapshot +c24, +c41 true, airborne think2237c2c0 and schedule now. Unlike no-enemy fallback, this path does not set condition58 or relocate +1. Original +774 2237b520 constructs damage info and calls 2237b480 with live damage 2259d4d8+28, type4, actor both inflictor and attacker; dirty velocity/origin/velocity order then shared force helper 221f8f20 scale1 and apply damage. No generic damage engine or physics recreated. Separate native APIs; event session can connect callbacks. Fixture verifies order, vectors, branch differences and slash owner request.

VA/bytes/SHA256:
2237d990/614/a7f5e851cb8d1a79aaff32d3ea232390306866638cf13c5dfbbccab28303deb7
22027c30/91/7bd375fadb682ca28763503174a92c69ed47dd02d4551d7ac1c5b3d3eace0f48

Additional original slash routines inspected at 2237b480 and 2237b520. CI pending. No original executable bytes committed.
